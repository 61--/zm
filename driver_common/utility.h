#ifndef __UTILITY_H
#define __UTILITY_H
#include <ntddk.h>
#include <ntstrsafe.h>

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

#define ZM_MAKELONG(low, high) \
	((U32)(((U16)((U32)(low)& 0xffff)) | ((U32)((U16)((U32)(high)& 0xffff))) << 16))

#define ZM_LOW16_OF_32(data) \
	((U16)(((U32)data) & 0xffff))

#define ZM_HIGH16_OF_32(data) \
	((U16)(((U32)data) >> 16))


NTSTATUS zm_sleep(size_t tick);

//ÖÐ¶Ï±í
typedef struct ZM_IDTR_{
	U16 limit;
	U32 base;
} ZM_IDTR, *ZM_PIDTR;

#pragma pack(push,1)
typedef struct ZM_IDT_ENTRY_ {
	U16 offset_low;
	U16 selector;
	U8 reserved;
	U8 type : 4;
	U8 always0 : 1;
	U8 dpl : 2;
	U8 present : 1;
	U16 offset_high;
} ZM_IDTENTRY, *ZM_PIDTENTRY;
#pragma pack(pop)

void* zm_GetIdt();





#endif//__UTILITY_H