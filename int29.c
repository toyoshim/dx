#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include "int.h"
#include "file.h"

void
int29(i86_Regs *regs)
{
	/* Fast Console Output */
	unsigned char code = REG_AL;
	int fd = file_dos2native(STDOUT_FILENO);
	write(fd, &code, 1);
}

