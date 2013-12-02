#include <stdio.h>

typedef struct _Test{
	int a;
	int b;
	int c;
}Test, *PTest;

void __cdecl main()
{
	PTest p = (PTest) 0x1000;
	p += 1;
	printf("p = %X", p);
	printf("some info");
}