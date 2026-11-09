#define C_BG     0x060a12
#define C_SCREEN 0x0b1322
#define C_PANEL  0x111a2b
#define C_ROW    0x0d1627
#define C_HDR    0x10192e
#define C_LINE   0x24344d
#define C_FG     0xe9f0fa
#define C_MUTED  0x8ba2c2
#define C_FAINT  0x5c738f
#define C_ACC    0x3fa9ff
#define C_ACC2   0x6fe0ff
#define C_ACCDIM 0x1f5d93
#define C_OK     0x4bd896
#define C_WARN   0xffb648
#define C_CRIT   0xff5f6e
#define C_PRIMTX 0x061320

#define W 640
static int H;
static u32 *fb;
static GSGLOBAL *gs;
static GSTEXTURE tex;
static char romver[16];

static inline u32 px(u32 c){return (c>>16&255)|(c&0xff00)|((c&255)<<16)|0x80000000u;}
static inline void blend(int x,int y,u32 c,int a){
 if(x<0||y<0||x>=W||y>=H||a<=0)return;
 if(a>=255){fb[y*W+x]=px(c);return;}
 u32 d=fb[y*W+x];int dr=d&255,dg=d>>8&255,db=d>>16&255;
 int r=c>>16&255,g=c>>8&255,b=c&255;
 dr+=(r-dr)*a/255;dg+=(g-dg)*a/255;db+=(b-db)*a/255;
 fb[y*W+x]=dr|dg<<8|db<<16|0x80000000u;}
static void fill(int x,int y,int w,int h,u32 c){
 int i,j;u32 p=px(c);
 if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>W)w=W-x; if(y+h>H)h=H-y;
 for(j=0;j<h;j++)for(i=0;i<w;i++)fb[(y+j)*W+x+i]=p;}
static u32 mix(u32 a,u32 b,int t){
 int r=(a>>16&255)+((int)(b>>16&255)-(int)(a>>16&255))*t/255;
 int g=(a>>8&255)+((int)(b>>8&255)-(int)(a>>8&255))*t/255;
 int bl=(a&255)+((int)(b&255)-(int)(a&255))*t/255;
 return (u32)(r<<16|g<<8|bl);}
static void vgrad(int x,int y,int w,int h,u32 c1,u32 c2){int j;for(j=0;j<h;j++)fill(x,y+j,w,1,mix(c1,c2,h>1?j*255/(h-1):0));}

static void rrect(int x,int y,int w,int h,int r,u32 c){
 int i,j;
 for(j=0;j<h;j++)for(i=0;i<w;i++){
  float cx=-1,cy=-1;
  if(i<r&&j<r){cx=r;cy=r;}else if(i>=w-r&&j<r){cx=w-r;cy=r;}
  else if(i<r&&j>=h-r){cx=r;cy=h-r;}else if(i>=w-r&&j>=h-r){cx=w-r;cy=h-r;}
  if(cx<0){blend(x+i,y+j,c,255);continue;}
  float dx=i+.5f-cx,dy=j+.5f-cy,d=sqrtf(dx*dx+dy*dy);
  float cov=r-d+.5f; if(cov>1)cov=1; if(cov>0)blend(x+i,y+j,c,(int)(cov*255));}}
static void box(int x,int y,int w,int h,int r,u32 fillc,u32 border,int bw){
 if(bw){rrect(x,y,w,h,r,border);rrect(x+bw,y+bw,w-2*bw,h-2*bw,r>bw?r-bw:0,fillc);}else rrect(x,y,w,h,r,fillc);}

static float segd(float px_,float py,float ax,float ay,float bx,float by){
 float vx=bx-ax,vy=by-ay,wx=px_-ax,wy=py-ay,t=(vx*wx+vy*wy)/(vx*vx+vy*vy);
 if(t<0)t=0; if(t>1)t=1; float dx=wx-t*vx,dy=wy-t*vy; return sqrtf(dx*dx+dy*dy);}
