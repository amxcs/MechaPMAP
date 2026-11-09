static int fget(const Field *f,const unsigned char *img){
 unsigned w=p142_word(img,f->word);int v=f->part=='L'?(int)(w&255):f->part=='H'?(int)(w>>8):(int)w;
 if(f->k=='s'){int m=1<<f->bits;v&=m-1;if(v&(m>>1))v-=m;}
 return v;}
static unsigned fput(const Field *f,unsigned w,int v){
 if(f->part=='L')return (w&0xff00u)|((unsigned)v&255u);
 if(f->part=='H')return (w&0x00ffu)|(((unsigned)v&255u)<<8);
 if(f->k=='s'){unsigned m=(1u<<f->bits)-1;return (w&~m)|((unsigned)v&m);}
 return (unsigned)v&0xffffu;}

static int kind_word(const char *kind,unsigned w){
 if(w==0x30)return 1;
 if(!strcmp(kind,"CD"))return w==0x33||w==0x34||(w>=0x40&&w<=0x47);
 if(!strcmp(kind,"DVD-SL"))return w==0x33||w==0x35||w==0x36||(w>=0x48&&w<=0x4f);
 return w>=0x50&&w<=(ee_last30<0x59?ee_last30:0x59);}
static int only_kind_changes(const char *kind,const unsigned char *a,const unsigned char *b){unsigned i;
 for(i=0;i<512;i++)if(p142_word(a,i)!=p142_word(b,i)&&!kind_word(kind,i))return 0;return 1;}

static int c97_k=51;
static int c97_mid(int v34,int v35){int ucd=(2*v34+1)/3,udvd=2*v35+1;
 int cd=ucd*(10000-(100-c97_k)*50)/10/1000,dvd=udvd*(100+c97_k)/100;return (cd+dvd)/2;}
static int c97_gate(int v34,int v35){int q=v35?(v34*10)/(v35*3):0;return q>=15&&q<=29;}
static int clampf(const Field *f,int v){if(f->k=='s'){int m=1<<(f->bits-1);return v<-m?-m:v>m-1?m-1:v;}int m=(1<<f->bits)-1;return v<0?0:v>m?m:v;}
static int avgf(const Field *f){int i,s=0;if(!f->nruns)return f->now;for(i=0;i<f->nruns;i++)s+=f->runs[i];return clampf(f,(int)lroundf((float)s/f->nruns));}
static void fmtv(const Field *f,int v,char *o,int n){
 if(f->k=='h')snprintf(o,n,"%02X",v);else if(f->k=='s')snprintf(o,n,"%s%d",v>0?"+":v<0?"−":"",v<0?-v:v);else snprintf(o,n,"%d",v);}
static void fmtd(const Field *f,int a,int b,char *o,int n){int d=b-a,m=d<0?-d:d;
 if(!d)snprintf(o,n,"(0)");else if(f->k=='h')snprintf(o,n,"(%s%X)",d>0?"+":"−",m);else snprintf(o,n,"(%s%d)",d>0?"+":"−",m);}

static int statusf(const Field *f,int v,char *msg,int n){int i,lo=f->now,hi=f->now,b,d;char a1[16],a2[16];msg[0]=0;
 for(i=0;i<f->nruns;i++){if(f->runs[i]<lo)lo=f->runs[i];if(f->runs[i]>hi)hi=f->runs[i];}
 if(v>=lo&&v<=hi)return 0;
 b=v<lo?lo:hi;d=v<lo?lo-v:v-hi;{int base=b<0?-b:b;if(base<16)base=16;
 float p=d*100.f/base;if(p<=10)return 0;
 fmtv(f,lo,a1,sizeof a1);fmtv(f,hi,a2,sizeof a2);
 snprintf(msg,n,"%d %% outside measured %s..%s",(int)(p+.5f),a1,a2);
 return p>50?2:1;}}
static u32 stc(int s){return s==2?C_CRIT:C_WARN;}

