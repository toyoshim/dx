#include <stdio.h>
#include <stdlib.h>
#include "int.h"
#include "file.h"

void
GetDeviceInformation(i86_Regs *regs)
{
	int rc;
	rc = file_get_devinfo(WORD(REG_BX));
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_DX = WORD(rc);
		RESET_CARRY;
	}
	printf("GET DEVICE INFORMATION %d => %04x\n", WORD(REG_BX), rc);
}

void
SetDeviceInformation(i86_Regs *regs)
{
	int rc;
	rc = file_set_devinfo(WORD(REG_BX), WORD(REG_DX));
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		RESET_CARRY;
	}
	printf("SET DEVICE INFORMATION %d <= %04x => %d\n", WORD(REG_BX), WORD(REG_DX), rc);
}

void
DosCall44h(i86_Regs *regs)
{
	switch (REG_AL) {
	case 0x00:	GetDeviceInformation(regs);		break;
	case 0x01:	SetDeviceInformation(regs);		break;
	default:
		fprintf(stdout, "int21(AX=%04x): not implemented.\n", WORD(REG_AX));
		exit(1);
		break;
	}
}

