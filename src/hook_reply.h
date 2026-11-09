#ifndef P156_HOOK_REPLY_H
#define P156_HOOK_REPLY_H

#define P156_ARM_MAGIC 0x31344151u
static inline unsigned p156_op1_arg(unsigned off,unsigned half){return (off<<16)|(half&0xffffu);}
static inline int p156_op1_echo_ok(const unsigned char *out,unsigned off,unsigned half){unsigned v=p156_op1_arg(off,half);return out[0]==0x48&&out[1]==(unsigned char)v&&out[2]==(unsigned char)(v>>8)&&out[3]==(unsigned char)(v>>16)&&out[4]==(unsigned char)(v>>24);}

static inline int p156_adj_ok(unsigned w0){return (w0&0xffffu)==0x0001u;}

static inline int p156_c17_is_sl(unsigned w0){return (w0&0x00ffffffu)==0x00140001u;}

static inline int p156_type_is_cd(int t){return t>=0x10&&t<=0x13;}

static inline int p156_cd_disc(int t){return p156_type_is_cd(t)||t==0xfd;}
#define P156_DISC_STATE 0x020000d4u
#endif
