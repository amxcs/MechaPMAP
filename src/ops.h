#define LV_N 60
static unsigned lv_hist[LV_N],lv_n,lv_best,lv_avg10,lv_lim,lv_lba,lv_size;static int lv_pos=0,lv_pause;    static const char *lv_posname[3]={"inner","middle","outer"};
static int lc_n,lc_cd,lc_c17,lc_fe,lc_te,lc_rf,lc_fe_ok,lc_te_ok,lc_rf_ok,lc_j_ok;static unsigned lc_j[3];static char lc_kind[24],lc_jlim[32];
static int ensure_window(void);static const char *type_name(int t);

static char rng30[16],rng30_ram[40],rngdl[40];
static void set_ranges(void){snprintf(rng30,sizeof rng30,"0x30..0x%02X",P->last30);snprintf(rng30_ram,sizeof rng30_ram,"EEPROM 0x30..0x%02X → MechaCon RAM",P->last30);
 snprintf(rngdl,sizeof rngdl,"words 0x50..0x%02X, both layers",P->last30);}

static int cfg_status(uint8_t command,const uint8_t *in,uint8_t len){
 uint8_t r[16];unsigned attempt;
 for(attempt=0;attempt<300;++attempt){memset(r,0xcc,sizeof r);
  if(MechaScmd(command,(void *)in,len,r,sizeof r)!=1){report("CFG %02x transport fail\n",command);return 0;}
  if(!r[0])return 1;
  if(command!=0x43||r[0]!=1){report("CFG %02x status %02x\n",command,r[0]);return 0;}
  diag_pause();}
 return 0;}
static int cfg_open(unsigned write,unsigned count){uint8_t in[3]={(uint8_t)write,1,(uint8_t)count};return cfg_status(0x40,in,sizeof in);}
static int cfg_close(void){return cfg_status(0x43,NULL,0);}
static int cfg_read(uint8_t *d){memset(d,0xcc,16);return MechaScmd(0x41,NULL,0,d,16)==1;}
static int cfg_write(const uint8_t *d){return cfg_status(0x42,d,16);}

static int cfg_unlock(const uint8_t *before){uint8_t cfg[112];int i;
 if(!cfg_open(0,7))goto fail;
 for(i=0;i<7;++i)if(!cfg_read(cfg+16*i))goto fail;
 if(!cfg_close())goto fail;
 if(memcmp(cfg,before+0x2b0,sizeof cfg)){report("CFG blocks differ from the NVRAM image; nothing written\n");return 0;}
 if(!cfg_open(1,21))goto fail;
 for(i=0;i<7;++i)if(!cfg_write(cfg+16*i))goto fail;
 for(i=0;i<14;++i)if(!cfg_write(P->unlock+16*i))goto fail;
 if(!cfg_close())goto fail;
 if(!cfg_open(1,7))goto fail;
 for(i=0;i<7;++i)if(!cfg_write(cfg+16*i))goto fail;
 if(!cfg_close())goto fail;
 return 1;
fail:report("Config unlock command failed; closing config session\n");cfg_close();return 0;}

static int reread(unsigned char *img){if(!stable_image(img)){fail_("EEPROM read failed. Nothing written.",1);return 0;}return 1;}
static int archive(const char *what,const unsigned char *img){if(!save_seq(what,img)){fail_("USB archive failed. Nothing written.",1);return 0;}return 1;}

static int op_unlock(const char *what){int ok;
 task_begin("UNLOCK · RUNNING","Unlocking…",P->unlock_name,"words 400..511");
 task_row(G_USB,0,"NVRAM read twice + archive",NULL);task_row(G_LOCK,0,"Unlock Mechacon","config blocks + MechaPwn");task_row(G_CHECK,0,"Readback",NULL);
 task_mark(0,1);if(!reread(pre))return 0;
 if(classify(pre)==P_UNKNOWN){fail_("Unknown patch in NVRAM. Nothing written.",0);return 0;}
 if(!archive(what,pre))return 0;
 task_mark(1,1);report("UNLOCK BEGIN source=%s\n",patch_name);flush();wrote_any=1;
 ok=cfg_unlock(pre);
 task_mark(2,1);
 if(!stable_image(now)){fail_("Readback after unlock failed. Keep the USB files; do not restart yet.",0);return 0;}
 if(ok&&!memcmp(now,pre,PATCH)&&!memcmp(now+PATCH,P->unlock,224)){report("UNLOCK verified\n");flush();task_mark(2,2);return 1;}
 if(!memcmp(now,pre,1024)){fail_("Unlock failed. NVRAM verified unchanged.",1);return 0;}
 fail_("Unlock NOT verified. Keep the USB files; do not restart yet.",0);return 0;}

static int known_blob(const unsigned char *blob){unsigned i;if(!memcmp(blob,P->factory,224)||!memcmp(blob,P->hook,224)||!memcmp(blob,P->fuv2,224)||!memcmp(blob,P->unlock,224))return 1;if(P!=&PROFILES[0])return 0;for(i=0;i<sizeof choice_patches/sizeof choice_patches[0];i++)if(!memcmp(blob,choice_patches[i].blob,224))return 1;return 0;}
static int op_patch(const unsigned char *blob,const char *name,const char *what){int i,ok=1;uint16_t w;
 if(!known_blob(blob)){fail_("The patch to write is not a known container. Nothing written.",1);return 0;}
 task_begin("WRITING","Writing patch…",name,"words 400..511");
 task_row(G_USB,0,"NVRAM read twice + archive",NULL);task_row(G_LOCK,0,"Unlock active?","word 400 written back unchanged");
 task_row(G_CHIP,0,"Write patch",name);task_row(G_CHECK,0,"Readback",NULL);
 task_mark(0,1);if(!reread(pre))return 0;
 if(classify(pre)!=P_UNLOCK){fail_("MechaPwn unlock is not in NVRAM. Nothing written.",1);return 0;}
 if(!archive(what,pre))return 0;
 task_mark(1,1);
 if(!diag_read_word(400,&w)||!diag_write_verified(400,w)){fail_("Unlock is not active yet: unplug from the wall and start again. Nothing written.",1);return 0;}
 task_mark(2,1);report("PATCH BEGIN %s\n",name);flush();wrote_any=1;
 for(i=0;i<112;++i){uint16_t word=(uint16_t)(blob[2*i]|(blob[2*i+1]<<8));
  if(memcmp(pre+PATCH+2*i,blob+2*i,2)&&!diag_write_verified(400+i,word)){ok=0;break;}}
 task_mark(3,1);
 if(ok)ok=stable_image(now)&&!memcmp(now,pre,PATCH)&&!memcmp(now+PATCH,blob,224);
 if(ok){report("PATCH verified %s\n",name);flush();task_mark(3,2);return 1;}
 report("Install verification failed; starting restoration\n");flush();
 for(i=400;i<512;i++){uint16_t word=(uint16_t)p142_word(pre,i),obs;
  if(diag_read_word(i,&obs)&&obs==word)continue;if(!diag_write_verified(i,word))report("Restoration word %03x failed\n",i);}
 ok=stable_image(now)&&!memcmp(now,pre,1024);
 fail_(ok?"Write failed. Previous patch restored and verified.":"Write failed; restoration NOT verified. Keep the USB files; do not restart yet.",ok);return 0;}

