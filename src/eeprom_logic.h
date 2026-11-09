#ifndef PMAP142_EEPROM_LOGIC_H
#define PMAP142_EEPROM_LOGIC_H
#include <stdint.h>
#include <string.h>
#define P142_EE_BYTES 1024
#define P142_EE_WORDS 512
#define P142_SUM_WORD 0x30

static const uint8_t ee_sums610[11]={5,0x00,0x5e,0x30,0x52,0xa0,0x3a,0x60,0x3e,0x80,0x2a};
static const uint8_t *ee_sums=ee_sums610;
static unsigned ee_len30=0x52;

static unsigned ee_last30=0x59;
static unsigned p142_word(const uint8_t *p,unsigned w){return p[2*w]|((unsigned)p[2*w+1]<<8);}
static void p142_put_word(uint8_t *p,unsigned w,unsigned v){p[2*w]=(uint8_t)v;p[2*w+1]=(uint8_t)(v>>8);}
static unsigned p142_sum30(const uint8_t *img){unsigned s=0,i;for(i=0;i<ee_len30;i++)s+=img[2*0x31+i];return (~s)&0xff;}
static int p142_block30_ok(const uint8_t *img){return p142_word(img,0x30)==p142_sum30(img);}
static int p142_all_sums_ok(const uint8_t *img){unsigned b,i,s,w,n;for(b=0;b<ee_sums[0];b++){w=ee_sums[1+2*b];n=ee_sums[2+2*b];s=0;for(i=0;i<n;i++)s+=img[2*(w+1)+i];if(p142_word(img,w)!=((~s)&0xff))return 0;}return 1;}

static int ee_set_sums(const uint8_t *t){unsigned b;ee_sums=t;for(b=0;b<t[0];b++)if(t[1+2*b]==0x30){ee_len30=t[2+2*b];ee_last30=0x30+ee_len30/2;return 1;}return 0;}

static int p142_make_fixed(const uint8_t *after,uint8_t *fixed,unsigned sum){
 unsigned i;for(i=0;i<P142_EE_BYTES;i++)fixed[i]=after[i];p142_put_word(fixed,0x30,sum);
 return p142_block30_ok(fixed);
}
#endif
