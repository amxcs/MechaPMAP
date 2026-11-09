#ifndef PMAP142_PROTOCOL_H
#define PMAP142_PROTOCOL_H
#define PMAP142_RPC 0x80001420
#define PMAP142_MAGIC 0x32343150
enum { P142_STATUS=0, P142_SCMD=1, P142_NCMD0A=2, P142_READ=3, P142_WRITE=4, P142_STOP=5, P142_SYNC=6 };
enum { P142_OK=0, P142_BAD_REQUEST=1, P142_NOT_READY=2 };
typedef struct {
    unsigned magic; int result, error, type, status, ready;
    unsigned polls, busy; unsigned char length, refused, timeout, reserved;
    unsigned char in[12], out[16];
} pmap142_rpc_t;
typedef char pmap142_rpc_size[(sizeof(pmap142_rpc_t)==64)?1:-1];
#endif
