static const char *type_name(int t){return t<0?"?":t==0?"no disc":t<=3?"detecting":t==0x10?"PS1 CD":t==0x11?"PS1 CD+DA":
 t==0x12?"PS2 CD":t==0x13?"PS2 CD+DA":t==0x14?"PS2 DVD":t==0xfd?"audio CD":t==0xfe?"DVD-Video":t==0x05?"detect error":"not recognised";}
static void power_screen(void){
 eyebrow(pw_eb);
 int y=cur_y,iw=288,ih=162,i,j;
 box(SX,y,iw+12,ih+12,12,0x000000,0x4a3a1c,1);
 const unsigned char *p=pwr70k_rgb;
 for(j=0;j<ih;j++)for(i=0;i<iw;i++){int sx=i*400/iw,sy=j*225/ih;const unsigned char *q=p+(sy*400+sx)*3;blend(SX+6+i,y+6+j,(u32)(q[0]<<16|q[1]<<8|q[2]),255);}
 int tx=SX+iw+28,tw=CW-iw-28;
 text(f_disp,26,tx,y+4,C_FG,pw_title);para(f_body,17,23,tx,y+44,tw,C_MUTED,pw_lede);
 cur_y=y+ih+24;note(1,pw_note);
 for(i=0;i<4;i++)act[i]=-1;}

static char finish_txt[80];
static const char *finish_target(void){int r=pending_region();
 if(r>=0){snprintf(finish_txt,sizeof finish_txt,"Retail-DEX %s, then Force Unlock v2",REGIONS[r].name);return finish_txt;}
 return !strncmp(pending,"v2",2)?"Force Unlock v2":"the NVRAM patch";}
static int step_now(void){
 if(scr==S_ERROR||scr==S_POWER)return -1;
 if(scr==S_UNLOCK_CONFIRM)return 0;if(scr==S_HOOK_CONFIRM)return 1;
 if(scr==S_FINISH||scr==S_FINISH_B||scr==S_DONE)return 3;
 if(scr>=S_CALIB_INTRO&&scr<=S_ADV_REVIEW)return 2;
 return patch_kind==P_UNLOCK?(pending[0]?3:1):patch_kind==P_PMAP?2:0;}