static int ram_matches(const unsigned char *img){unsigned i,v,bad=0,nw=P->last30-0x30+1,base=(P->ee30-2)&~3u,off=(P->ee30-2)-base,nb=(off+2+2*nw+3)&~3u;unsigned char ram[0x60];
 if(nb>sizeof ram)return 0;
 for(i=0;i<nb;i+=4){if(!ramword(base+i,&v)){report("RAMCOPY read fail\n");return 0;}ram[i]=(unsigned char)v;ram[i+1]=(unsigned char)(v>>8);ram[i+2]=(unsigned char)(v>>16);ram[i+3]=(unsigned char)(v>>24);}
 for(i=0;i<nw;i++){unsigned r=ram[off+2+2*i]|(ram[off+3+2*i]<<8);if(r!=p142_word(img,0x30+i)){if(bad<6)report("RAMCOPY word %03x ram=%04x eeprom=%04x\n",0x30+i,r,p142_word(img,0x30+i));bad++;}}
 report("RAMCOPY 0x30..0x%02x differing=%u\n",P->last30,bad);flush();return bad==0;}
static int op_apply(void){
 task_begin("TEST DISC","Applying values…",NULL,NULL);
 task_row(G_CHIP,0,"Window","uploaded or checked");task_row(-1,"E2","ce2 06",rng30_ram);task_row(G_CHECK,0,"RAM copy = EEPROM",NULL);
 task_mark(0,1);if(classify(img0)!=P_PMAP){fail_("PMAP is not in NVRAM: Test disc needs it. Nothing written.",1);return 0;}
 if(!ensure_window()){fail_("Window upload/check failed. Nothing written.",1);return 0;}
 task_mark(1,1);if(!stable_image(pre)){fail_("EEPROM read failed.",1);return 0;}
 report("RAMCOPY before ce2:\n");ram_matches(pre);
 if(!pmap(14,"ce2_06")){fail_("ce2 06 failed. Nothing written.",1);return 0;}
 task_mark(2,1);if(!ram_matches(pre)){fail_("After ce2 06 the RAM copy differs from the EEPROM. Nothing written.",1);return 0;}
 task_mark(2,2);return 1;}
static int td_n,td_good,td_ok;static char td_what[48],td_detail[96];
static unsigned char td_buf[16*2048] __attribute__((aligned(64)));
static void td_read(int t){sceCdRMode m;unsigned t0=ms();int err=-1,done=0;
 td_n++;snprintf(td_what,sizeof td_what,"%s",type_name(t));
 if(t==0xfd){td_ok=1;td_good++;snprintf(td_detail,sizeof td_detail,"recognised; audio, no data sectors to read");report("TESTDISC type=fd audio recognised\n");flush();return;}
 m.trycount=0;m.spindlctrl=SCECdSpinNom;m.datapattern=SCECdSecS2048;
 if(sceCdRead(16,16,td_buf,&m)){while(ms()-t0<15000){if(!sceCdSync(1)){done=1;break;}pause_ms(5);}if(!done)sceCdBreak();err=sceCdGetError();}
 td_ok=done&&err==0;if(td_ok)td_good++;
 snprintf(td_detail,sizeof td_detail,td_ok?"16 sectors at LBA 16 in %u ms":(done?"read error %02X after %u ms":"no answer in %u ms"),td_ok?ms()-t0:(done?(unsigned)err:ms()-t0),ms()-t0);
 report("TESTDISC type=%02x read=%s err=%d ms=%u\n",t,td_ok?"ok":"fail",err,ms()-t0);flush();}
static int td_edit;
static void test_disc(void){unsigned last=0;int done_for=-1;
 ins_type=ins_ready=ins_lid=-1;td_n=td_good=0;td_edit=0;scr=S_TESTDISC;draw();present();report("TESTDISC BEGIN\n");flush();
 for(;;){u32 p=ReadCombinedPadStatus();
  if(p&PAD_CIRCLE){report("TESTDISC END tried=%d ok=%d\n",td_n,td_good);flush();return;}
  if(p&PAD_TRIANGLE){report("TESTDISC END tried=%d ok=%d -> edit\n",td_n,td_good);flush();td_edit=1;return;}
  if((p&PAD_SQUARE)&&ins_ready==2&&ins_type>=0x10){td_read(ins_type);draw();present();}
  if(ms()-last>250){int lid,t,rd;last=ms();if(!rpc(P142_STATUS,NULL,0,3000)){fail_("Drive status failed.",1);return;}
   lid=(reply.status&SCECdStatShellOpen)?1:0;t=reply.type&255;rd=reply.ready;
   if(lid!=ins_lid||t!=ins_type||rd!=ins_ready){report("DRIVE TEST lid=%s type=%02x ready=%d status=%02x\n",lid?"open":"closed",t,rd,reply.status&255);flush();
    if(lid)done_for=-1;ins_lid=lid;ins_type=t;ins_ready=rd;
    if(!lid&&rd==2&&t>=0x10&&done_for!=t){done_for=t;td_read(t);}
    draw();present();}}
  gsKit_vsync_wait();}}

