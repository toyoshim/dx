#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <setjmp.h>
#include "int.h"
#include "file.h"
#include "process.h"

extern int jmpcode;
extern jmp_buf jmpdata;

extern void DosCall43h(i86_Regs *regs);
extern void DosCall44h(i86_Regs *regs);

void
TerminateProgram(i86_Regs *regs)
{
	printf("TERMINATE PROGRAM\n");
	jmpcode = 0;
	longjmp(jmpdata, 1);
}

void
WriteStringToStandardOutput(i86_Regs *regs)
{
	const char *str = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	char *end = strchr(str, '$');
	int len;
	printf("WRITE STRING TO STANDARD OUTPUT: ");
	if (NULL == end) {
		printf("can not find terminate character\n");
		return;
	}
	len = (long)end - (long)str;
#ifdef _DEBUG
	*end = 0;
	printf("%s\n", str);
	*end = '$';
#endif /* _DEBUG */
	file_write(STDOUT_FILENO, (void *)str, len);
	REG_AL = '$';
}

void
SelectDefaultDrive(i86_Regs *regs)
{
	int drive = REG_DL;
	printf("SELECT DEFAULT DRIVE <= %c:\n", 'A' + drive);
	REG_AL = 1;	/* LASTDRIVE=A */
}

void
GetCurrentDefaultDrive(i86_Regs *regs)
{
	REG_AL = 0;	/* A: */
	printf("GET CURRENT DEFAULT DRIVE => %c:\n", 'A' + REG_AL);
}

void
SetDiskTransferAreaAddress(i86_Regs *regs)
{
	process_info *pi;
	printf("SET DISK TRANSFER AREA ADDRESS <= %02x:%02x\n", WORD(REG_DS), WORD(REG_DX));
	pi = process_get_current();
	pi->dta_seg = WORD(REG_DS);
	pi->dta_adr = WORD(REG_DX);
}

void
SetInterruptVector(i86_Regs *regs)
{
	int no = REG_AL;
	printf("SET INTERRUPT VECTOR %d <= [%04x:%04x]\n", no, WORD(REG_DS), WORD(REG_DX));
	*(unsigned short *)&memory[no * 4 + 0] = WORD(REG_DX);
	*(unsigned short *)&memory[no * 4 + 2] = WORD(REG_DS);
}

void
ParseFilenameIntoFCB(i86_Regs *regs)
{
	int opt = REG_AL;
	int rc;
	int offset1 = WORD(REG_DS) * 16 + WORD(REG_SI);
	int offset2 = WORD(REG_ES) * 16 + WORD(REG_DI);
	char *fname = &memory[offset1];
	struct fcb *fcb = (struct fcb *)&memory[offset2];

	printf("PARSE FILENAME INTO FCB: %s[%04x:%04x](%d) to %08x\n", fname, WORD(REG_DS), WORD(REG_SI), opt, offset2);
	rc = fcb_parse(&fname, opt, fcb);
	if (rc < 0) {
		REG_AL = 0xff;
/*
		REG_DS = WORD(((unsigned long)fname - (unsigned long)memory) / 16);
		REG_SI = WORD(((unsigned long)fname - (unsigned long)memory) % 16);
*/
	} else {
		REG_AL = rc;
	}
}

void
GetSystemDate(i86_Regs *regs)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	REG_CX = WORD(tm->tm_year - 80);
	REG_DH = tm->tm_mon;
	REG_DL = tm->tm_mday;
	REG_AL = tm->tm_wday;
	printf("GET SYSTEM DATE => %04d/%2d/%2d\n", 1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday);
}

void
GetSystemTime(i86_Regs *regs)
{
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	REG_CH = tm->tm_hour;
	REG_CL = tm->tm_min;
	REG_DH = tm->tm_sec;
	REG_DL = tv.tv_usec / 10000;
	printf("GET SYSTEM TIME => %02d:%02d:%02d:%03d\n", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 10000);
}

void
GetDiskTransferAreaAddress(i86_Regs *regs)
{
	process_info *pi = process_get_current();
	printf("GET DISK TRANSFER AREA ADDRESS => %02x:%02x\n", pi->dta_seg, pi->dta_adr);
	REG_ES = WORD(pi->dta_seg);
	REG_BX = WORD(pi->dta_adr);
}

void
GetInterruptVector(i86_Regs *regs)
{
	int no = REG_AL;
	REG_ES = WORD(*(unsigned short *)&memory[no * 4 + 0]);
	REG_BX = WORD(*(unsigned short *)&memory[no * 4 + 0]);
	printf("GET INTERRUPT VECTOR %d => [%04x:%04x]\n", no, WORD(REG_ES), WORD(REG_BX));
}

void
GetDosVersion(i86_Regs *regs)
{
	if (REG_AL) {
		printf("GET DOS VERSION => 7.10\n");
		REG_AL = 0x07;
		REG_AH = 0x10;
	} else {
		printf("GET DOS VERSION (OEM number)");
	}
}