static int runs_count;
static void runs_add(const char *kind,const unsigned char *img){runs_count++;int i;char line[512];int n;
 n=snprintf(line,sizeof line,"%s",kind);
 for(i=0;i<NF;i++){Field *f=&F[i];if(kind_word(kind,f->word)&&f->word!=0x30){int v=fget(f,img);
   if(f->nruns<16&&f->part!='M')f->runs[f->nruns++]=v;n+=snprintf(line+n,sizeof line-n,",%d",v);}else n+=snprintf(line+n,sizeof line-n,",-");}
 n+=snprintf(line+n,sizeof line-n,"\n");
 {char p[96];int fd;snprintf(p,sizeof p,"%s%s",device,csvname);fd=open(p,O_WRONLY|O_CREAT|O_APPEND,0666);
  if(fd>=0){if(write(fd,line,n)!=n)report("CSV_WRITE_SHORT\n");close(fd);}else report("CSV_OPEN_FAIL\n");}
 report("RUN %s",line);}

static void runs_add_thr(int v33,int v34){char line[256];int n,i,fd;char p[96];runs_count++;
 n=snprintf(line,sizeof line,"CD,%d,%d",v33,v34);for(i=2;i<NF;i++)n+=snprintf(line+n,sizeof line-n,",-");n+=snprintf(line+n,sizeof line-n,"\n");
 if(F[1].nruns<16)F[1].runs[F[1].nruns++]=v34;
 snprintf(p,sizeof p,"%s%s",device,csvname);fd=open(p,O_WRONLY|O_CREAT|O_APPEND,0666);
 if(fd>=0){if(write(fd,line,n)!=n)report("CSV_WRITE_SHORT\n");close(fd);}else report("CSV_OPEN_FAIL\n");
 report("RUN_THRESHOLDS_ONLY %s",line);}
static void runs_load(void){static char buf[16384];int n=read_file(csvname,buf,sizeof buf-1),i;char *line,*save=NULL;
 runs_count=0;if(n<=0)return;buf[n]=0;
 for(line=strtok_r(buf,"\n",&save);line;line=strtok_r(NULL,"\n",&save)){char *tok,*s2=NULL;int col=0;
  for(tok=strtok_r(line,",",&s2);tok;tok=strtok_r(NULL,",",&s2),col++){
   if(col==0)continue;i=col-1;if(i>=NF)break;
   if((tok[0]!='-'||tok[1])&&F[i].part!='M'){int v=atoi(tok);if(F[i].word==0x49&&v>255)v-=512;if(F[i].nruns<16)F[i].runs[F[i].nruns++]=v;}}
  runs_count++;}}

static void vlabel(int x,int ytop,int ybot,u32 c,const char *s){
 int size=11,tw,asc;
 while((tw=text_(f_disp,size,0,0,0,s,1,0))>ybot-ytop-6&&size>8)size--;

 asc=asc_cache(f_disp,size);
 int pen=0,yc=(ytop+ybot)/2+tw/2;const char *p=s;
 while(*p){unsigned ch=utf8(&p);const Glyph *g=glyph_get(f_disp,size,ch);if(!g)continue;
  if(g->bm){int i,j;for(j=0;j<g->h;j++)for(i=0;i<g->w;i++){int a=g->bm[j*g->w+i];
    if(a)blend(x+asc-g->top+j,yc-(pen+g->left+i),c,a);}}
  pen+=g->adv+1;}}