static void lc_line(int y,const char *name,long v,int ok,const char *txt,const char *lim){char t[64];
 text(f_body,16,SX,y,C_FG,name);
 if(txt)snprintf(t,sizeof t,"%s",txt);else if(!strcmp(name,"Jitter (3x)"))snprintf(t,sizeof t,"%ld",v);else snprintf(t,sizeof t,"%02lX (%ld)",v,v);
 text(f_mono,16,SX+140,y,ok?C_OK:C_WARN,t);textr(f_mono,13,SX+CW,y+2,C_FAINT,lim);}

static int lc_op(unsigned op,unsigned char b[4]){unsigned r,v;if(!hookcall(op,&r)||!ramword(P156_RESULT,&v))return 0;
 b[0]=(unsigned char)v;b[1]=(unsigned char)(v>>8);b[2]=(unsigned char)(v>>16);b[3]=(unsigned char)(v>>24);return 1;}
static void lc_measure(int type){unsigned char b[4];sceCdRMode m;unsigned t0;int i;char line[160];
 lc_cd=type!=0x14&&type!=0xfe;
 if(type!=0xfd){m.trycount=0;m.spindlctrl=SCECdSpinNom;m.datapattern=SCECdSecS2048;t0=ms();
  if(sceCdRead(16,16,td_buf,&m)){while(ms()-t0<15000&&sceCdSync(1))pause_ms(5);}
  report("LASER read err=%d ms=%u\n",sceCdGetError(),ms()-t0);}
 lc_c17=lc_op(1,b)?b[2]:-1;
 lc_fe=lc_op(2,b)&&b[0]==1&&b[1]==0?b[2]:-1;
 lc_te=lc_op(3,b)&&b[0]==1&&b[1]==0?b[2]:-1;
 lc_rf=lc_op(4,b)&&b[0]==2&&b[1]==0?(int)b[2]-(int)b[3]:-999;
 for(i=0;i<3;i++)lc_j[i]=lc_op(5,b)&&b[0]==2&&b[1]==0?(unsigned)(b[2]<<8|b[3]):0xffff;
 {unsigned a=lc_j[0],c=lc_j[1],d=lc_j[2],t;if(a>c){t=a;a=c;c=t;}if(c>d){t=c;c=d;d=t;}if(a>c){t=a;a=c;c=t;}lc_j[0]=a;lc_j[1]=c;lc_j[2]=d;}
 snprintf(lc_kind,sizeof lc_kind,"%s",lc_cd?"CD":lc_c17==0x15?"DVD-DL":"DVD-SL");
 lc_fe_ok=lc_fe>=0x08&&lc_fe<=0x60;lc_te_ok=lc_te>=0x10&&lc_te<=0x60;
 lc_rf_ok=lc_cd?(lc_rf>=0x49&&lc_rf<=0x89):(lc_rf>=0x35);
 if(lc_cd){lc_j_ok=1;snprintf(lc_jlim,sizeof lc_jlim,"CD: not judged");}
 else if(lc_c17==0x15){lc_j_ok=lc_j[1]<=0x4c00;snprintf(lc_jlim,sizeof lc_jlim,"DL <= 19456 / 11520 DEX");}
 else{lc_j_ok=lc_j[1]<=0x3e00;snprintf(lc_jlim,sizeof lc_jlim,"SL <= 15872 / 10608 DEX");}
 lc_n++;
 snprintf(line,sizeof line,"%s,%02x,%d,%d,%d,%u,%u,%u\n",lc_kind,lc_c17&255,lc_fe,lc_te,lc_rf,lc_j[0],lc_j[1],lc_j[2]);
 report("LASER %s",line);flush();
 {char p[96],nm[48];int fd;snprintf(nm,sizeof nm,"pmap_laser_%s_%u.csv",P->name,(unsigned)serial);snprintf(p,sizeof p,"%s%s",device,nm);
  fd=open(p,O_WRONLY|O_CREAT|O_APPEND,0666);if(fd>=0){if(write(fd,line,strlen(line))<0)report("LASER_CSV_FAIL\n");close(fd);}}}

static int lv_seek(void){unsigned t0=ms();
 lv_lba=lv_pos==0?2000:lv_pos==1?lv_size/2:(lv_size>40000?lv_size-20000:lv_size*9/10);
 if(!sceCdSeek(lv_lba))return 0;while(ms()-t0<10000&&sceCdSync(1))pause_ms(5);
 report("LIVE seek %s lba=%u err=%d ms=%u\n",lv_posname[lv_pos],lv_lba,sceCdGetError(),ms()-t0);flush();return 1;}
static void live_jitter(void){unsigned char b[4];unsigned last_seek=0,i,s,c,lv_poll=0;

 lv_size=td_buf[80]|(td_buf[81]<<8)|(td_buf[82]<<16)|((unsigned)td_buf[83]<<24);
 if(lv_size<50000||lv_size>4500000)lv_size=lc_cd?300000:2000000;
 lv_lim=lc_cd?0xffff:lc_c17==0x15?0x4c00:0x3e00;lv_n=0;lv_best=0xffffffffu;lv_avg10=0;
 lv_pause=0;scr=S_LIVE;report("LIVE BEGIN %s size=%u\n",lc_kind,lv_size);flush();lv_seek();last_seek=ms();draw();present();
 for(;;){u32 p=ReadCombinedPadStatus();
  if(p&PAD_CIRCLE){report("LIVE END n=%u best=%u\n",lv_n,lv_best);flush();scr=S_LASER;return;}
  if(p&PAD_SQUARE){lv_pause=!lv_pause;report("LIVE %s\n",lv_pause?"PAUSE":"RESUME");draw();present();}

  if(ms()-lv_poll>500){lv_poll=ms();if(rpc(P142_STATUS,NULL,0,3000)&&(reply.status&SCECdStatShellOpen)){
    report("LIVE END lid=open n=%u best=%u\n",lv_n,lv_best);flush();ins_lid=ins_type=ins_ready=-1;scr=S_LASER;return;}}
  if(lv_pause){gsKit_vsync_wait();continue;}
  if(p&(PAD_LEFT|PAD_RIGHT)){lv_pos=(lv_pos+(p&PAD_RIGHT?1:2))%3;lv_n=0;lv_best=0xffffffffu;lv_seek();last_seek=ms();}
  if(ms()-last_seek>15000){lv_seek();last_seek=ms();}
  if(lc_op(5,b)&&b[0]==2&&b[1]==0){unsigned v=(unsigned)(b[2]<<8|b[3]);lv_hist[lv_n%LV_N]=v;lv_n++;if(v<lv_best)lv_best=v;
   for(s=0,c=0,i=0;i<10&&i<lv_n;i++,c++)s+=lv_hist[(lv_n-1-i)%LV_N];lv_avg10=c?s/c:0;
   if(lv_n%20==1)report("LIVE %s n=%u jitter=%u best=%u avg10=%u\n",lv_posname[lv_pos],lv_n,v,lv_best,lv_avg10);}
  draw();present();}}