typedef struct{float ax,ay,bx,by;}Seg;
static void strokes(int x0,int y0,int sz,const Seg *s,int n,float ring_r,float lw,u32 c){
 int i,j,k;
 for(j=-1;j<=sz;j++)for(i=-1;i<=sz;i++){
  float p=i+.5f,q=j+.5f,d=1e9f;
  for(k=0;k<n;k++){float e=segd(p,q,s[k].ax*sz,s[k].ay*sz,s[k].bx*sz,s[k].by*sz);if(e<d)d=e;}
  if(ring_r>0){float dx=p-sz/2.f,dy=q-sz/2.f,e=fabsf(sqrtf(dx*dx+dy*dy)-ring_r*sz);if(e<d)d=e;}
  float cov=lw/2-d+.5f; if(cov>1)cov=1; if(cov>0)blend(x0+i,y0+j,c,(int)(cov*255));}}
enum{G_CROSS,G_CIRCLE,G_TRI,G_SQUARE,G_CHECK,G_DISC,G_WARN,G_LOCK,G_POWER,G_USB,G_CHIP,G_RESTORE,G_CD,G_DVD};

static u32 hue(float h){float r=fabsf(h*6-3)-1,g=2-fabsf(h*6-2),b=2-fabsf(h*6-4);
 r=r<0?0:r>1?1:r;g=g<0?0:g>1?1:g;b=b<0?0:b>1?1:b;return (u32)((int)(r*255)<<16|(int)(g*255)<<8|(int)(b*255));}
static void disc_logo(int x0,int y0,int sz){
 int i,j;float R=sz*.48f,hub=sz*.17f,ring=sz*.21f,hole=sz*.075f,c=sz/2.f;
 for(j=0;j<sz;j++)for(i=0;i<sz;i++){float dx=i+.5f-c,dy=j+.5f-c,d=sqrtf(dx*dx+dy*dy);
  float cov=R-d+.5f;if(cov<=0)continue;if(cov>1)cov=1;
  if(d<hole){float e=d-hole+.5f;if(e<=0)continue;cov=e>1?1:e;}
  float a=atan2f(dy,dx);u32 col;
  if(d<hub)col=0xdfe6ef;
  else if(d<ring)col=0x9aa8ba;
  else{float t=.5f+.5f*cosf(2.f*(a+.7f));
   u32 rb=hue(fmodf((a+3.14159f)/6.28318f*2.f+d/R*.35f,1.f));
   col=mix(0x7d8ea6,0xf2f6fb,(int)(t*t*230));
   col=mix(col,rb,(int)(70+90*(1-t)));
   if(d>R-1.6f)col=mix(col,0x2a3a55,150);}
  blend(x0+i,y0+j,col,(int)(cov*255));}}
static void disc_icon(int x0,int y0,int sz,u32 base,u32 lite){
 int i,j;float R=sz*.47f,hub=sz*.15f,hole=sz*.07f,c=sz/2.f;
 for(j=0;j<sz;j++)for(i=0;i<sz;i++){float dx=i+.5f-c,dy=j+.5f-c,d=sqrtf(dx*dx+dy*dy);
  float cov=R-d+.5f;if(cov<=0)continue;if(cov>1)cov=1;
  u32 col;
  if(d<hole)continue;
  else if(d<hub)col=0xc8d2e0;
  else{float a=atan2f(dy,dx),t=.5f+.5f*cosf(2.f*(a+.6f));col=mix(base,lite,(int)(t*t*200));if(d>R-1.5f)col=mix(col,0x000000,90);}
  blend(x0+i,y0+j,col,(int)(cov*255));}}
