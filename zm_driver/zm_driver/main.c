#include <ntddk.h>
#include <ntstrsafe.h>
#include <Wdm.h>
#include "out.h"
#include "utility.h"


NTSTATUS DriverEntry(
	PDRIVER_OBJECT driver, PUNICODE_STRING reg_path);
#pragma alloc_text(INIT, DriverEntry)


void* g_int93_pos;
void Interrupt0x93_Proc()
{

}
__declspec(naked) Hook_0x93Interrupt()
{
	__asm{
		pushad
		pushfd
		call Interrupt0x93_Proc
		popfd
		popad
		jmp g_int93_pos
	}
}

void HookInt93(BOOLEAN is_hook)
{
	ZM_PIDTENTRY idt_addr = (ZM_PIDTENTRY)zm_GetIdt();
	idt_addr += 0x93;
	if (is_hook){
		g_int93_pos = (void*)ZM_MAKELONG(
			idt_addr->offset_low, idt_addr->offset_high);

		DbgOut("Start Hook Int 93");
	}
}

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgOut("DriverUnload");
	zm_sleep(20 * 1000);
	DbgOut("DriverUnload Success");
}

NTSTATUS DriverEntry(
	PDRIVER_OBJECT driver,PUNICODE_STRING reg_path) 
{ 
	DbgOut("some info %d", 112233);
	DbgOut("====================");
	DbgOut(">>>>>>>>>>>>>>>>>");


	driver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS; 
}