#define TN_N 5
static const char *tn_name[TN_N]={"FE gain","TE gain","F0","F4","Reg 0x16"};
static const int tn_field[TN_N]={10,11,9,12,13};
static const int tn_op[TN_N]={0x24,0x25,0x26,0x27,0x22};
static int tn_val[TN_N],tn_start[TN_N],tn_ee[TN_N],tn_known[TN_N],tn_set[TN_N],tn_sel,tn_edit,tn_resets,tn_errs;
static unsigned tn_n,tn_j,tn_best,tn_avg10,tn_hist[LV_N],tn_mb100,tn_reads;
static char tn_pt[4][96];static int tn_npt;
static unsigned tn_pj,tn_pc,tn_pm,tn_pmc;
static unsigned char tn_buf[32*2048] __attribute__((aligned(64)));
static int tn_lo(int i){return i==2||i==3?-256:0;}
static int tn_hi(int i){return i==2||i==3?255:i==4?0x7f:255;}
static int sext9(unsigned v){v&=0x1ffu;return (v&0x100u)?(int)v-512:(int)v;}
static int tn_write(int i,int v){unsigned r;
 if(!hookcall((unsigned)tn_op[i]|(((unsigned)v&0x1ffu)<<16),&r)){report("TUNE SET %s %d RPC_FAIL\n",tn_name[i],v);return 0;}return 1;}
static int tn_read(int i,int *v){unsigned char b[4];unsigned r;
 if(i==0||i==1){if(!lc_op(i==0?2:3,b))return 0;*v=b[2];return 1;}
 if(i==3){if(!hookcall(0x21,&r))return 0;*v=sext9(r);return 1;}
 return 0;}
static void tn_point(void){
 if(!tn_pc&&!tn_pmc)return;
 {char *s=tn_pt[tn_npt%4];unsigned j=tn_pc?tn_pj/tn_pc:0,m=tn_pmc?tn_pm/tn_pmc:0;char p[96];int fd,n;
  snprintf(s,96,"FE %d TE %d F0 %d F4 %d R16 %d · jitter %u · %u.%02u MB/s",tn_val[0],tn_val[1],tn_val[2],tn_val[3],tn_val[4],j,m/100,m%100);tn_npt++;
  report("TUNE POINT FE=%d TE=%d F0=%d F4=%d R16=%d jitter_avg=%u n=%u mbps100=%u reads=%u\n",tn_val[0],tn_val[1],tn_val[2],tn_val[3],tn_val[4],j,tn_pc,m,tn_pmc);
  n=snprintf(p,sizeof p,"%spmap_tune_%s_%s.csv",device,P->name,serial_txt);(void)n;
  fd=open(p,O_WRONLY|O_CREAT|O_APPEND,0666);
  if(fd>=0){char line[128];int k=snprintf(line,sizeof line,"%d,%d,%d,%d,%d,%u,%u,%u,%u\n",tn_val[0],tn_val[1],tn_val[2],tn_val[3],tn_val[4],j,tn_pc,m,tn_pmc);if(write(fd,line,k)!=k)report("TUNE CSV_SHORT\n");close(fd);}}
 tn_pj=tn_pc=tn_pm=tn_pmc=0;}
static void tn_measure(void){unsigned char b[4];int i,v;
 for(i=0;i<TN_N;i++)if(tn_set[i]&&tn_read(i,&v)&&v!=tn_val[i]){tn_resets++;
  report("TUNE RESET_BY_FIRMWARE %s dsp=%d want=%d\n",tn_name[i],v,tn_val[i]);tn_write(i,tn_val[i]);}
 if(lc_op(5,b)&&b[0]==2&&b[1]==0){unsigned j=(unsigned)(b[2]<<8|b[3]),s=0,c=0,k;tn_hist[tn_n%LV_N]=j;tn_n++;if(j<tn_best)tn_best=j;
  for(k=0;k<10&&k<tn_n;k++,c++)s+=tn_hist[(tn_n-1-k)%LV_N];tn_avg10=c?s/c:0;tn_j=j;tn_pj+=j;tn_pc++;}
 if(tn_n%2==0){sceCdRMode m;unsigned t0=ms(),lba=2000+(tn_reads%32)*32,dt;int done=0,err;
  m.trycount=0;m.spindlctrl=SCECdSpinNom;m.datapattern=SCECdSecS2048;
  if(sceCdRead(lba,32,tn_buf,&m)){while(ms()-t0<15000){if(!sceCdSync(1)){done=1;break;}pause_ms(2);}if(!done)sceCdBreak();}
  err=sceCdGetError();dt=ms()-t0;tn_reads++;
  if(done&&!err&&dt){tn_mb100=(6554u+dt/2)/dt;tn_pm+=tn_mb100;tn_pmc++;}else{tn_errs++;tn_mb100=0;report("TUNE READ lba=%u err=%d done=%d ms=%u\n",lba,err,done,dt);}}
 if(tn_n%10==1)report("TUNE n=%u FE=%d TE=%d F0=%d F4=%d R16=%d jitter=%u best=%u avg10=%u mbps100=%u resets=%d errs=%d\n",
  tn_n,tn_val[0],tn_val[1],tn_val[2],tn_val[3],tn_val[4],tn_j,tn_best,tn_avg10,tn_mb100,tn_resets,tn_errs);}