static void glyph(int g,int x,int y,int sz,u32 c){
 static const Seg cross[]={{.22f,.22f,.78f,.78f},{.78f,.22f,.22f,.78f}};
 static const Seg tri[]={{.5f,.16f,.86f,.80f},{.86f,.80f,.14f,.80f},{.14f,.80f,.5f,.16f}};
 static const Seg sq[]={{.2f,.2f,.8f,.2f},{.8f,.2f,.8f,.8f},{.8f,.8f,.2f,.8f},{.2f,.8f,.2f,.2f}};
 static const Seg chk[]={{.18f,.52f,.40f,.74f},{.40f,.74f,.84f,.28f}};
 static const Seg warn[]={{.5f,.12f,.92f,.86f},{.92f,.86f,.08f,.86f},{.08f,.86f,.5f,.12f},{.5f,.40f,.5f,.60f},{.5f,.72f,.5f,.73f}};

 static const Seg lock[]={{.2f,.48f,.8f,.48f},{.8f,.48f,.8f,.88f},{.8f,.88f,.2f,.88f},{.2f,.88f,.2f,.48f},{.35f,.48f,.35f,.30f},
  {0.350f,0.300f,0.361f,0.243f},{0.361f,0.243f,0.394f,0.194f},{0.394f,0.194f,0.443f,0.161f},{0.443f,0.161f,0.500f,0.150f},{0.500f,0.150f,0.557f,0.161f},{0.557f,0.161f,0.606f,0.194f},{0.606f,0.194f,0.639f,0.243f},{0.639f,0.243f,0.650f,0.300f},
  {.65f,.30f,.65f,.36f},{.5f,.62f,.5f,.74f}};
 static const Seg pwr[]={{.5f,.10f,.5f,.46f}};
 static const Seg usb[]={{.5f,.10f,.5f,.80f},{.34f,.26f,.5f,.10f},{.5f,.10f,.66f,.26f},{.5f,.56f,.28f,.42f},{.5f,.62f,.72f,.48f}};
 static const Seg chip[]={{.26f,.26f,.74f,.26f},{.74f,.26f,.74f,.74f},{.74f,.74f,.26f,.74f},{.26f,.74f,.26f,.26f},{.38f,.1f,.38f,.26f},{.62f,.1f,.62f,.26f},{.38f,.74f,.38f,.9f},{.62f,.74f,.62f,.9f},{.1f,.38f,.26f,.38f},{.1f,.62f,.26f,.62f},{.74f,.38f,.9f,.38f},{.74f,.62f,.9f,.62f}};
 static const Seg rst[]={{.14f,.20f,.14f,.42f},{.14f,.42f,.36f,.42f}};
 float lw=sz>=30?sz*.09f:2.2f;
 switch(g){
 case G_CROSS:strokes(x,y,sz,cross,2,0,lw,c);break;
 case G_CIRCLE:strokes(x,y,sz,NULL,0,.31f,lw,c);break;
 case G_TRI:strokes(x,y,sz,tri,3,0,lw,c);break;
 case G_SQUARE:strokes(x,y,sz,sq,4,0,lw,c);break;
 case G_CHECK:strokes(x,y,sz,chk,2,0,lw*1.2f,c);break;
 case G_DISC:strokes(x,y,sz,NULL,0,.40f,lw,c);strokes(x,y,sz,NULL,0,.10f,lw,c);break;
 case G_WARN:strokes(x,y,sz,warn,5,0,lw,c);break;
 case G_LOCK:strokes(x,y,sz,lock,15,0,lw,c);break;
 case G_POWER:strokes(x,y,sz,pwr,1,.34f,lw,c);break;
 case G_USB:strokes(x,y,sz,usb,5,0,lw,c);break;
 case G_CHIP:strokes(x,y,sz,chip,12,0,lw,c);break;
 case G_RESTORE:strokes(x,y,sz,rst,2,.36f,lw,c);break;
 case G_CD:disc_icon(x,y,sz,0x2f6fd0,0xa8d4ff);break;
 case G_DVD:disc_icon(x,y,sz,0xc8901a,0xffe58a);break;}}

static FT_Library ftl;
static FT_Face f_disp,f_body,f_bold,f_mono;
static unsigned utf8(const char **s){
 const unsigned char *p=(const unsigned char*)*s;unsigned c=*p++;
 if(c>=0xf0){c=(c&7)<<18|(p[0]&63)<<12|(p[1]&63)<<6|(p[2]&63);p+=3;}
 else if(c>=0xe0){c=(c&15)<<12|(p[0]&63)<<6|(p[1]&63);p+=2;}
 else if(c>=0xc0){c=(c&31)<<6|(p[0]&63);p+=1;}
 *s=(const char*)p;return c;}