void
ExtendedBreakChecking(i86_Regs *regs)
{
	static int state = 0x01;	/* ON */
	int func = REG_AL;
	int rc;
	switch (func) {
	case 0:	/* get current state */
		rc = state;
		break;
	case 1: /* set state */
		state = REG_DL;
		rc = state;
		break;
	}
	printf("EXTENDED BREAK CHECKING (%d) => %d\n", func, rc);
	REG_DL = rc;
}

void
CreateOrTruncateFile(i86_Regs *regs)
{
	int attr = WORD(REG_CX);
	char *fname = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	int rc = file_create(fname, attr);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_AX = WORD(rc);
		RESET_CARRY;
	}
	printf("CREATE OR TRUNCATE FILE: %s(%04x) => %d\n", fname, attr, rc);
}

void
OpenExistingFile(i86_Regs *regs)
{
	char *fname = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	int rc;
	rc = file_open(fname, REG_AL);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_AX = WORD(rc);
		RESET_CARRY;
	}
	printf("OPEN EXISTING FILE: %s => %d\n", fname, rc);
}

void
CloseFile(i86_Regs *regs)
{
	int fd = WORD(REG_BX);
	int rc = file_close(fd);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		RESET_CARRY;
	}
	printf("CLOSE FILE: %d => %d\n", fd, rc);
}

void
WriteToFileOrDevice(i86_Regs *regs)
{
	int rc;
	int fd = WORD(REG_BX);
	size_t size = WORD(REG_CX);
	int offset = WORD(REG_DS) * 16 + WORD(REG_DX);
	void *data = &memory[offset];
	rc = file_write(fd, data, size);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_AX = WORD(rc);
		RESET_CARRY;
	}
	printf("WRITE TO FILE OR DEVICE %d <= %08x(%04x) => %d\n", fd, offset, size, rc);
}

void
DeleteFile(i86_Regs *regs)
{
	int rc;
	const char *fname = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	rc = file_unlink(fname);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_AL = 0; /* A: */
		RESET_CARRY;
	}
	printf("DELETE FILE: %s => $d\n", fname, rc);
}

void
SetCurrentFilePosition(i86_Regs *regs)
{
	int rc;
	int from = REG_AL;
	int fd = WORD(REG_BX);
	int offset = (int)((WORD(REG_CX) << 16) | WORD(REG_DX));
	rc = file_seek(fd, offset, from);
	printf("SET CURRENT FILE POSITION: %d (%08x:%d) => %d\n", fd, offset, from, rc);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_DX = WORD(rc >> 16);
		REG_AX = WORD(rc & 0xffff);
		RESET_CARRY;
	}
}

void
ReadFromFileOrDevice(i86_Regs *regs)
{
	int rc;
	int fd = WORD(REG_BX);
	size_t size = WORD(REG_CX);
	int offset = WORD(REG_DS) * 16 + WORD(REG_DX);
	void *data = &memory[offset];
	rc = file_read(fd, data, size);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		REG_AX = WORD(rc);
		RESET_CARRY;
	}
	printf("READ FROM FILE OR DEVIUCE %d <= %08x(%04x) => %d\n", fd, offset, size, rc);
}

void
ForceDuplicateFileHandle(i86_Regs *regs)
{
	int srcfd = WORD(REG_BX);
	int dstfd = WORD(REG_CX);
	int rc = file_dup2(srcfd, dstfd);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		RESET_CARRY;
	}
	printf("FORCE DUPLICATE FILE HANDLE %d => %d\n", srcfd, rc);
}

void
AllocateMemory(i86_Regs *regs)
{
	int nop = WORD(REG_BX);
	int rc = memory_alloc(nop);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		REG_BX = 0;	/* TODO : to be available block */
		SET_CARRY;
	} else {
		REG_AX = rc;
		RESET_CARRY;
	}
	printf("ALLOCATE MEMORY %d => $%08x\n", nop, rc);
}

void
FreeMemory(i86_Regs *regs)
{
	int seg = WORD(REG_ES);
	int rc = memory_free(seg);
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		RESET_CARRY;
	}
}

void
ResizeMemoryBlock(i86_Regs *regs)
{
	int rc;
	printf("RESIZE MEMORY BLOCK %04x <= %04x\n", WORD(REG_ES), WORD(REG_BX));
	rc = memory_realloc(WORD(REG_ES), WORD(REG_BX));
	if (rc < 0) {
		REG_AX = WORD(-rc);
		SET_CARRY;
	} else {
		RESET_CARRY;
	}
}