static void tn_restore(void){int i;for(i=0;i<TN_N;i++)if(tn_set[i]){tn_write(i,tn_start[i]);tn_val[i]=tn_start[i];tn_set[i]=0;}}
static void live_tune(void){int i,v;const u32 AR=PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT;u32 held=0;unsigned t0=0,next=0;
 tn_edit=0;tn_resets=tn_errs=0;tn_n=0;tn_best=0xffffffffu;tn_avg10=tn_j=tn_mb100=tn_reads=0;tn_npt=0;tn_pj=tn_pc=tn_pm=tn_pmc=0;tn_sel=0;
 for(i=0;i<TN_N;i++){tn_ee[i]=F[tn_field[i]].now;tn_set[i]=0;tn_known[i]=tn_read(i,&v);tn_val[i]=tn_start[i]=tn_known[i]?v:tn_ee[i];}
 report("TUNE BEGIN FE=%d TE=%d F4=%d (DSP) F0=%d R16=%d (EEPROM) eeprom FE=%d TE=%d F4=%d\n",tn_val[0],tn_val[1],tn_val[3],tn_val[2],tn_val[4],tn_ee[0],tn_ee[1],tn_ee[3]);flush();
 if(sceCdSeek(2000)){unsigned s0=ms();while(ms()-s0<10000&&sceCdSync(1))pause_ms(5);}
 scr=S_TUNE;draw();present();
 for(;;){u32 p=ReadCombinedPadStatus(),h=(u32)ReadCombinedPadStatus_raw()&AR;unsigned now=ms();int step=1;
  if(p&AR){held=p&AR;t0=now;next=now+200;}
  else if(held&&(h&held)==held){if((int)(now-next)>=0){p|=held;if(now-t0>1000)step=4;next=now+(now-t0>1000?30:45);}}
  else held=0;
  if(p&PAD_CIRCLE){tn_point();tn_restore();report("TUNE END n=%u resets=%d errs=%d\n",tn_n,tn_resets,tn_errs);flush();scr=S_LASER;return;}
  if(p&PAD_TRIANGLE){tn_point();for(i=0;i<TN_N;i++)if(tn_set[i])tn_ee[i]=tn_val[i];tn_restore();tn_edit=1;
   report("TUNE TO_EDIT FE=%d TE=%d F0=%d F4=%d R16=%d\n",tn_ee[0],tn_ee[1],tn_ee[2],tn_ee[3],tn_ee[4]);flush();scr=S_LASER;return;}
  if(p&PAD_SQUARE){tn_point();tn_restore();report("TUNE RESET to start values\n");flush();}
  if(p&PAD_UP){tn_sel=tn_sel?tn_sel-1:TN_N-1;}
  if(p&PAD_DOWN){tn_sel=(tn_sel+1)%TN_N;}
  if(p&(PAD_LEFT|PAD_RIGHT)){int nv=tn_val[tn_sel]+((p&PAD_RIGHT)?step:-step);
   if(nv<tn_lo(tn_sel))nv=tn_lo(tn_sel);if(nv>tn_hi(tn_sel))nv=tn_hi(tn_sel);
   if(nv!=tn_val[tn_sel]){tn_point();if(tn_write(tn_sel,nv)){report("TUNE SET %s %d -> %d\n",tn_name[tn_sel],tn_val[tn_sel],nv);tn_val[tn_sel]=nv;tn_set[tn_sel]=1;flush();}}}
  if(p){draw();present();}
  tn_measure();draw();present();}}
static void laser_check(void){unsigned last=0;int done_for=-1;
 tn_edit=0;
 ins_type=ins_ready=ins_lid=-1;lc_n=0;scr=S_LASER;draw();present();report("LASER BEGIN\n");flush();
 for(;;){u32 p=ReadCombinedPadStatus();
  if(p&PAD_CIRCLE){report("LASER END n=%d\n",lc_n);flush();return;}
  if((p&PAD_SQUARE)&&ins_ready==2&&ins_type>=0x10){lc_measure(ins_type);draw();present();}
  if((p&PAD_CROSS)&&P->walk&&ins_ready==2&&ins_type==0x14&&lc_n&&lc_c17==0x14){live_tune();if(tn_edit)return;draw();present();}
  if((p&PAD_TRIANGLE)&&ins_ready==2&&ins_type>=0x10&&ins_type!=0xfd){if(!lc_n||done_for!=ins_type)lc_measure(ins_type);live_jitter();draw();present();}
  if(ms()-last>250){int lid,t,rd;last=ms();if(!rpc(P142_STATUS,NULL,0,3000)){fail_("Drive status failed.",1);return;}
   lid=(reply.status&SCECdStatShellOpen)?1:0;t=reply.type&255;rd=reply.ready;
   if(lid!=ins_lid||t!=ins_type||rd!=ins_ready){report("DRIVE LASER lid=%s type=%02x ready=%d\n",lid?"open":"closed",t,rd);flush();
    if(lid)done_for=-1;ins_lid=lid;ins_type=t;ins_ready=rd;
    if(!lid&&rd==2&&t>=0x10&&done_for!=t){done_for=t;draw();present();lc_measure(t);}
    draw();present();}}
  gsKit_vsync_wait();}}

static int op_region(int r){unsigned char tgt[1024];int i,ok=1;uint16_t w;
 task_begin("FINISH · REGION","Writing region…",REGIONS[r].name,"words 192..197, 227..236, 320..327");
 task_row(G_USB,0,"NVRAM read twice + archive",NULL);task_row(G_LOCK,0,"Unlock active?",NULL);
 task_row(G_CHIP,0,"Write Retail-DEX region",REGIONS[r].name);task_row(G_CHECK,0,"Readback",NULL);
 task_mark(0,1);if(!reread(pre))return 0;
 if(classify(pre)!=P_UNLOCK){fail_("MechaPwn unlock is not in NVRAM. Nothing written.",1);return 0;}
 if(!archive("before_region",pre))return 0;
 task_mark(1,1);
 if(!diag_read_word(400,&w)||!diag_write_verified(400,w)){fail_("Unlock is not active yet: unplug from the wall and start again. Nothing written.",1);return 0;}
 memcpy(tgt,pre,1024);region_apply(tgt,r);
 for(i=0;i<512;i++)if(p142_word(tgt,i)!=p142_word(pre,i)&&!region_word((unsigned)i)){fail_("Region write outside its words. Nothing written.",1);return 0;}
 task_mark(2,1);report("REGION BEGIN %s\n",REGIONS[r].name);flush();wrote_any=1;
 for(i=0;i<512&&ok;i++)if(p142_word(tgt,i)!=p142_word(pre,i)&&!diag_write_verified((uint16_t)i,(uint16_t)p142_word(tgt,i))){report("REGION word %03x failed\n",i);ok=0;}
 task_mark(3,1);
 if(ok)ok=stable_image(now)&&!memcmp(now,tgt,1024);
 if(ok){report("REGION verified %s\n",REGIONS[r].name);flush();task_mark(3,2);return 1;}
 for(i=0;i<512;i++){uint16_t obs;if(!region_word((unsigned)i)||(diag_read_word((uint16_t)i,&obs)&&obs==p142_word(pre,i)))continue;
  if(!diag_write_verified((uint16_t)i,(uint16_t)p142_word(pre,i)))report("REGION rollback %03x failed\n",i);}
 ok=stable_image(now)&&!memcmp(now,pre,1024);
 fail_(ok?"Region write failed. Previous region written back and verified.":"Region write failed; rollback NOT verified. Keep the USB files; do not restart yet.",ok);return 0;}