typedef struct{FT_Face f;short size;unsigned ch;short left,top,w,h,adv;unsigned char *bm;}Glyph;
#define GC 2048
static Glyph gcache[GC];
static int asc_cache(FT_Face f,int size){FT_Set_Pixel_Sizes(f,0,size);return (int)(f->size->metrics.ascender>>6);}
static const Glyph *glyph_get(FT_Face f,int size,unsigned ch){
 unsigned h=((unsigned)(size_t)f*31u+(unsigned)size*131u+ch*2654435761u)%GC,k;
 for(k=0;k<GC;k++){Glyph *e=&gcache[(h+k)%GC];
  if(e->f==f&&e->size==size&&e->ch==ch)return e;
  if(!e->f){FT_Set_Pixel_Sizes(f,0,size);
   if(FT_Load_Char(f,ch,FT_LOAD_RENDER))return NULL;
   FT_GlyphSlot g=f->glyph;FT_Bitmap *b=&g->bitmap;int j;
   e->f=f;e->size=(short)size;e->ch=ch;e->left=(short)g->bitmap_left;e->top=(short)g->bitmap_top;
   e->w=(short)b->width;e->h=(short)b->rows;e->adv=(short)(g->advance.x>>6);
   e->bm=(e->w>0&&e->h>0)?malloc(e->w*e->h):NULL;
   if(e->bm)for(j=0;j<e->h;j++)memcpy(e->bm+j*e->w,b->buffer+j*b->pitch,e->w);
   return e;}}
 return NULL;}

static int text_(FT_Face f,int size,int x,int y,u32 c,const char *s,int track,int draw){
 int x0=x,asc=draw?asc_cache(f,size):0;
 while(*s){unsigned ch=utf8(&s);const Glyph *g=glyph_get(f,size,ch);if(!g)continue;
  if(draw&&g->bm){int i,j;
   for(j=0;j<g->h;j++)for(i=0;i<g->w;i++){int a=g->bm[j*g->w+i];if(a)blend(x+g->left+i,y+asc-g->top+j,c,a);}}
  x+=g->adv+track;}
 return x-x0;}
static int text(FT_Face f,int size,int x,int y,u32 c,const char *s){return text_(f,size,x,y,c,s,0,1);}
static int textw(FT_Face f,int size,const char *s){return text_(f,size,0,0,0,s,0,0);}
static int textr(FT_Face f,int size,int xr,int y,u32 c,const char *s){int w=textw(f,size,s);text(f,size,xr-w,y,c,s);return w;}

static void label(int x,int y,int size,u32 c,const char *s){text_(f_disp,size,x,y,c,s,2,1);}

static int para(FT_Face f,int size,int lh,int x,int y,int w,u32 c,const char *s){
 char line[256],word[96];int n=0,used=0;line[0]=0;
 while(*s){int k=0;while(*s==' ')s++;while(*s&&*s!=' '&&*s!='\n'&&k<95)word[k++]=*s++;word[k]=0;
  char trial[256];snprintf(trial,sizeof trial,"%s%s%s",line,n?" ":"",word);
  if(n&&textw(f,size,trial)>w){text(f,size,x,y+used,c,line);used+=lh;snprintf(line,sizeof line,"%s",word);n=1;}
  else{snprintf(line,sizeof line,"%s",trial);n=1;}
  if(*s=='\n'){s++;text(f,size,x,y+used,c,line);used+=lh;line[0]=0;n=0;}}
 if(n){text(f,size,x,y+used,c,line);used+=lh;}
 return used;}

#define SX 34
#define SY 6
#define CW (W-2*SX)
static int stage_top,stage_bot,cur_y;

enum{ST_FACTORY,ST_UNLOCKED,ST_HOOKED};
static int con=ST_FACTORY,prog,frame_n;
static char serial_txt[16]="-------";

enum{K_X,K_O,K_TRI,K_SQ,K_R1,K_L1};

typedef struct{const char *t;int key,target,kind,dis;}Btn;
static Btn bar_b[8];static int bar_n;
static int act[4],act_dis[4];

