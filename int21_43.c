#include <stdio.h>
#include <stdlib.h>
#include "int.h"
#include "file.h"

void
GetFileAttributes(i86_Regs *regs)
{
	int rc;
	const char * fname = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	rc = file_attribute(fname);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_CX = REG_AX = WORD(rc);
		RESET_CARRY;
	}
	printf("GET FILE ATTRIBUTES: %s => %04x\n", fname, rc);
}

void
DosCall43h(i86_Regs *regs)
{
	switch (REG_AL) {
	case 0x00:	GetFileAttributes(regs);		break;
	default:
		fprintf(stdout, "int21(AX=%04x): not implemented.\n", WORD(REG_AX));
		exit(1);
		break;
	}
}