static int op_restore_full(void){int i,ok=1;static const int sums[5]={0x00,0x30,0x60,0x80,0xa0};
 task_begin("RESTORE · RUNNING","Writing the backup…",NULL,bakname);
 task_row(G_USB,0,"NVRAM read twice + archive",NULL);task_row(G_LOCK,0,"Unlock active?",NULL);
 task_row(G_RESTORE,0,"Write back to NVRAM","calibration words and patch region");task_row(G_CHECK,0,"Readback = backup",NULL);
 task_mark(0,1);if(!reread(pre))return 0;
 for(i=0;i<512;i++)if(p142_word(pre,i)!=p142_word(bak,i)&&!(i>=0x30&&i<=(int)P->last30)&&!(i>=400)&&!region_word((unsigned)i)){
  char m[96];snprintf(m,sizeof m,"Word %03X differs outside calibration, region and patch. Nothing written.",i);fail_(m,1);return 0;}
 if(!archive("before_restore",pre))return 0;
 task_mark(1,1);
 if(memcmp(pre+PATCH,bak+PATCH,224)){uint16_t w;if(classify(pre)!=P_UNLOCK||!diag_read_word(400,&w)||!diag_write_verified(400,w)){
   fail_("Unlock is not active yet: unplug from the wall and start again. Nothing written.",1);return 0;}}
 task_mark(2,1);report("RESTORE BEGIN\n");flush();wrote_any=1;
 for(i=0;i<512&&ok;i++){int k,is_sum=0;for(k=0;k<5;k++)if(i==sums[k])is_sum=1;
  if(!is_sum&&p142_word(pre,i)!=p142_word(bak,i)&&!diag_write_verified((uint16_t)i,(uint16_t)p142_word(bak,i))){report("RESTORE word %03x failed\n",i);ok=0;}}
 for(i=0;i<5&&ok;i++)if(p142_word(pre,sums[i])!=p142_word(bak,sums[i])&&!diag_write_verified((uint16_t)sums[i],(uint16_t)p142_word(bak,sums[i]))){report("RESTORE sum %03x failed\n",sums[i]);ok=0;}
 task_mark(3,1);
 ok=ok&&stable_image(now)&&!memcmp(now,bak,1024);
 if(!ok){int back=1;report("Restore failed; writing the previous image back\n");flush();
  for(i=0;i<512;i++){int k,is_sum=0;uint16_t obs;for(k=0;k<5;k++)if(i==sums[k])is_sum=1;
   if(is_sum||(diag_read_word((uint16_t)i,&obs)&&obs==p142_word(pre,i)))continue;
   if(!diag_write_verified((uint16_t)i,(uint16_t)p142_word(pre,i))){report("ROLLBACK word %03x failed\n",i);back=0;}}
  for(i=0;i<5;i++){uint16_t obs;if(diag_read_word((uint16_t)sums[i],&obs)&&obs==p142_word(pre,sums[i]))continue;
   if(!diag_write_verified((uint16_t)sums[i],(uint16_t)p142_word(pre,sums[i]))){report("ROLLBACK sum %03x failed\n",sums[i]);back=0;}}
  back=back&&stable_image(now)&&!memcmp(now,pre,1024);
  fail_(back?"Restore failed. The previous NVRAM was written back and verified.":"Restore failed; rollback NOT verified. Keep the USB files; do not restart yet.",back);return 0;}
 report("RESTORED full_eeprom_equal_backup=1\n");flush();task_mark(3,2);return 1;}

static int win_up;
static int ensure_window(void){unsigned off,v=0,e;

 if(!win_up){unsigned m=0;if(ramword(WINDOW,&m)&&m==ARM_MAGIC){report("WINDOW_ALREADY_ARMED; checking the code\n");win_up=1;}
  else{if(!upload_window())return 0;win_up=1;return 1;}}
 for(off=0;off<P156_JOB-WINDOW;off+=4){e=off?(unsigned)P->win[off]|((unsigned)P->win[off+1]<<8)|((unsigned)P->win[off+2]<<16)|((unsigned)P->win[off+3]<<24):ARM_MAGIC;
  if(!ramword(WINDOW+off,&v)||v!=e){report("WINDOW_CHANGED offset=%03x got=%08x\n",off,v);return 0;}}
 report("WINDOW_INTACT code_bytes=%u\n",(unsigned)(P156_JOB-WINDOW));return 1;}
static int sumfix(const char *label){unsigned sum;if(!live_sum30(&sum)||!write_word(0x30,sum)){report("%s CHECKSUM_FAIL\n",label);return 0;}report("%s CHECKSUM 030=%04x\n",label,sum);return 1;}

static int restore_to(const unsigned char *ref){unsigned i;int ok=1;
 if(ncmd_busy){report("RESTORE_ABORT N-command still busy\n");return 0;}
 if(!stable_image(now))return 0;
 for(i=0;i<512;i++)if(p142_word(now,i)!=p142_word(ref,i)&&!(i>=0x30&&i<=P->last30)){report("RESTORE_ABORT word %03x outside 0x30..0x%02x\n",i,P->last30);return 0;}
 for(i=0x31;i<=P->last30;i++)if(p142_word(now,i)!=p142_word(ref,i)&&!write_word(i,p142_word(ref,i)))ok=0;
 if(ok){ok=stable_image(now);for(i=0;ok&&i<512;i++)if(i!=0x30&&p142_word(now,i)!=p142_word(ref,i))ok=0;}
 if(ok&&p142_word(now,0x30)!=p142_word(ref,0x30)&&!write_word(0x30,p142_word(ref,0x30)))ok=0;
 ok=ok&&stable_image(now)&&!memcmp(now,ref,1024);
 report("RESTORE_TO full_eeprom_equal=%d\n",ok);flush();return ok;}
