#include <stdio.h>
#include <stdlib.h>
#include "int.h"

extern void int1a(i86_Regs *regs);
extern void int21(i86_Regs *regs);

void
int_hook(int no, void *_regs)
{
	i86_Regs *regs = (i86_Regs *)_regs;
	printf("[%08x(%04x:%04x)]: ", (WORD(MEM_CS) * 16 + WORD(MEM_IP)), WORD(MEM_CS), WORD(MEM_IP));
	if (0x21 == no) int21((i86_Regs *)regs);
	else if (0x1a == no) int1a((i86_Regs *)regs);
	else fprintf(stderr, "INT %02X\n", no);
}