static char foot[160];static u32 footc;
static int res_before[NF],res_after[NF];
static void draw_table_fields(int mode){
 int top=cur_y,bottom=stage_bot-40,rh=23,y,i,n=0,idx[64],gy=0;const char *gname=NULL;char a[24],b[24],d[24],m[160];
 const int XW=SX+24,XF=SX+102,XN=SX+272;
 for(i=0;i<NF;i++){int bv=mode==0?res_before[i]:F[i].now,v=mode==0?res_after[i]:F[i].val;if(mode==1||v!=bv)idx[n++]=i;}
 box(SX,top,CW,bottom-top,9,0x0a111e,C_LINE,1);
 fill(SX+1,top+1,CW-2,24,C_PANEL);
 label(XW,top+6,11,C_MUTED,"WORD");label(XF,top+6,11,C_MUTED,"FIELD");
 label(XN,top+6,11,C_MUTED,mode==1?"NOW":"BEFORE → NEW");if(mode==1){label(XN+60,top+6,11,C_MUTED,"AVG");label(XN+130,top+6,11,C_MUTED,"NEW");}
 if(!n){text(f_body,15,XW,top+34,C_MUTED,"No value changed.");cur_y=bottom+8;return;}
 int rows=(bottom-top-30)/rh;if(mode==1){if(sel<scroll_)scroll_=sel;if(sel>=scroll_+rows-1)scroll_=sel-rows+2;if(scroll_<0)scroll_=0;}
 else{int mx=n>rows-1?n-(rows-1):0;
  if(sel>mx)sel=mx;scroll_=sel;}
 y=top+28;
 for(i=scroll_;i<n&&y+rh<=bottom;i++){Field *f=&F[idx[i]];int bv=mode==0?res_before[idx[i]]:f->now,v=mode==0?res_after[idx[i]]:f->val;int s=statusf(f,v,m,sizeof m);
  if(!gname||strcmp(gname,f->g)){if(gname){if(y-gy<2*rh&&gy+2*rh<=bottom)y=gy+2*rh;vlabel(SX+3,gy,y,C_ACC2,gname);fill(SX+1,y-1,CW-2,1,C_ACCDIM);}gname=f->g;gy=y;}
  if(s)fill(SX+20,y,CW-21,rh-1,s==2?0x1d0d11:0x1a1509);
  if(mode==1&&i==sel){box(SX+20,y,CW-22,rh-1,6,0x10233c,C_ACC,1);}
  text(f_mono,14,XW,y+3,C_FG,f->w);text(f_body,15,XF,y+2,C_FG,f->name);
  fmtv(f,bv,a,sizeof a);fmtv(f,v,b,sizeof b);fmtd(f,bv,v,d,sizeof d);
  if(mode==1){char av[24];fmtv(f,avgf(f),av,sizeof av);
   text(f_mono,14,XN,y+3,C_MUTED,a);text(f_mono,14,XN+60,y+3,C_MUTED,f->nruns?av:"—");
   text(f_mono,14,XN+118,y+3,i==sel?C_ACC2:C_MUTED,"−");text(f_mono,15,XN+136,y+2,s?stc(s):C_FG,b);text(f_mono,14,XN+184,y+3,i==sel?C_ACC2:C_MUTED,"+");
   if(v!=bv)text(f_mono,14,XN+202,y+3,s?stc(s):C_MUTED,d);}
  else{int x=XN;x+=text(f_mono,14,x,y+3,C_MUTED,a);x+=text(f_mono,14,x,y+3,C_MUTED," → ");x+=text(f_mono,15,x,y+2,s?stc(s):C_FG,b);text(f_mono,14,x+6,y+3,s?stc(s):C_MUTED,d);}
  y+=rh;}

 if(gname){if(y-gy<2*rh&&gy+2*rh<=bottom)y=gy+2*rh;vlabel(SX+3,gy,y,C_ACC2,gname);}
 if(mode==1){Field *f=&F[idx[sel]];int s=statusf(f,f->val,m,sizeof m);
  if(s){}
  else if(f->part=='M'){snprintf(foot,sizeof foot,"set by c97 03 from 0x34 / 0x35");footc=C_FAINT;}
  else if((f->word==0x34||f->word==0x35)&&!c97_gate(F[1].val,F[2].val)){snprintf(foot,sizeof foot,"0x34·10/(0x35·3) outside 15..29: c97 03 would not write");footc=0xffd28a;}
  }
 else if(n>rows-1){snprintf(foot,sizeof foot,"Up/down scroll");footc=C_FAINT;}
 cur_y=bottom+8;}
static void legend(int y){
 const char *t1="10 % out of range",*t2="50 % out of range";
 int x=SX+CW-(14+textw(f_body,13,t1)+18+14+textw(f_body,13,t2));
 rrect(x,y+5,9,9,4,C_WARN);x+=14;x+=text(f_body,13,x,y,C_MUTED,t1)+18;
 rrect(x,y+5,9,9,4,C_CRIT);x+=14;text(f_body,13,x,y,C_MUTED,t2);}