static void draw(void){
 header();foot[0]=0;
 int done=patch_kind==P_UNLOCK?(pending[0]&&strcmp(pending,"restore")?3:1):patch_kind==P_PMAP?2:0;
 if(scr==S_DONE)done=4;
 rail(step_now(),done);
 switch(scr){
 case S_WELCOME:{char b[96];int tone;
  eyebrow("MECHACON FIRMWARE SETUP");
  if(patch_kind==P_UNLOCK&&pending[0])snprintf(b,sizeof b,"DETECTED: UNLOCKED · %s PENDING",!strcmp(pending,"restore")?"RESTORE":"FINISH"),tone=1;
  else if(patch_kind==P_UNLOCK)snprintf(b,sizeof b,"DETECTED: UNLOCKED · NO PMAP"),tone=1;
  else if(patch_kind==P_PMAP)snprintf(b,sizeof b,"DETECTED: READY · PMAP ACTIVE"),tone=2;
  else if(patch_kind==P_UNKNOWN)snprintf(b,sizeof b,"DETECTED: UNKNOWN PATCH · NOTHING CAN BE WRITTEN"),tone=0;
  else snprintf(b,sizeof b,"DETECTED: %s",patch_name),tone=0;
  badge(b,tone);
  {int r=region_of(img0);char t[80];if(r>=0)snprintf(t,sizeof t,"Region: Retail-DEX · %s",REGIONS[r].name);else snprintf(t,sizeof t,"Region: Factory / CEX");
   text(f_mono,13,SX+4,cur_y-6,C_MUTED,t);cur_y+=14;}
  row(-1,"1","Unlock","Back up NVRAM, then Unlock Mechacon",done>0?"OK":"pending",done>0?2:0);
  row(-1,"2","Install PMAP","PMAP container in NVRAM",done>1?"OK":"pending",done>1?2:0);
  row(-1,"3","Calibrate","PMAP ELECT on CD, DVD-SL and DVD-DL",done>2?"OK":"pending",done>2?2:0);
  row(-1,"4","Finish","Remove the PMAP container or install Force Unlock v2","pending",0);
  Btn bt[3];int n=0;
  if(patch_kind==P_UNLOCK&&pending[0])bt[n++]=(Btn){!strcmp(pending,"restore")?"Continue · restore NVRAM":"Continue · Finish",K_X,!strcmp(pending,"restore")?S_RESTORE_B:S_FINISH_B,0,strcmp(pending,"v2")&&!have_bak};
  else if(patch_kind==P_UNLOCK)bt[n++]=(Btn){"Start Step 2 · Install PMAP",K_X,S_HOOK_CONFIRM,0,0};
  else if(patch_kind==P_PMAP){bt[n++]=(Btn){"Start Step 3 · Calibrate",K_X,S_CALIB_INTRO,0,0};bt[n++]=(Btn){"Skip to Step 4",K_SQ,S_FINISH,1,0};}
  else if(patch_kind!=P_UNKNOWN)bt[n++]=(Btn){"Start Step 1 · Unlock",K_X,S_UNLOCK_CONFIRM,0,0};

  if(have_bak&&memcmp(bak,img0,1024)&&patch_kind!=P_UNKNOWN&&!(patch_kind==P_UNLOCK&&!strcmp(pending,"restore")))bt[n++]=(Btn){"Restore NVRAM",K_TRI,S_RESTORE_CONFIRM,3,0};
  buttons(bt,n);break;}
 case S_UNLOCK_CONFIRM:{eyebrow("STEP 1 OF 4");hero(G_LOCK,0);title("Unlock + NVRAM backup");
  lede("A full NVRAM backup is written to USB first, then Unlock Mechacon.");
  {char m[160];snprintf(m,sizeof m,"The backup %s (1 KB) is on the USB stick and verified. It was saved at the first run and is never overwritten.",bakname);note(0,m);}
  Btn b[]={{"Start",K_X,-1,0,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_HOOK_CONFIRM:{eyebrow("STEP 2 OF 4");hero(G_CHIP,0);title("Install PMAP");
  lede("Writes the PMAP container to NVRAM — this opens the ELECT commands for calibration.");
  Btn b[]={{"Start",K_X,-1,0,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_CALIB_INTRO:{eyebrow("STEP 3 OF 4");hero(G_CD,0);title("Calibration");
  lede("Runs the PMAP ELECT chain over three discs:\nCD, DVD-SL and DVD-DL.");
  note(1,"The write is permanent. If reading gets worse, just restore the backup.");
  Btn b[]={{"Start",K_X,-1,0,0},{"Advanced",K_SQ,S_ADV_MENU,1,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,3);break;}
 case S_INSERT:{int cd=!strcmp(ins_kind,"CD");char t[48];
  eyebrow(cd?"CALIBRATION · CD":!strcmp(ins_kind,"DVD-SL")?"CALIBRATION · DVD-SL":"CALIBRATION · DVD-DL");hero(cd?G_CD:G_DVD,0);
  snprintf(t,sizeof t,"Insert the %s",cd?"CD":!strcmp(ins_kind,"DVD-SL")?"single-layer DVD":"dual-layer DVD");title(t);
  lede(cd?"Open the lid, insert a CD and close the lid.":!strcmp(ins_kind,"DVD-SL")?"Take out the previous disc, insert a single-layer DVD and close the lid.":"Take out the previous disc, insert a dual-layer PS2 DVD and close the lid.");
  {char t2[96];const char *tn=type_name(ins_type);
   snprintf(t2,sizeof t2,"Drive: lid %s · type %02X (%s) · ready %d",ins_lid?"open":"closed",ins_type<0?0:ins_type,tn,ins_ready);
   text(f_mono,14,SX,cur_y,C_MUTED,t2);cur_y+=22;
   if(ins_force)text(f_body,14,SX,cur_y,C_WARN,"Not recognised by the firmware. Continue anyway: PMAP's c16 checks the disc itself.");
   else text(f_mono,14,SX,cur_y,C_FAINT,"waiting for the disc…");}
  {Btn b[2]={{"Cancel",K_O,-1,2,0},{"Continue anyway",K_SQ,-1,1,0}};buttons(b,ins_force?2:1);}break;}
 case S_TASK:{int i;eyebrow(t_eb);title(t_title);
  for(i=0;i<ntrow;i++)row(trow[i].icon,trow[i].ic,trow[i].label,trow[i].sub,trow[i].st==2?"OK":trow[i].st==1?"…":"—",trow[i].st);
  if(t_cap)bar(t_cap,t_right?t_right:"");for(i=0;i<4;i++)act[i]=-1;break;}
 case S_RESULT:{eyebrow("STEP 3 · WRITTEN");title("What changed");legend(cur_y-30);draw_table_fields(0);

  Btn b[]={{"Continue to Step 4",K_X,S_FINISH,0,0},{"Apply values and test a disc",K_SQ,-1,1,0},{"Restore NVRAM",K_TRI,S_RESTORE_CONFIRM,3,0},{"Back",K_O,S_ADV_MENU,2,0}};
  if(!have_bak)b[2]=b[3];buttons(b,have_bak?4:3);break;}
 case S_ADV_MENU:{eyebrow("STEP 3 · ADVANCED");title("Advanced calibration");
  lede("Measure with several discs without writing, then choose the values: one run, the average of several, or your own number.");

  row(-1,"1","Measure only","Full ELECT on one disc CD/DVD-SL/DVD-DL",NULL,sel==0?3:0);
  row(-1,"2","Edit values","Average of runs or manual entry, then review & write",NULL,sel==1?3:0);

  row(-1,"3","Laser check","Measure a disc, nothing is written",NULL,sel==2?3:0);
  Btn b[]={{"Open",K_X,-1,0,0},{"Back",K_O,S_CALIB_INTRO,2,0}};buttons(b,2);break;}
 case S_ADV_MEASURE:{char m[96];eyebrow("ADVANCED · MEASURE ONLY");title("Measure a disc");
  lede("Insert a disc and select its type.");
  snprintf(m,sizeof m,"%d measured runs in %s.",runs_count,csvname);text(f_body,15,SX,cur_y,C_MUTED,m);
  Btn b[]={{"CD",K_X,-1,0,0},{"DVD-SL",K_SQ,-1,1,0},{"DVD-DL",K_TRI,-1,1,0},{"Back",K_O,S_ADV_MENU,2,0}};buttons(b,4);break;}
 case S_ADV_EDIT:{eyebrow("ADVANCED · EDIT VALUES");legend(cur_y-21);cur_y+=2;draw_table_fields(1);
  {int m=F[sel].part=='M';
   Btn b[]={{"Write",K_X,S_ADV_REVIEW,0,0},{"Average",K_SQ,-1,1,m},{"All average",K_TRI,-1,1,0},{"Backup",K_R1,-1,1,m||!have_bak},{"All backup",K_L1,-1,1,!have_bak},{"Back",K_O,S_ADV_MENU,2,0}};buttons(b,6);}break;}
 case S_ADV_REVIEW:{int i,ny=0,nr=0,nc=0;char m[160];
  for(i=0;i<NF;i++)if(F[i].val!=F[i].now){nc++;int s=statusf(&F[i],F[i].val,m,sizeof m);if(s==1)ny++;if(s==2)nr++;}
  eyebrow("ADVANCED · REVIEW");title("Review & write");
  if(!nc)lede("Nothing to write: every value equals NVRAM now. Change values in Edit first.");else draw_table_fields(2);
  Btn b[]={{"Write",K_X,-1,nr?3:0,!nc},{"Back to edit",K_O,S_ADV_EDIT,2,0}};buttons(b,2);
  (void)ny;break;}
 case S_TESTDISC:{char t[120];eyebrow("TEST DISC · NO RESTART");title("Apply values and test a disc");
  lede("The EEPROM values are in use now. Open and close the lid with a disc; it is read as soon as the firmware recognises it.");
  snprintf(t,sizeof t,"Drive: lid %s · type %02X (%s) · ready %d",ins_lid?"open":"closed",ins_type<0?0:ins_type,type_name(ins_type),ins_ready);
  text(f_mono,14,SX,cur_y,C_MUTED,t);cur_y+=26;
  if(td_n){row(td_ok?G_CHECK:G_WARN,0,td_what,td_detail,td_ok?"OK":"FAIL",td_ok?2:0);}
  snprintf(t,sizeof t,"%d discs tried, %d read OK",td_n,td_good);text(f_body,14,SX,cur_y,C_FAINT,t);
  Btn b[]={{"Read again",K_SQ,-1,1,0},{"Edit values",K_TRI,-1,1,0},{"Back",K_O,-1,2,0}};buttons(b,3);break;}
 case S_LASER:{char t[120];int y;eyebrow("LASER CHECK · NO WRITE");title("Laser check");
  snprintf(t,sizeof t,"Drive: lid %s · type %02X (%s) · ready %d",ins_lid?"open":"closed",ins_type<0?0:ins_type,type_name(ins_type),ins_ready);
  text(f_mono,14,SX,cur_y,C_MUTED,t);cur_y+=24;
  if(!lc_n){lede("Insert a pressed disc and close the lid. It is measured as soon as the firmware recognises it; nothing is written.");}
  else{y=cur_y;snprintf(t,sizeof t,"%s · c17 %02X · measurement %d",lc_kind,lc_c17,lc_n);text(f_bold,16,SX,y,C_FG,t);y+=28;
   lc_line(y,"FE gain",lc_fe,lc_fe_ok,lc_fe<0?"—":NULL,"08..60 hex");y+=26;
   lc_line(y,"TE gain",lc_te,lc_te_ok,NULL,"10..60 hex");y+=26;
   lc_line(y,"RF DC",lc_rf,lc_rf_ok,NULL,lc_cd?"49..89 hex":">= 35 hex");y+=26;
   {char jt[24];snprintf(jt,sizeof jt,"%u (%04X)",lc_j[1],lc_j[1]);lc_line(y,"Jitter (3x)",lc_j[1],lc_j_ok,jt,lc_jlim);}y+=26;
   snprintf(t,sizeof t,"jitter runs %u / %u / %u (lower is better)",lc_j[0],lc_j[1],lc_j[2]);text(f_mono,13,SX+140,y,C_FAINT,t);y+=22;
   text(f_body,13,SX,y,C_FAINT,"High gain = weak signal. Prefer the lowest laser power with low jitter and gains mid-range.");
   cur_y=y+20;}
  {int tune=P->walk&&ins_ready==2&&ins_type==0x14&&lc_n&&lc_c17==0x14;
   Btn b[]={{"Live tune",K_X,-1,0,!tune},{"Measure again",K_SQ,-1,1,0},{"Adjust mechanics",K_TRI,-1,1,!(ins_ready==2&&ins_type>=0x10&&ins_type!=0xfd)},{"Back",K_O,-1,2,0}};buttons(b,4);}break;}
 case S_LIVE:{char t[120];int i,gx=SX,gw=CW,gy,gh=150,y;unsigned top=lv_lim*2,v;
  eyebrow("ADJUST MECHANICS · NO WRITE");title("Live jitter");
  snprintf(t,sizeof t,"%s · position %s (LBA %u of %u) · lower is better",lc_kind,lv_posname[lv_pos],lv_lba,lv_size);
  text(f_mono,13,SX,cur_y-8,C_MUTED,t);cur_y+=12;
  y=cur_y;
  if(lv_n){v=lv_hist[(lv_n-1)%LV_N];snprintf(t,sizeof t,"%u",v);{int w2=text(f_disp,40,SX,y,v<=lv_lim?C_OK:C_WARN,t);char h[16];snprintf(h,sizeof h,"%04X",v);text(f_mono,20,SX+w2+10,y+16,C_MUTED,h);}
   snprintf(t,sizeof t,"best %u (%04X)  avg10 %u (%04X)",lv_best,lv_best,lv_avg10,lv_avg10);text(f_mono,13,SX+250,y+6,C_MUTED,t);
   snprintf(t,sizeof t,"PMAP limit %u (%04X)%s",lv_lim,lv_lim,lv_pause?"   PAUSED":"");text(f_mono,13,SX+250,y+26,lv_pause?C_WARN:C_FAINT,t);}
  else text(f_body,16,SX,y+10,C_MUTED,"measuring…");
  gy=y+56;box(gx,gy,gw,gh,8,0x0a111e,C_LINE,1);
  if(top<1000)top=1000;
  {int ly=gy+gh-(int)((unsigned long long)lv_lim*(gh-4)/top);fill(gx+2,ly,gw-4,1,C_WARN);}
  for(i=0;i<LV_N&&i<(int)lv_n;i++){unsigned hv=lv_hist[(lv_n-1-i)%LV_N];int bh=(int)((unsigned long long)(hv>top?top:hv)*(gh-4)/top),bx=gx+gw-4-(i+1)*((gw-6)/LV_N);
   fill(bx,gy+gh-2-bh,(gw-6)/LV_N-1,bh,hv<=lv_lim?C_OK:C_WARN);}
  cur_y=gy+gh+8;
  text(f_body,13,SX,cur_y,C_FAINT,"Turn the screw slowly; wait 2-3 values after each step. Left/right: position.");
  snprintf(foot,sizeof foot,"left/right: position");footc=C_FAINT;
  {Btn b[]={{lv_pause?"Resume":"Pause",K_SQ,-1,1,0},{"Back",K_O,-1,2,0}};buttons(b,2);}break;}
 case S_TUNE:{char t[120];int i,y,x2=SX+318;
  eyebrow("LIVE TUNE · DVD-SL · NOTHING WRITTEN");
  snprintf(t,sizeof t,"PS2 DVD · c17 %02X · reads at LBA 2000..3023 · DSP only",lc_c17);text(f_mono,13,SX,cur_y-2,C_MUTED,t);cur_y+=20;
  y=cur_y;
  for(i=0;i<TN_N;i++){int on=i==tn_sel,yy=y+i*30;char v[24],e[32];
   if(on)box(SX,yy-3,304,28,7,0x10233c,C_ACC,1);
   text(f_body,16,SX+10,yy,on?C_FG:C_MUTED,tn_name[i]);
   snprintf(v,sizeof v,(i==0||i==1)?"%d (%02X)":"%d",tn_val[i],tn_val[i]&0xff);text(f_mono,16,SX+104,yy,tn_set[i]?C_ACC2:C_FG,v);
   snprintf(e,sizeof e,"%s %d",tn_known[i]?"start":"EEPROM",tn_start[i]);textr(f_mono,12,SX+296,yy+3,C_FAINT,e);}
  {unsigned lim=0x3e00;snprintf(t,sizeof t,"%u",tn_j);text(f_disp,34,x2,y-4,tn_n?(tn_j<=lim?C_OK:C_WARN):C_FAINT,tn_n?t:"…");
   snprintf(t,sizeof t,"jitter · best %u · avg10 %u",tn_n?tn_best:0,tn_avg10);text(f_mono,12,x2,y+38,C_MUTED,t);
   snprintf(t,sizeof t,"%u.%02u MB/s",tn_mb100/100,tn_mb100%100);text(f_disp,26,x2,y+62,tn_mb100?C_FG:C_FAINT,t);
   snprintf(t,sizeof t,"%u reads · %d errors",tn_reads,tn_errs);text(f_mono,12,x2,y+96,tn_errs?C_WARN:C_MUTED,t);
   snprintf(t,sizeof t,"reset by firmware: %d",tn_resets);text(f_mono,12,x2,y+116,tn_resets?C_WARN:C_MUTED,t);}
  y+=TN_N*30+8;
  for(i=0;i<4&&i<tn_npt;i++){text(f_mono,12,SX,y,C_FAINT,tn_pt[(tn_npt-1-i)%4]);y+=17;}
  cur_y=y;
  snprintf(foot,sizeof foot,"Up/down: value · left/right: change");footc=C_FAINT;
  {Btn b[]={{"Reset",K_SQ,-1,1,0},{"Use in Edit",K_TRI,-1,1,0},{"Back",K_O,-1,2,0}};buttons(b,3);}break;}
 case S_REGION:{int i,r=region_of(img0),y;eyebrow("STEP 4 · REGION");title("Retail-DEX region");
  y=cur_y;for(i=0;i<=REG_ROWS;i++){char t[80];int on=i==sel;
   if(!i){if(r>=0)snprintf(t,sizeof t,"Keep current region (Retail-DEX · %s)",REGIONS[r].name);else snprintf(t,sizeof t,"Keep current region (factory / CEX)");}else snprintf(t,sizeof t,"Retail-DEX · %s",REGIONS[i-1].name);
   if(on)box(SX,y-2,CW,26,7,0x10233c,C_ACC,1);text(f_body,16,SX+12,y,on?C_FG:C_MUTED,t);y+=27;}
  snprintf(foot,sizeof foot,"Up/down: region");footc=C_FAINT;
  Btn b[]={{"Select",K_X,-1,0,0},{"Back",K_O,S_FINISH,2,0}};buttons(b,2);break;}
 case S_FINISH:{eyebrow("STEP 4 OF 4");hero(G_LOCK,0);title("Finish");
  lede("Replaces the PMAP container. The calibrated values (block 0x30) stay either way. Needs Unlock first, so it takes two power cycles.");

  row(-1,"1","Install Force Unlock v2","read burnt CDs, DVDs and DVD-Video",NULL,sel==0?3:0);
  {char m[96];if(have_bak)snprintf(m,sizeof m,"Restores the original patch from %s",bakname);else snprintf(m,sizeof m,"No backup on USB");
   row(-1,"2","Remove PMAP container",m,NULL,!have_bak?4:sel==1?3:0);}
  Btn b[]={{"Open",K_X,-1,0,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_FINISH_B:{char t[128];eyebrow("STEP 4 · SECOND PART");hero(G_LOCK,0);title("Finish");
  snprintf(t,sizeof t,"Unlock is active. Now writes %s.",finish_target());lede(t);
  Btn b[]={{"Write",K_X,-1,0,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_RESTORE_CONFIRM:{eyebrow("RESTORE FROM BACKUP");hero(G_RESTORE,1);title("Restore the original NVRAM");
  {char m[120];snprintf(m,sizeof m,"Writes back %s — the exact state of NVRAM at the beginning.",bakname);lede(m);}
  note(1,"If the patch differs it needs Unlock first: two power cycles.");
  Btn b[]={{"Yes, restore NVRAM",K_X,-1,3,0},{"Cancel",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_RESTORE_B:{eyebrow("RESTORE · SECOND PART");hero(G_RESTORE,1);title("Restore the original NVRAM");
  {char m[120];snprintf(m,sizeof m,"Unlock is active. Now writes back %s.",bakname);lede(m);}
  Btn b[]={{"Write",K_X,-1,3,0},{"Back",K_O,S_WELCOME,2,0}};buttons(b,2);break;}
 case S_POWER:power_screen();break;
 case S_DONE:{eyebrow("DONE");title("Console is ready");
  lede("All four steps are complete. If something is wrong, you can restore the exact NVRAM from the backup.");
  row(-1,0,"Unlock",NULL,"OK",2);row(-1,0,"Install PMAP",NULL,"OK",2);row(-1,0,"Calibrate",NULL,"OK",2);row(-1,0,"Finish",NULL,"OK",2);
  for(int i=0;i<4;i++)act[i]=-1;break;}
 case S_ERROR:{char m[160];eyebrow("STOPPED");hero(G_WARN,1);title("Stopped");lede(failure);
  snprintf(m,sizeof m,"Save %s and mechapmap_*.bin from the USB flash drive.",logname);note(err_back?0:2,m);
  if(err_back){Btn b[]={{"Back to start",K_O,S_WELCOME,2,0}};buttons(b,1);}else{for(int i=0;i<4;i++)act[i]=-1;}break;}
 }
 padbar(NULL);}

static void refresh(void){int i;
 if(stable_image(img0)){patch_kind=classify(img0);c97_k=p142_word(img0,0x37)&0x3f;for(i=0;i<NF;i++)F[i].now=F[i].val=fget(&F[i],img0);}}
static void startup(void){char buf[24];int n;uint8_t ver[4]={0},date[5]={0};
 header();fill(0,SY+70,W,200,C_BG);text(f_body,18,SX,SY+90,C_MUTED,"Starting…");present();
 if(!start_stack()){char m[64];snprintf(m,sizeof m,"IOP startup failed at %s.",stack_fail);snprintf(failure,sizeof failure,"%s",m);err_back=0;scr=S_ERROR;draw();present();for(;;)gsKit_vsync_wait();}
 PadInitPads();MechaInit();
 if(!pick_usb()){snprintf(failure,sizeof failure,"No USB stick found (mass:/). Nothing written.");err_back=0;scr=S_ERROR;draw();present();for(;;)gsKit_vsync_wait();}
 report("MechaPMAP v1\n");flush();
 if(sceCdInit(SCECdINIT)<0||!verdate()||!getMechaVersion(ver)||!getMechaBuildDate(date)){fail_("This MechaCon is not supported (supported: 5.00, 5.02, 5.04, 5.06, 5.12, 6.00, 6.02, 6.04, 6.06, 6.10, 6.12). Nothing written.",0);return;}
 set_ranges();
 if(!ee_set_sums(P->sums)||ee_len30!=(P->last30-0x30)*2){fail_("Profile checksum table is inconsistent. Nothing written.",0);return;}
 if(!getSerial(&serial)){fail_("Cannot read the serial number. Nothing written.",0);return;}
 snprintf(serial_txt,sizeof serial_txt,"%u",(unsigned)serial);
 snprintf(bakname,sizeof bakname,"pmap_%s_%u.bin",P->name,(unsigned)serial);snprintf(statename,sizeof statename,"pmap_state_%s_%u.txt",P->name,(unsigned)serial);
 snprintf(csvname,sizeof csvname,"pmap_values_%s_%u.csv",P->name,(unsigned)serial);
 report("SERIAL %u\n",(unsigned)serial);
 if(!stable_image(img0)){fail_("EEPROM read failed. Nothing written.",0);return;}
 patch_kind=classify(img0);report("NVRAM patch=%s sums=%s\n",patch_name,p142_all_sums_ok(img0)?"ok":"BAD");
 n=read_file(bakname,bak,1024);
 if(n==1024&&bak_valid(bak))have_bak=1;
 else if(n==-1){if(patch_kind!=P_UNKNOWN&&p142_all_sums_ok(img0)&&write_file(bakname,img0,1024)){memcpy(bak,img0,1024);have_bak=1;report("BACKUP created %s\n",bakname);}
  else report("BACKUP not created\n");}
 else{fail_("The backup on USB is damaged or not 1 KB. Nothing written; keep the file.",0);return;}
 classify(img0);
 n=read_file(statename,buf,sizeof buf-1);if(n>0){buf[n]=0;if(pending_valid(buf))strcpy(pending,buf);}
 if(pending[0]&&patch_kind!=P_UNLOCK&&patch_kind!=P_PMAP){report("PENDING %s dropped (patch=%s)\n",pending,patch_name);set_pending("");}
 if(patch_kind==P_PMAP&&pending[0])set_pending("");
 refresh();runs_load();report("RUNS %d\n",runs_count);flush();
 scr=S_WELCOME;draw();present();}

static void derive_mid(void){int m;if(F[1].val==F[1].now&&F[2].val==F[2].now){F[0].val=F[0].now;F[3].val=F[3].now;return;}
 m=c97_mid(F[1].val,F[2].val);F[0].val=clampf(&F[0],m);F[3].val=clampf(&F[3],m-1);}
static int handle(u32 p){static const u32 keys[4]={PAD_CROSS,PAD_CIRCLE,PAD_TRIANGLE,PAD_SQUARE};int k,i;
 if(scr==S_ADV_EDIT){
  if(p&PAD_DOWN){sel=sel<NF-1?sel+1:sel;return 1;}if(p&PAD_UP){sel=sel?sel-1:0;return 1;}
  if(F[sel].part=='M'&&(p&(PAD_RIGHT|PAD_LEFT|PAD_SQUARE|PAD_R1)))return 1;
  if(p&PAD_RIGHT){F[sel].val=clampf(&F[sel],F[sel].val+pad_step);derive_mid();return 1;}if(p&PAD_LEFT){F[sel].val=clampf(&F[sel],F[sel].val-pad_step);derive_mid();return 1;}
  if(p&PAD_SQUARE){F[sel].val=avgf(&F[sel]);derive_mid();return 1;}
  if(p&PAD_TRIANGLE){for(i=0;i<NF;i++)if(F[i].part!='M')F[i].val=avgf(&F[i]);derive_mid();return 1;}
  if(have_bak&&(p&PAD_R1)){F[sel].val=fget(&F[sel],bak);derive_mid();return 1;}
  if(have_bak&&(p&PAD_L1)){for(i=0;i<NF;i++)F[i].val=fget(&F[i],bak);return 1;}}
 if(scr==S_ADV_EDIT)derive_mid();
 if(scr==S_RESULT||scr==S_ADV_REVIEW){if(p&PAD_DOWN){sel++;return 1;}if(p&PAD_UP){sel=sel?sel-1:0;return 1;}}

 if(scr==S_ADV_MENU){if(p&PAD_DOWN){sel=sel<2?sel+1:sel;return 1;}if(p&PAD_UP){sel=sel?sel-1:0;return 1;}}
 if(scr==S_FINISH){if(p&PAD_DOWN){sel=have_bak?1:0;return 1;}if(p&PAD_UP){sel=0;return 1;}}
 if(scr==S_REGION){if(p&PAD_DOWN){sel=sel<REG_ROWS?sel+1:sel;return 1;}if(p&PAD_UP){sel=sel?sel-1:0;return 1;}}
 for(k=0;k<4;k++)if((p&keys[k])&&!act_dis[k]){

  if(act[k]==-1){
   if(scr==S_UNLOCK_CONFIRM&&k==K_X){if(op_unlock("before_unlock")){refresh();power("STEP 1 · DONE","Power off now","Unlock is written and verified.","COLD: a full unplug from the wall, not just Reset.");}return 1;}
   if(scr==S_HOOK_CONFIRM&&k==K_X){if(op_patch(P->hook,"PMAP hook","before_pmap")){refresh();power("STEP 2 · DONE","Restart required","PMAP is written and verified. A cold start loads the container.","COLD: a full unplug from the wall. The wizard continues to Step 3.");}return 1;}
   if(scr==S_CALIB_INTRO&&k==K_X){op_calibrate();return 1;}
   if(scr==S_ADV_MENU&&k==K_X&&sel==0){scr=S_ADV_MEASURE;return 1;}
   if(scr==S_ADV_MENU&&k==K_X&&sel==1){scr=S_ADV_EDIT;sel=0;scroll_=0;return 1;}
   if(scr==S_ADV_MENU&&k==K_X&&sel==2){refresh();if(classify(img0)!=P_PMAP){fail_("PMAP is not in NVRAM: Laser check needs it. Nothing written.",1);return 1;}
    if(!ensure_window()){fail_("Window upload/check failed. Nothing written.",1);return 1;}laser_check();if(tn_edit){int i;for(i=0;i<TN_N;i++)F[tn_field[i]].val=clampf(&F[tn_field[i]],tn_ee[i]);scr=S_ADV_EDIT;sel=tn_field[0];scroll_=0;return 1;}
    if(scr==S_LASER||scr==S_LIVE){scr=S_ADV_MENU;sel=2;}return 1;}
   if(scr==S_ADV_MEASURE){const char *kk=k==K_X?"CD":k==K_SQ?"DVD-SL":k==K_TRI?"DVD-DL":NULL;
    if(kk){int r=elect_disc(kk,0);if(r>=0){refresh();scr=S_ADV_MEASURE;}return 1;}}
   if(scr==S_RESULT&&k==K_SQ){refresh();if(op_apply()){test_disc();

     if(scr==S_TESTDISC){if(td_edit){refresh();scr=S_ADV_EDIT;sel=0;scroll_=0;}else scr=S_RESULT;}}return 1;}
   if(scr==S_ADV_EDIT&&k==K_TRI){for(i=0;i<NF;i++)F[i].val=avgf(&F[i]);return 1;}
   if(scr==S_ADV_REVIEW&&k==K_X){op_edit_write();return 1;}
   if(scr==S_FINISH&&k==K_X&&sel==0){refresh();int r=region_of(img0);scr=S_REGION;sel=r>=0||!REG_ROWS?0:REG_EUROPE+1;return 1;}
   if(scr==S_FINISH&&k==K_X&&sel==1&&!have_bak)return 1;
   if((scr==S_FINISH&&k==K_X&&sel==1)||(scr==S_REGION&&k==K_X)){char t[16];

    if(scr==S_REGION){if(sel&&sel-1!=region_of(img0))snprintf(t,sizeof t,"v2:%d",sel-1);else strcpy(t,"v2");}else strcpy(t,"patch");refresh();
    if(patch_kind==P_UNLOCK){if(!set_pending(t)){fail_("Cannot write the state file on USB. Nothing written.",1);return 1;}scr=S_FINISH_B;return 1;}
    if(!set_pending(t)){fail_("Cannot write the state file on USB. Nothing written.",1);return 1;}
    if(op_unlock("before_finish_unlock")){refresh();power("STEP 4 · FIRST PART","Power off now","Unlock is written and verified.","COLD: a full unplug from the wall. At the next start the wizard writes the patch.");}
    else set_pending("");return 1;}
   if(scr==S_FINISH_B&&k==K_X){int ok,v2=!strncmp(pending,"v2",2),r=pending_region();if(!v2&&!have_bak){fail_("The backup is missing on USB. Nothing written.",1);return 1;}
    if(r>=0&&!op_region(r))return 1;
    ok=v2?op_patch(P->fuv2,"Force Unlock v2","before_finish"):op_patch(bak+PATCH,"NVRAM patch from the backup","before_finish");
    if(ok){set_pending("");refresh();power("STEP 4 · DONE","Power off now","The patch is written and verified.","COLD: a full unplug from the wall, not just Reset. The patch loads at the next power-on.");}return 1;}
   if(scr==S_RESTORE_CONFIRM&&k==K_X){
    refresh();
    if(!memcmp(img0+PATCH,bak+PATCH,224)||patch_kind==P_UNLOCK){if(op_restore_full()){refresh();power("RESTORED","Power off now","NVRAM matches the backup byte for byte.","COLD: a full unplug from the wall, not just Reset.");}return 1;}
    if(!set_pending("restore")){fail_("Cannot write the state file on USB. Nothing written.",1);return 1;}
    if(op_unlock("before_restore_unlock")){refresh();power("RESTORE · FIRST PART","Power off now","Unlock is written and verified.","COLD: a full unplug from the wall. At the next start the wizard writes the backup.");}
    else set_pending("");return 1;}
   if(scr==S_RESTORE_B&&k==K_X){if(op_restore_full()){set_pending("");refresh();power("RESTORED","Power off now","NVRAM matches the backup byte for byte.","COLD: a full unplug from the wall, not just Reset.");}return 1;}
   continue;}
  if(act[k]>=0){if(act[k]==S_WELCOME)refresh();scr=act[k];sel=0;scroll_=0;return 1;}}
 return 0;}
