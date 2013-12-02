#include "utility.h"
#include <Wdm.h>

#define  DELAY_ONE_MICROSECOND  (-10)
NTSTATUS zm_sleep(size_t tick)
{
	LARGE_INTEGER interval;
	interval.QuadPart = (tick * DELAY_ONE_MICROSECOND);
	return KeDelayExecutionThread(
		KernelMode, FALSE, &interval);
}

void* zm_GetIdt()
{
	ZM_IDTR idtr;
	_asm sidt idtr
	return (void*)idtr.base;
}