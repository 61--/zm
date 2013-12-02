#include "out.h"
#include <stdarg.h>
#include <Wdm.h>
#include <ntstrsafe.h>

#define OUT_BUFFER_SIZE 246

void __DbgOut(const char* format, ...)
{
	char buf[OUT_BUFFER_SIZE] = { 0 };
	NTSTATUS status;
	va_list list;
	size_t len;
	va_start(list, format);
	
	status = RtlStringCbVPrintfA(
		buf, sizeof(buf), format, list);
		
	if (status == STATUS_SUCCESS){
		len = strlen(buf);
		if (len < OUT_BUFFER_SIZE - 1){
			buf[len] = '\n';
		}
		DbgPrint(buf);
	}
	else{
		DbgPrint("DbgOut format string error %d", status);
	}
	va_end(list);
}

void __DbgOut_(const char* file, int line, const char* format, ...)
{
	char buf[OUT_BUFFER_SIZE] = { 0 };
	NTSTATUS status;
	va_list list;
	size_t len;
	va_start(list, format);

	status = RtlStringCbVPrintfA(
		buf, sizeof(buf), format, list);

	if (status == STATUS_SUCCESS){
		len = strlen(buf);
		if (len < OUT_BUFFER_SIZE - 1){
			buf[len] = '\n';
		}
		DbgPrint("[zm][%s:%d] %s", file, line, buf);
	}
	else{
		DbgPrint("[zm][%s:%d]  DbgOut format string error: %d", file, line, status);
	}
	va_end(list);
}