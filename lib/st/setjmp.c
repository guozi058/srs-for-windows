#include <setjmp.h>

int __declspec(naked) asm_setjmp(jmp_buf b)
{
	__asm {
		mov ECX, [ESP]
		mov EAX, [ESP+4]
		mov [EAX], EBP
			mov [EAX+4], EBX
			mov [EAX+8], EDI
			mov [EAX+12], ESI
			mov [EAX+16], ESP
			mov [EAX+20], ECX
			mov EAX, 0
			ret
	}
}

void __declspec(naked) asm_longjmp(jmp_buf b, int v)
{
	__asm {
		mov EAX, [ESP+8]
		mov ECX, [ESP+4]
		mov ESP, [ECX+16]
		mov EBP, [ECX]
		mov EBX, [ECX+4]
		mov EDI, [ECX+8]
		mov ESI, [ECX+12]
		mov ECX, [ECX+20]
		mov [ESP], ECX
			ret
	}
}