void
LoadAndOrExecuteProgram(i86_Regs *regs)
{
	int type = REG_AL;
	const char *exename = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	const char *find_exename = file_search(exename);
	struct exec_pb *pb = (struct exec_pb *)&memory[WORD(REG_ES) * 16 + WORD(REG_BX)];
	const char *cmd = &memory[WORD(pb->command_seg) * 16 + WORD(pb->command_offset)];
	process_info *pi = process_open((NULL != find_exename)? find_exename: exename, pb->env_seg, cmd);
	if (0 != type) {
		fprintf(stdout, "int21(AH=%02x): not implemented.\n", REG_AH);
		exit(1);
	}
	if (NULL == pi) {
		printf("LOAD AND/OR EXECUTE PROGRAM: %s(%d): failed\n", exename, type);
		printf("\t found file: %s\n", find_exename);
		SET_CARRY;
	} else {
		jmp_buf old_jmpdata;
		process_info *old_pi = process_get_current();
		char *p = &memory[WORD(pb->env_seg) * 16];
		memcpy(&old_jmpdata, &jmpdata, sizeof(jmp_buf));
		while ((p[0] != 0) || (p[1] != 0)) p++;
		p += 2;
		*p++ = 1;
		*p++ = 0;
		strcpy(p, exename);
		printf("LOAD AND/OR EXECUTE PROGRAM: %s(%d)\n", exename, type);
#ifdef _DEBUG
		printf("\t environment: %04x:0000\n", WORD(pb->env_seg));
		printf("\t command:     %04x:%04x\n", WORD(pb->command_seg), WORD(pb->command_offset));
		printf("\t fcb1:        %04x:%04x\n", WORD(pb->first_fcb_seg), WORD(pb->first_fcb_offset));
		printf("\t fcb2:        %04x:%04x\n", WORD(pb->second_fcb_seg), WORD(pb->second_fcb_offset));
		debug_memory_dump(WORD(pb->command_seg) * 16 + WORD(pb->command_offset), 0x80);
		debug_memory_dump(WORD(pb->first_fcb_seg) * 16 + WORD(pb->first_fcb_offset), sizeof(struct fcb));
		debug_memory_dump(WORD(pb->second_fcb_seg) * 16 + WORD(pb->second_fcb_offset), sizeof(struct fcb));
#endif /* _DEBUG */
		process_suspend(old_pi);
		pi->parent = old_pi;
		if (0 == setjmp(jmpdata)) process_resume(pi);
		printf(" --- CHILD PROCESS EXIT with %d ---\n", jmpcode);
		memcpy(&jmpdata, &old_jmpdata, sizeof(jmp_buf));
		process_close(pi);
		process_resume(old_pi);
		REG_AH = 0;
		RESET_CARRY;
#ifdef _DEBUG
		debug_step_on();
#endif /* _DEBUG */
	}
}

void
TerminateWithReturnCode(i86_Regs *regs)
{
	printf("TERMINATE WITH RETURN CODE: %d\n", REG_AL);
	jmpcode = REG_AL;
	longjmp(jmpdata, 1);
}

void
GetReturnCode(i86_Regs *regs)
{
	printf("GET RETURN CODE => %d\n", jmpcode);
	REG_AH = 0;
	REG_AL = jmpcode;
	RESET_CARRY;
}

void
FindFirstMatchingFile(i86_Regs *regs)
{
	const char *fpath = &memory[WORD(REG_DS) * 16 + WORD(REG_DX)];
	printf("FIND FIRST MATCHING FILE: %s (%02x, %04x)\n", fpath, REG_AL, WORD(REG_CX));
	fprintf(stdout, "not impl.\n");
	exit(0);
}

void
int21(i86_Regs *regs)
{
	switch (REG_AH) {
	case 0x00:	TerminateProgram(regs);			break;
	case 0x09:	WriteStringToStandardOutput(regs);	break;
	case 0x0e:	SelectDefaultDrive(regs);		break;
	case 0x19:	GetCurrentDefaultDrive(regs);		break;
	case 0x1a:	SetDiskTransferAreaAddress(regs);	break;
	case 0x25:	SetInterruptVector(regs);		break;
	case 0x29:	ParseFilenameIntoFCB(regs);		break;
	case 0x2a:	GetSystemDate(regs);			break;
	case 0x2c:	GetSystemTime(regs);			break;
	case 0x2f:	GetDiskTransferAreaAddress(regs);	break;
	case 0x35:	GetInterruptVector(regs);		break;
	case 0x30:	GetDosVersion(regs);			break;
	case 0x33:	ExtendedBreakChecking(regs);		break;
	case 0x3c:	CreateOrTruncateFile(regs);		break;
	case 0x3d:	OpenExistingFile(regs);			break;
	case 0x3e:	CloseFile(regs);			break;
	case 0x3f:	ReadFromFileOrDevice(regs);		break;
	case 0x40:	WriteToFileOrDevice(regs);		break;
	case 0x41:	DeleteFile(regs);			break;
	case 0x42:	SetCurrentFilePosition(regs);		break;
	case 0x43:	DosCall43h(regs);			break;
	case 0x44:	DosCall44h(regs);			break;
	case 0x46:	ForceDuplicateFileHandle(regs);		break;
	case 0x48:	AllocateMemory(regs);			break;
	case 0x49:	FreeMemory(regs);			break;
	case 0x4a:	ResizeMemoryBlock(regs);		break;
	case 0x4b:	LoadAndOrExecuteProgram(regs);		break;
	case 0x4c:	TerminateWithReturnCode(regs);		break;
	case 0x4d:	GetReturnCode(regs);			break;
	case 0x4e:	FindFirstMatchingFile(regs);		break;
	default:
		fprintf(stdout, "int21(AH=%02x): not implemented.\n", REG_AH);
		exit(1);
		break;
	}
}