static const char *STEPS[4]={"Unlock","Install","Calibrate","Finish"};
static void header(void){
 fill(0,0,W,H,C_BG);
 vgrad(0,0,W,SY+58,0x132038,0x0e192e);fill(0,SY+58,W,2,C_LINE);
 int lx=SX,ly=SY+6;
 disc_logo(lx-2,ly-2,40);
 text(f_disp,22,lx+48,SY+4,C_FG,"MechaPMAP");
 {char t[64];if(strcmp(serial_txt,"-------"))snprintf(t,sizeof t,"MECHACON %s · %s · %s",P->name,P->name[0]=='5'?"SCPH-5000x":P->name[2]=='0'&&P->name[3]<'4'?"SCPH-7000x":P->name[2]=='0'?"SCPH-7500x":P->name[3]=='0'?"SCPH-7700x":"SCPH-9000x",serial_txt);else snprintf(t,sizeof t,"MECHACON");label(lx+48,SY+38,12,C_MUTED,t);}}
static void rail(int active,int done){

 int rw=396,xe=SX+48+textw(f_disp,22,"MechaPMAP"),x0=xe+(W-xe-rw)/2,y=SY+2,h=32,gap=6,w=(rw-3*gap)/4,i;
 for(i=0;i<4;i++){int x=x0+i*(w+gap);int st=i<done?2:(i==active?1:0);
  u32 bd=st==1?C_ACC:C_LINE,bg=st==1?0x10233c:0x0c1527;
  box(x,y,w,h,8,bg,bd,st==1?2:1);
  u32 ic=st==2?C_OK:st==1?C_ACC2:C_FAINT;
  box(x+5,y+7,18,18,5,st==2?0x0c1f1a:st==1?0x0e2440:0x0a1220,st==2?0x1f4a3a:st==1?C_ACCDIM:C_LINE,1);
  if(st==2)glyph(G_CHECK,x+6,y+8,16,ic);else{char d[2]={(char)('1'+i),0};int tw=textw(f_mono,12,d);text(f_mono,12,x+14-tw/2,y+8,ic,d);}
  {int fs=13;while(fs>10&&textw(f_disp,fs,STEPS[i])>w-32)fs--;text(f_disp,fs,x+28,y+1+(13-fs)/2,st?C_FG:C_MUTED,STEPS[i]);}
  label(x+28,y+17,9,C_MUTED,st==2?"DONE":st==1?"ACTIVE":"PENDING");}
 stage_top=SY+70;stage_bot=H-SY-28;cur_y=stage_top;bar_n=0;}
static char foot[160];static u32 footc;

static void padbar(const char *extra){
 static const u32 padc[4]={0x7fd0ff,0xff8a97,0x6fe0b8,0xf0a0e0};
 int y=H-SY-21,x=SX,i;fill(0,y-5,W,1,C_LINE);
 for(i=0;i<bar_n;i++){const Btn *b=&bar_b[i];
  if(b->key>=K_R1){u32 fg=b->dis?mix(C_MUTED,C_SCREEN,150):C_MUTED,pc=b->dis?mix(0xc9d6e8,C_SCREEN,170):0xc9d6e8;const char *s=b->key==K_R1?"R1":"L1";
   box(x,y,26,17,5,0x0d1627,b->dis?mix(0x2b3f5e,C_SCREEN,120):0x2b3f5e,1);text(f_mono,11,x+13-textw(f_mono,11,s)/2,y+2,pc,s);
   x+=31;x+=text(f_body,14,x,y,fg,b->t)+18;continue;}
  {int g=b->key==K_X?G_CROSS:b->key==K_O?G_CIRCLE:b->key==K_TRI?G_TRI:G_SQUARE;
   u32 gc=padc[b->key],fg=b->kind==0?C_FG:b->kind==3?0xffd28a:C_MUTED;
   if(b->dis){gc=mix(gc,C_SCREEN,170);fg=mix(fg,C_SCREEN,150);}
   glyph(g,x,y+1,16,gc);x+=20;x+=text(b->kind==0?f_bold:f_body,14,x,y,fg,b->t)+18;}}
 if(extra)text(f_body,14,x,y,C_FAINT,extra);
 if(foot[0])textr(f_body,13,W-SX,stage_bot-18,footc,foot);}
