#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <libpad.h>
#include <libcdvd.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "pad.h"
#include "mecha.h"
#include "protocol.h"
#include "eeprom_logic.h"
#include "profile.h"

static const Profile *P=&PROFILES[0];
#define P156_JOB (P->job)
#define P156_RESULT (P->result)
#include "hook_reply.h"
#include "candidate_blob.h"
#include "factory610.h"
#include "region.h"

#define REG_ROWS (P->name[0]=='6'&&P->minor>=6?NREG:0)

#define DECL(n) extern unsigned char n[]; extern unsigned int size_##n
DECL(freesio2_irx); DECL(freepad_irx); DECL(mechaproxy_irx);
DECL(font_disp); DECL(font_body); DECL(font_bold); DECL(font_mono); DECL(pwr70k_rgb);
extern int MechaScmd(u8 cmd, void *input, u8 inputlength, void *output, u8 outputlength);

#include "ui_render.h"
#include "calib_core.h"
static void diag_pause(void){pause_ms(10);}
#include "nvm_io.h"

#define PATCH 0x320
static unsigned char img0[1024], pre[1024], bak[1024];
static int have_bak, wrote_any;
static uint32_t serial;
static char bakname[48], statename[48], csvname[48];
static const char *patch_name="unknown";
enum{P_UNKNOWN,P_UNLOCK,P_PMAP,P_V2,P_OTHER};
static int patch_kind;
static char pending[16];
static int pending_region(void){return !strncmp(pending,"v2:",3)?atoi(pending+3):-1;}
static int pending_valid(const char *p){return !strcmp(p,"v2")||!strcmp(p,"patch")||!strcmp(p,"restore")||
 (!strncmp(p,"v2:",3)&&p[3]>='0'&&p[3]<='9'&&!p[4]&&atoi(p+3)<REG_ROWS);}

typedef struct{const char *g,*w,*name;unsigned short word;char part,k;int bits,now,val,nruns,runs[16];}Field;
static Field F[]={
 {"Thresholds","0x33","CD/DVD boundary",0x33,'M','d',16},
 {"Thresholds","0x34","CD level (c97 00)",0x34,'W','d',16},
 {"Thresholds","0x35","DVD level (c97 01)",0x35,'W','d',16},
 {"Thresholds","0x36","boundary − 1",0x36,'M','d',16},
 {"CD","0x40 hi","Focus search time",0x40,'H','h',8},
 {"CD","0x42","F0",0x42,'W','s',9},
 {"CD","0x43 lo","FE gain",0x43,'L','h',8},
 {"CD","0x44 lo","TE gain",0x44,'L','h',8},
 {"CD","0x45","F4",0x45,'W','s',9},
 {"DVD-SL","0x49","F0",0x49,'W','s',9},
 {"DVD-SL","0x4A lo","FE gain",0x4A,'L','h',8},
 {"DVD-SL","0x4B lo","TE gain",0x4B,'L','h',8},
 {"DVD-SL","0x4C","F4",0x4C,'W','s',9},
 {"DVD-SL","0x4E hi","Register 0x16 base",0x4E,'H','h',8},
 {"DVD-DL","0x51 lo","FE gain L0",0x51,'L','h',8},
 {"DVD-DL","0x51 hi","FE gain L1",0x51,'H','h',8},
 {"DVD-DL","0x52 lo","TE gain L0",0x52,'L','h',8},
 {"DVD-DL","0x52 hi","TE gain L1",0x52,'H','h',8},
 {"DVD-DL","0x53","F4 L0",0x53,'W','s',9},
 {"DVD-DL","0x54","F4 L1",0x54,'W','s',9},
 {"DVD-DL","0x56 lo","Register 0x16 L0",0x56,'L','h',8},
 {"DVD-DL","0x56 hi","Register 0x16 L1",0x56,'H','h',8}};
#define NF ((int)(sizeof F/sizeof F[0]))
static int read_file(const char *name,void *buf,int max);
static int sel,scroll_,pad_step=1;
#include "fields.h"

