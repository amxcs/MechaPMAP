#include "irx_imports.h"
#include "../../src/protocol.h"
IRX_ID("pmap142",1,0);
extern struct irx_export_table _exp_pmap142;
static SifRpcDataQueue_t queue; static SifRpcServerData_t server;
static unsigned char rpcbuf[128] __attribute__((aligned(64)));
static unsigned ncmd_limit=6000;
static int nvm_word(const unsigned char *in){return (in[0]<<8)|in[1];}
static void wait_done(pmap142_rpc_t *p,unsigned limit){p->polls=0;p->timeout=0;while(sceCdSync(1)){if(p->polls>=limit){p->timeout=1;break;}DelayThread(10000);++p->polls;}}
static void *request(int fn,void *buf,int len){pmap142_rpc_t *p=buf;if(!p||len!=(int)sizeof(*p))return buf;
 p->magic=PMAP142_MAGIC;p->result=0;p->refused=0;p->timeout=0;p->polls=0;memset(p->out,0xcc,sizeof(p->out));
 switch(fn){
 case P142_STATUS:p->result=1;break;
 case P142_SCMD:
   if(p->length==9&&p->in[0]==0xa4&&p->in[1]==0x56&&p->in[2]==0x32&&p->in[3]==0x42)p->result=sceCdApplySCmd(3,p->in,9,p->out);
   else if(p->length==1&&(p->in[0]==0x00||p->in[0]==0xfd))p->result=sceCdApplySCmd(3,p->in,1,p->out);
   else {p->refused=P142_BAD_REQUEST;} break;
 case P142_NCMD0A:
   if(p->length!=0){p->refused=P142_BAD_REQUEST;break;}
   if(sceCdDiskReady(1)!=2){p->refused=P142_NOT_READY;break;}
   p->result=sceCdApplyNCmd(0x0a,p->in,0);if(p->result)wait_done(p,ncmd_limit);break;
 case P142_READ:
   if(p->length!=2||nvm_word(p->in)>=512){p->refused=P142_BAD_REQUEST;break;}
   p->result=sceCdApplySCmd(0x0a,p->in,2,p->out);break;
 case P142_WRITE:
   if(p->length!=4){p->refused=P142_BAD_REQUEST;break;}
   if(!(nvm_word(p->in)>=0x30&&nvm_word(p->in)<=0x5a)){p->refused=P142_BAD_REQUEST;break;}
   p->result=sceCdApplySCmd(0x0b,p->in,4,p->out);break;
 case P142_STOP:
   p->result=sceCdStop();if(p->result)wait_done(p,1500);break;
 case P142_SYNC:
   p->busy=(unsigned)sceCdSync(1);p->result=1;break;
 default:p->refused=P142_BAD_REQUEST;break;
 }
 p->error=sceCdGetError();p->type=sceCdGetDiskType();p->ready=sceCdDiskReady(1);p->status=sceCdStatus();return buf;
}
static void worker(void *unused){(void)unused;if(!sceSifCheckInit())sceSifInit();sceSifInitRpc(0);sceSifSetRpcQueue(&queue,GetThreadId());sceSifRegisterRpc(&server,PMAP142_RPC,request,rpcbuf,0,0,&queue);sceSifRpcLoop(&queue);}
int _start(int argc,char **argv){iop_thread_t t={0};int id;(void)argc;(void)argv;if(RegisterLibraryEntries(&_exp_pmap142))return MODULE_NO_RESIDENT_END;t.attr=TH_C;t.priority=0x28;t.stacksize=0x1000;t.thread=worker;id=CreateThread(&t);if(id<0||StartThread(id,0)<0)return MODULE_NO_RESIDENT_END;return MODULE_RESIDENT_END;}
