#include <stdio.h>
#include <stdlib.h>
#include "int.h"

extern void int1a(i86_Regs *regs);
extern void int21(i86_Regs *regs);
extern void int29(i86_Regs *regs);

void
int_hook(int no, void *_regs)
{
	i86_Regs *regs = (i86_Regs *)_regs;
	printf("[%08x(%04x:%04x)]: ", (WORD(MEM_CS) * 16 + WORD(MEM_IP)), WORD(MEM_CS), WORD(MEM_IP));
	switch (no) {
	case 0x1a:	int1a((i86_Regs *)regs);		break;
	case 0x21:	int21((i86_Regs *)regs);		break;
	case 0x29:	int29((i86_Regs *)regs);		break;
	default:	fprintf(stderr, "INT %02X\n", no);	break;
	}
}