static int pick_usb(void){static const char *roots[]={"mass:/","mass0:/",NULL};unsigned start=ms(),n;int r,fd;char p[96];
 do{for(r=0;roots[r];r++){fd=open(roots[r],O_RDONLY);if(fd>=0)close(fd);
   for(n=0;n<1000;n++){snprintf(p,sizeof p,"%smechapmap_%03u.log",roots[r],n);fd=open(p,O_RDONLY);if(fd>=0){close(fd);continue;}
    strcpy(device,roots[r]);logno=n;snprintf(logname,sizeof logname,"mechapmap_%03u.log",n);
    {int keep=loglen;report("USB_READY root=%s log=%s after_ms=%u\n",roots[r],logname,ms()-start);if(flush()){flush_bad=0;return 1;}
     loglen=keep;logbuf[loglen]=0;}usb_errno=errno;device[0]=0;break;}}
  pause_ms(250);}while(ms()-start<20000);
 return 0;}
static int read_file(const char *name,void *buf,int max){char p[96];int fd,n;snprintf(p,sizeof p,"%s%s",device,name);fd=open(p,O_RDONLY);if(fd<0)return (errno==ENOENT||fd==-ENOENT)?-1:-2;n=read(fd,buf,max);close(fd);return n;}
static int write_file(const char *name,const void *buf,int size){char p[96];int fd,n;snprintf(p,sizeof p,"%s%s",device,name);
 fd=open(p,O_WRONLY|O_CREAT|O_TRUNC,0666);if(fd<0)return 0;n=write(fd,buf,size);if(close(fd)<0)return 0;return n==size&&file_ok(name,buf,size);}
static int save_seq(const char *what,const unsigned char *img){static unsigned seq;char n[40];snprintf(n,sizeof n,"%s_%02u",what,seq++);return save(n,img,1024);}

static int set_pending(const char *s){int ok,e1=0,rm,n;char back[16],p[96];snprintf(pending,sizeof pending,"%s",s);
 snprintf(p,sizeof p,"%s%s",device,statename);
 if(!s[0]){errno=0;rm=remove(p);e1=errno;n=read_file(statename,back,sizeof back);ok=n==-1||n==0;
  report("PENDING (none) remove=%d errno=%d after=%d written=%d\n",rm,e1,n,ok);flush();return ok;}
 errno=0;ok=write_file(statename,pending,strlen(pending));e1=errno;
 if(!ok){errno=0;rm=remove(p);report("PENDING_RETRY first errno=%d remove=%d errno=%d\n",e1,rm,errno);errno=0;ok=write_file(statename,pending,strlen(pending));}
 report("PENDING %s written=%d errno=%d\n",pending,ok,ok?0:errno);flush();return ok;}

static int classify(const unsigned char *img){unsigned i;
 if(!memcmp(img+PATCH,P->unlock,224)){patch_name=P->unlock_name;return P_UNLOCK;}
 if(!memcmp(img+PATCH,P->hook,224)){patch_name=P->hook_name;return P_PMAP;}
 if(!memcmp(img+PATCH,P->fuv2,224)){patch_name="Force Unlock v2";return P_V2;}
 if(!memcmp(img+PATCH,P->factory,224)){patch_name=P->factory_name;return P_OTHER;}
 if(P==&PROFILES[0])for(i=0;i<sizeof choice_patches/sizeof choice_patches[0];i++)if(!memcmp(img+PATCH,choice_patches[i].blob,224)){patch_name=choice_patches[i].name;return P_OTHER;}
 patch_name="unknown";return P_UNKNOWN;}
static int bak_valid(const unsigned char *b){return p142_all_sums_ok(b)&&classify(b)!=P_UNKNOWN;}

typedef struct{int icon;const char *ic,*label,*sub;int st;}TRow;
static TRow trow[6];static int ntrow;static const char *t_eb,*t_title,*t_cap,*t_right;
enum{S_WELCOME,S_TESTDISC,S_LASER,S_LIVE,S_TUNE,S_REGION,S_UNLOCK_CONFIRM,S_HOOK_CONFIRM,S_CALIB_INTRO,S_INSERT,S_TASK,S_RESULT,S_ADV_MENU,S_ADV_MEASURE,S_ADV_EDIT,S_ADV_REVIEW,
 S_FINISH,S_FINISH_B,S_RESTORE_CONFIRM,S_RESTORE_B,S_POWER,S_DONE,S_ERROR};