static int disc_ok(const char *kind,int t){return !strcmp(kind,"CD")?(p156_type_is_cd(t)||t==0xfd):(t==0x14||t==0xfe);}

static int ins_swap;
static int wait_disc(const char *kind){unsigned last=0;int dvd=strcmp(kind,"CD")!=0,opened=!ins_swap;ins_swap=0;
 ins_kind=kind;ins_type=ins_ready=ins_lid=-1;ins_force=0;ins_since=ms();scr=S_INSERT;draw();present();
 for(;;){u32 p=ReadCombinedPadStatus();if(p&PAD_CIRCLE)return 0;
  if(ins_force&&(p&PAD_SQUARE)){report("DISC %s FORCED type=%02x ready=%d\n",kind,ins_type&255,ins_ready);return 2;}
  if(ms()-last>250){int lid,t,rd,chg;last=ms();if(!rpc(P142_STATUS,NULL,0,3000)){fail_("Drive status failed.",1);return -1;}
   lid=(reply.status&SCECdStatShellOpen)?1:0;t=reply.type&255;rd=reply.ready;chg=lid!=ins_lid||t!=ins_type||rd!=ins_ready;
   if(chg){report("DRIVE %s lid=%s type=%02x ready=%d status=%02x\n",kind,lid?"open":"closed",t,rd,reply.status&255);flush();
    if(lid!=ins_lid||t!=ins_type)ins_since=ms();ins_lid=lid;ins_type=t;ins_ready=rd;}
   if(lid)opened=1;
   if(opened&&!lid&&rd==2&&disc_ok(kind,t)){report("DISC %s type=%02x\n",kind,t);return 1;}
   {int f=opened&&dvd&&!lid&&t!=0&&ms()-ins_since>20000;if(f!=ins_force)chg=1;ins_force=f;}
   if(chg){draw();present();}}
  gsKit_vsync_wait();}}
static int c97(unsigned jobop,const char *label){unsigned r=0;
 if(!job(jobop)||!ramword(P156_RESULT,&r)||((r>>8)&255)!=0){report("%s FAIL result=%08x\n",label,r);return 0;}
 report("%s ok result=%08x\n",label,r);return sumfix(label);}
static int adj(unsigned jobop,const char *label){unsigned r=0;
 if(!job(jobop)||!sumfix(label)||!ramword(P156_RESULT,&r)||!p156_adj_ok(r)){report("%s FAIL result=%08x\n",label,r);return 0;}
 report("%s ok class=%u\n",label,(r>>16)&255);return 1;}

static int elect_disc(const char *kind,int keep){int ok=0,started=0,r,thr=0,forced;unsigned ret=0,t33=0,t34=0;const char *step="start";
 r=wait_disc(kind);if(r<=0)return r;forced=r==2;
 if(!strcmp(kind,"CD")){task_begin(keep?"STEP 3 · CD · WRITES PERMANENTLY":"MEASURE ONLY · CD","Calibrating CD…","ca3 01","0x30..0x47");
  task_row(G_CD,0,"c11 · c22 00 · c44 · c44","CD 12 cm mode, as PMAP");task_row(-1,"97","c97 00 · CD detect adjust","words 0x33, 0x34");
  task_row(-1,"A3","c10 · ca3 01 · CD auto adjustment","words 0x40..0x47");task_row(G_CHECK,0,"Checksums + readback",NULL);}
 else if(!strcmp(kind,"DVD-SL")){task_begin(keep?"STEP 3 · DVD-SL · WRITES PERMANENTLY":"MEASURE ONLY · DVD-SL","Calibrating DVD-SL…","ca5 0105","0x30..0x4F");
  task_row(G_DVD,0,"c44 · c16 / c17 · c14","expect 01 00 14, DVD-SL mode");task_row(-1,"97","c97 01 · c97 03","DVD detect, then the midpoint");
  task_row(-1,"A5","ca5 0105 · DVD-SL auto adjustment","words 0x48..0x4F");task_row(G_CHECK,0,"Checksums + readback",NULL);}
 else{task_begin(keep?"STEP 3 · DVD-DL · WRITES PERMANENTLY":"MEASURE ONLY · DVD-DL","Calibrating DVD-DL…","ca3 01",rng30);
  task_row(G_DVD,0,"c44 · c16 / c17 · c15","expect 01 00 15, DVD-DL mode");task_row(-1,"A3","ca3 01 · DVD-DL auto adjustment",rngdl);
  task_row(-1,"F2","cf2 · layer 1","gain / jitter checks");task_row(G_CHECK,0,"Checksums + readback",NULL);}
 task_mark(0,1);report("ELECT %s keep=%d BEGIN\n",kind,keep);flush();
 step="stop";if(!callrpc(P142_STOP,NULL,0)||(!forced&&reply.ready!=2))goto out;
 if(forced)report("STOP done ready=%d type=%02x (forced; c16/c17 decide)\n",reply.ready,reply.type&255);
 step="window";if(!ensure_window())goto out;
 step="eeprom";if(!stable_image(pre)||classify(pre)!=P_PMAP||!p142_all_sums_ok(pre))goto out;
 {char a[24];snprintf(a,sizeof a,"before_%s",kind);step="archive";if(!save_seq(a,pre))goto out;}
 if(!strcmp(kind,"CD")){
  step="c11..c44";if(!pmap(13,"c11")||!pmap(10,"c22_00")||!pmap(9,"c44")||!pmap(9,"c44_again"))goto out;
  task_mark(1,1);started=1;wrote_any|=keep;step="c97 00";if(!c97(0x17,"C97_00"))goto out;
  if(read_word(0x33,&t33)&&read_word(0x34,&t34)){thr=1;report("C97_00 033=%u 034=%u\n",t33,t34);}
  task_mark(2,1);step="c10";if(!pmap(11,"c10"))goto out;step="ca3 01";if(!adj(0x12,"CA3_CD"))goto out;
  if(!pmap(2,"CD_FE_cd3_13")||!pmap(3,"CD_TE_cd3_23"))goto out;}
 else if(!strcmp(kind,"DVD-SL")){
  step="c44";if(!pmap(9,"c44"))goto out;
  step="c16/c17";if(!job(0x11)||!pmap(1,"c17")||!ramword(P156_RESULT,&ret))goto out;
  if(!p156_c17_is_sl(ret)){step="not a single-layer DVD";goto out;}
  step="c14";if(!pmap(12,"c14"))goto out;
  task_mark(1,1);started=1;wrote_any|=keep;step="c97 01";if(!c97(0x15,"C97_01"))goto out;step="c97 03";if(!c97(0x16,"C97_03"))goto out;
  task_mark(2,1);step="ca5 0105";if(!adj(0x14,"CA5_SL"))goto out;
  if(!pmap(4,"SL_ca7")||!pmap(2,"SL_FE")||!pmap(3,"SL_TE")||!pmap(5,"SL_ce9_a")||!pmap(6,"SL_cd1_d6d8")||!pmap(5,"SL_ce9_b")||!pmap(7,"SL_cd1_d6de"))goto out;}
 else{
  step="c44";if(!pmap(9,"c44"))goto out;
  step="c16/c17";if(!job(0x11)||!pmap(1,"c17")||!ramword(P156_RESULT,&ret))goto out;
  if((ret&0x00ffffffu)!=0x00150001u){step="not a dual-layer DVD";goto out;}
  step="c15";if(!pmap(8,"c15"))goto out;
  task_mark(1,1);started=1;wrote_any|=keep;step="ca3 01";if(!adj(0x12,"CA3_DL"))goto out;
  if(!pmap(4,"L0_ca7")||!pmap(2,"L0_FE")||!pmap(3,"L0_TE")||!pmap(5,"L0_ce9_a")||!pmap(6,"L0_cd1_d6d8")||!pmap(5,"L0_ce9_b")||!pmap(7,"L0_cd1_d6de"))goto out;
  task_mark(2,1);step="cf2";ret=0xffffffffu;if(!job(0x13)||!ramword(P156_RESULT,&ret)||(ret&0xffffu)!=0)goto out;
  if(!pmap(4,"L1_ca7")||!pmap(2,"L1_FE")||!pmap(3,"L1_TE")||!pmap(5,"L1_ce9"))goto out;}
 task_mark(3,1);step="c22";if(!pmap(10,"c22_stop"))goto out;
 step="readback";if(!stable_image(now)||!only_kind_changes(kind,pre,now)||!p142_all_sums_ok(now))goto out;
 runs_add(kind,now);
 if(!keep){step="restore";if(!restore_to(pre))goto out;}
 ok=1;
out:
 if(!ok){char m[120];int back=1;report("ELECT %s FAIL at %s\n",kind,step);
  if(thr&&!strcmp(step,"ca3 01"))runs_add_thr((int)t33,(int)t34);
  if(started&&!restore_to(pre)){back=0;snprintf(m,sizeof m,"%s failed at %s; restore NOT verified. Keep the USB files.",kind,step);}
  else snprintf(m,sizeof m,"%s failed at %s.%s%s",kind,step,started?" This disc's values were put back.":" Nothing written.",
   thr&&!strcmp(step,"ca3 01")?" The c97 00 thresholds were saved as a run.":"");
  fail_(m,back);return -1;}
 report("ELECT %s keep=%d OK\n",kind,keep);flush();task_mark(3,2);return 1;}

