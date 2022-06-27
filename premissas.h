#ifndef __PREMISSAS_H__
#define __PREMISSAS_H__

#include "types.h"

#define OFFSET_BITS 0
#define MAX_PAGE    4
#define MAX_ADDR    (MAX_PAGE << OFFSET_BITS)

#define MAX_FRAME   64
#define FRAME_SIZE  (1 << OFFSET_BITS)

#define MEM_SIZE    (MAX_FRAME * FRAME_SIZE)

#define WORKING_SET 4

typedef u8 FrameIdx;
typedef u8 PageNum;

typedef u8 Pid;
#define S_PID           "%hhu"
#define P_PID(pid)      (pid)

typedef u8 Vaddr;
#define VirtPage(va)    ((va)>>OFFSET_BITS)
#define VirtOffset(va)  ((PageNum)((va)&((1<<OFFSET_BITS)-1)))
#define S_VADDR         "%02hhuv%01hhu"
#define P_VADDR(va)     VirtPage(va), VirtOffset(va)

typedef u8 Faddr;
#define RealAddr(frm, va)   (((frm)<<OFFSET_BITS) | (VirtOffset(va)))
#define S_FADDR             "%02hhx"
#define P_FADDR(fa)         (fa)

#define SLEEP_TIME  3

#endif // __PREMISSAS_H__
