#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include "int.h"

static void
GetSystemTime(i86_Regs *regs)
{
	double t;
	struct timeval tv;
	unsigned long lt;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	t *= 1000000.0;
	t += (double)tv.tv_usec;
	t *= 18.2;
	t /= 1000000.0;
	t = fmod(rint(t), (double)0xffffffff);
	lt = (unsigned long)t;
	REG_CX = WORD(lt >> 16);
	REG_DX = WORD(lt);
	REG_AL = 0;	/* TODO: midnight flag */
	printf("GET SYSTEM TIME => %08x\n", lt);
}

void
int1a(i86_Regs *regs)
{
	switch (REG_AH) {
	case 0x00:	GetSystemTime(regs);			break;
	default:
		fprintf(stdout, "int1A(AH=%02x): not implemented.\n", REG_AH);
		exit(1);
		break;
	}
}