static int scr=S_WELCOME;
static int ins_type=-1,ins_ready=-1,ins_lid=-1,ins_force;static unsigned ins_since;
static const char *ins_kind,*pw_eb,*pw_title,*pw_lede,*pw_note;static int err_back;
static void draw(void);static void present(void);
static void task_begin(const char *eb,const char *title,const char *cap,const char *right){t_eb=eb;t_title=title;t_cap=cap;t_right=right;ntrow=0;prog=0;scr=S_TASK;}
static void task_row(int icon,const char *ic,const char *label,const char *sub){trow[ntrow++]=(TRow){icon,ic,label,sub,0};}
static void task_mark(int i,int st){int k;trow[i].st=st;if(st==1){for(k=0;k<i;k++)trow[k].st=2;}
 {int done=0;for(k=0;k<ntrow;k++)if(trow[k].st==2)done++;prog=done*100/(ntrow?ntrow:1);}draw();present();}
static void fail_(const char *msg,int back){snprintf(failure,sizeof failure,"%s",msg);report("FAIL %s\n",msg);flush();err_back=back;scr=S_ERROR;}
static void power(const char *eb,const char *title,const char *lede,const char *note_){pw_eb=eb;pw_title=title;pw_lede=lede;pw_note=note_;scr=S_POWER;}

#include "ops.h"
#include "screens.h"

static void present(void){tex.Mem=fb;FlushCache(0);gsKit_texture_upload(gs,&tex);
 gsKit_prim_sprite_texture(gs,&tex,0,0,0,0,W,gs->Height,W,H,1,GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0));
 gsKit_queue_exec(gs);gsKit_sync_flip(gs);}
static char romver[16];
int main(void){
 last_tick=ticks();SifInitRpc(0);
 {FILE *rv=fopen("rom0:ROMVER","r");if(rv){if(fread(romver,1,sizeof romver-1,rv)<5)romver[0]=0;fclose(rv);}}
 int pal=romver[4]=='E';
 gs=gsKit_init_global();
 gs->Mode=pal?GS_MODE_PAL:GS_MODE_NTSC;gs->Interlace=GS_INTERLACED;gs->Field=GS_FIELD;gs->Width=640;gs->Height=pal?512:448;
 gs->PSM=GS_PSM_CT24;gs->PSMZ=GS_PSMZ_16S;gs->ZBuffering=GS_SETTING_OFF;gs->DoubleBuffering=GS_SETTING_OFF;gs->PrimAlphaEnable=GS_SETTING_OFF;
 dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
 dmaKit_chan_init(DMA_CHANNEL_GIF);gsKit_init_screen(gs);gsKit_mode_switch(gs,GS_ONESHOT);
 H=512;fb=memalign(128,W*H*4);
 memset(&tex,0,sizeof tex);tex.Width=W;tex.Height=H;tex.PSM=GS_PSM_CT32;tex.Filter=gs->Height==H?GS_FILTER_NEAREST:GS_FILTER_LINEAR;
 tex.Vram=gsKit_vram_alloc(gs,gsKit_texture_size(W,H,GS_PSM_CT32),GSKIT_ALLOC_USERBUFFER);
 FT_Init_FreeType(&ftl);
 FT_New_Memory_Face(ftl,font_disp,size_font_disp,0,&f_disp);FT_New_Memory_Face(ftl,font_body,size_font_body,0,&f_body);
 FT_New_Memory_Face(ftl,font_bold,size_font_bold,0,&f_bold);FT_New_Memory_Face(ftl,font_mono,size_font_mono,0,&f_mono);
 startup();
 draw();present();

 {const u32 AR=PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT;u32 held=0;unsigned t0=0,next=0;
  for(;;){u32 p=ReadCombinedPadStatus(),h=(u32)ReadCombinedPadStatus_raw()&AR;unsigned now=ms();
   if(p&AR){held=p&AR;t0=now;next=now+400;pad_step=1;}
   else if(held&&(h&held)==held){if((int)(now-next)>=0){p|=held;pad_step=now-t0>2000?10:1;next=now+(now-t0>2000?40:70);}}
   else{held=0;pad_step=1;}
   if(p&&handle(p))draw(),present();else gsKit_vsync_wait();}}
 return 0;}