static unsigned char img_start[1024];
static void result_from(const unsigned char *a,const unsigned char *b){int i;for(i=0;i<NF;i++){res_before[i]=fget(&F[i],a);res_after[i]=fget(&F[i],b);F[i].now=F[i].val=res_after[i];}}
static void op_calibrate(void){static const char *k[3]={"CD","DVD-SL","DVD-DL"};int i,r;
 if(!stable_image(img_start)){fail_("EEPROM read failed. Nothing written.",1);return;}
 for(i=0;i<3;i++){ins_swap=i>0;r=elect_disc(k[i],1);if(r==0){scr=S_CALIB_INTRO;if(i)result_from(img_start,now),scr=S_RESULT;return;}if(r<0)return;}
 result_from(img_start,now);sel=0;scr=S_RESULT;}

static void op_edit_write(void){unsigned char tgt[1024];int i;unsigned sum;
 task_begin("ADVANCED · WRITING","Writing values…","writing",rng30);
 task_row(G_USB,0,"NVRAM read twice + archive",NULL);task_row(G_CHIP,0,"Write each word, read it back","stock NVRAM write");
 task_row(-1,"30","Checksum 0x30","written last");task_row(G_CHECK,0,"Full image compare",NULL);
 task_mark(0,1);if(!reread(pre))return;
 for(i=0;i<NF;i++)if(F[i].val!=F[i].now&&fget(&F[i],pre)!=F[i].now){fail_("NVRAM changed since Edit values was opened. Nothing written.",1);return;}
 memcpy(tgt,pre,1024);for(i=0;i<NF;i++)if(F[i].val!=F[i].now)p142_put_word(tgt,F[i].word,fput(&F[i],p142_word(tgt,F[i].word),F[i].val));
 sum=p142_sum30(tgt);p142_put_word(tgt,0x30,sum);
 if(!archive("before_edit",pre))return;
 task_mark(1,1);report("EDIT BEGIN\n");flush();wrote_any=1;
 for(i=0x31;i<=(int)P->last30;i++)if(p142_word(pre,i)!=p142_word(tgt,i)&&!write_word(i,p142_word(tgt,i))){if(!restore_to(pre))fail_("Write failed; restore NOT verified. Keep the USB files.",0);else fail_("Write failed. Previous values put back.",1);return;}
 task_mark(2,1);if(p142_word(pre,0x30)!=sum&&!write_word(0x30,sum)){if(!restore_to(pre))fail_("Checksum write failed; restore NOT verified.",0);else fail_("Checksum write failed. Previous values put back.",1);return;}
 task_mark(3,1);if(!stable_image(now)||memcmp(now,tgt,1024)){if(restore_to(pre))fail_("Readback differed. Previous values put back.",1);else fail_("Readback differed; restore NOT verified. Keep the USB files.",0);return;}
 report("EDIT verified\n");flush();result_from(pre,now);sel=0;scr=S_RESULT;}