static void eyebrow(const char *s){label(SX,cur_y,13,C_ACC2,s);cur_y+=20;}
static void title(const char *s){text(f_disp,28,SX,cur_y,C_FG,s);cur_y+=40;}
static void lede(const char *s){cur_y+=para(f_body,18,24,SX,cur_y,CW-20,C_MUTED,s)+8;}
static void hero(int g,int tone){u32 c=tone==1?C_WARN:tone==2?C_OK:C_ACC2,bg=tone==1?0x211a0e:tone==2?0x0d2219:0x0f1f38,bd=tone==1?0x4a3a1c:tone==2?0x1f4a3a:C_LINE;
 box(SX,cur_y,64,64,14,bg,bd,1);glyph(g,SX+12,cur_y+12,40,c);cur_y+=76;}
static void badge(const char *s,int tone){u32 c=tone==0?C_WARN:tone==1?C_ACC2:C_OK,bg=tone==0?0x17120a:tone==1?0x0d1d33:0x0b1d17,bd=tone==0?0x4a3a1c:tone==1?C_ACCDIM:0x1f4a3a;
 int w=textw(f_mono,15,s)+28;box(SX,cur_y,w,30,15,bg,bd,1);text(f_mono,15,SX+14,cur_y+6,c,s);cur_y+=42;}

static void row(int icon,const char *ictext,const char *b,const char *sm,const char *rs,int st){
 int h=sm?40:32,y=cur_y;

 if(st==3)box(SX,y,CW,h,9,0x10233c,C_ACC,2);else box(SX,y,CW,h,9,C_ROW,st==1?C_ACCDIM:C_LINE,1);
 u32 ic=st==2?C_OK:st==1||st==3?C_ACC2:st==4?C_FAINT:C_MUTED;
 box(SX+8,y+(h-24)/2,24,24,6,st==2?0x0c1f1a:st==1?0x0e2440:0x0a1220,st==2?0x1f4a3a:st==1?C_ACCDIM:C_LINE,1);
 if(st==2)glyph(G_CHECK,SX+10,y+(h-24)/2+2,20,ic);
 else if(icon>=0)glyph(icon,SX+10,y+(h-24)/2+2,20,ic);
 else if(ictext){int tw=textw(f_mono,13,ictext);text(f_mono,13,SX+20-tw/2,y+(h-24)/2+4,ic,ictext);}
 text(f_bold,16,SX+44,y+(sm?2:6),st==4?C_FAINT:C_FG,b);
 if(sm)text(f_body,13,SX+44,y+21,st==4?mix(C_FAINT,C_SCREEN,90):C_MUTED,sm);
 if(rs)textr(f_mono,14,SX+CW-12,y+(h-18)/2,st==2?C_OK:st==1?C_ACC2:C_FAINT,rs);
 cur_y+=h+6;}
static void note(int tone,const char *s){
 u32 bg=tone==0?0x0d1d33:tone==1?0x1c150a:0x200d10,bd=tone==0?C_ACCDIM:tone==1?0x4a3a1c:0x5a2028,fg=tone==0?0xcfe7ff:tone==1?0xffe2b0:0xffc2c8;
 int th=para(f_body,15,20,0,-1000,CW-60,fg,s);int h=th+18;
 box(SX,cur_y,CW,h,9,bg,bd,1);glyph(tone?G_WARN:G_CHECK,SX+10,cur_y+9,20,fg);
 para(f_body,15,20,SX+40,cur_y+9,CW-60,fg,s);cur_y+=h+8;}
static void bar(const char *cap,const char *right){
 int y=cur_y+4;box(SX,y,CW,12,6,0x0a1322,C_LINE,1);
 int w=(CW-4)*prog/100;if(w>0)vgrad(SX+2,y+2,w,8,C_ACC,C_ACC2);
 text(f_mono,13,SX,y+18,C_MUTED,prog>=100?"done":cap);textr(f_mono,13,SX+CW,y+18,C_MUTED,right);cur_y+=42;}

static void buttons(const Btn *b,int n){int i;
 for(i=0;i<4;i++){act[i]=-1;act_dis[i]=0;}
 bar_n=0;for(i=0;i<n&&bar_n<8;i++){bar_b[bar_n++]=b[i];if(b[i].key<=K_SQ){act[b[i].key]=b[i].target;act_dis[b[i].key]=b[i].dis;}}}
