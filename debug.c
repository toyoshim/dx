#include <stdio.h>
#include <stdlib.h>
#include "i86intrf.h"
#include "int.h"

#define	MAX_BP	16
int debug_flag = 1;
static int step_flag = 0;
unsigned long break_point[MAX_BP];

static void help(void)
{
	puts("h,?		show this message");
	puts("sb		show breakpoints");
	puts("b n,[addr]	set break point");
	puts("s,t		stepped execue");
	puts("g,r		execute");
	puts("l [addr]:[len]	show disasm list");
	puts("md [addr]:[len]	memory dump");
	puts("q,exit		exit");
	puts("");
}

static void show_bp(void)
{
	int i;
	for (i = 0; i < MAX_BP; i++) {
		if (break_point[i] == 0) continue;
		printf("%3d: %08x\n", i, break_point[i]);
	}
}

static void show_regs(void)
{
	i86_Regs r;
	i86_Regs *regs = &r;
	i86_get_context(regs);
	printf("AX:%04x\tCX:%04x\tDX:%04x\tBX:%04x\n",
		WORD(REG_AX), WORD(REG_CX), WORD(REG_DX), WORD(REG_BX));
	printf("SP:%04x\tBP:%04x\tSI:%04x\tDI:%04x\n",
		WORD(REG_SP), WORD(REG_BP), WORD(REG_SI), WORD(REG_DI));
	printf("ES:%04x\tCS:%04x\tSS:%04x\tDS:%04x\n",
		WORD(REG_ES), WORD(REG_CS), WORD(REG_SS), WORD(REG_DS));
	printf("IP:%04x\t",
		WORD(regs->ip));
	printf("FLAGS:____%c%c%c%c%c%c_%c_%c_%c\n",
		regs->OverVal? 'O': 'o',
		(WORD(regs->flags) & 0x40)? 'D': 'd',
		(WORD(regs->flags) & 0x20)? 'I': 'i',
		(WORD(regs->flags) & 0x10)? 'T': 't',
		regs->SignVal? 'S': 's',
		regs->ZeroVal? 'Z': 'z',
		regs->AuxVal? 'A': 'a',
		regs->ParityVal? 'P': 'p',
		regs->CarryVal? 'C': 'c');
	printf("\n");
}

static void disasm(unsigned long pc, int len)
{
	char buf[1024];
	unsigned long d;
	int i;
	for (i = 0; i < len; i += d) {
		d = DasmI86(buf, pc + i);
		printf("%08x: %s\n", pc + i, buf);
	}
	printf("\n");
}

static void memdmp(unsigned long pc, int len)
{
	int n = len / 16;
	int m = len % 16;
	int i;
	int j;
	for (i = 0; i < n; i++) {
		int max = ((i + 1) == n)? m: 16;
		int offset = pc + 16 * i;
		if (max == 0) break;
		printf("%08x: ", offset);
		for (j = 0; j < max; j++) printf("%02x ", memory[offset + j]);
		printf("| ");
		for (; j < 16; j++) printf("   ");
		for (j = 0; j < max; j++) {
			unsigned char code = memory[offset + j];
			printf("%c", iscntrl(code)? '?': code);
		}
		printf("\n");
	}
	printf("\n");
}

static void break_main(unsigned long pc)
{
	unsigned long lp = pc;
	unsigned long mp = pc;
	static char cmd_cache[256] = {0};
	char cmd[256];
	show_regs();
	disasm(pc, 1);
	step_flag = 0;
	for (;;) {
		printf("> ");
		fflush(stdout);
		fgets(cmd, 256, stdin);
		if ('\n' == *cmd) memcpy(cmd, cmd_cache, 256);
		else memcpy(cmd_cache, cmd, 256);
		switch (cmd[0] | 0x20) {
		case 'h':
		case '?':
			help();
			break;
		case 'b':{
			int n;
			unsigned long adr = 0;
			if (cmd[1] == ' ') {
				sscanf(&cmd[2], "%d,%x", &n, &adr);
				if (n < MAX_BP) break_point[n] = adr;
			}
			break;}
		case 's':
		case 't':
			if (cmd[1] == 'b') show_bp();
			else {
				step_flag = 1;
				return;
			}
			break;
		case 'g':
		case 'r':
			return;
		case 'q':
		case 'e':
			exit(0);
		case 'l':{
			int len = 16;
			if (cmd[1] == ' ') {
				if (cmd[2] == ':') sscanf(&cmd[3], "%x", &len);
				else {
					sscanf(&cmd[2], "%x:%x", &lp, &len);
					snprintf(cmd_cache, 256, "l %x", len);
				}
			}
			disasm(lp, len);
			lp += len;
			break;}
		case 'm':{
			int len = 128;
			char *_cmd = cmd;
			if (cmd[1] == 'd') _cmd++;
			if (_cmd[1] == ' ') {
				if (_cmd[2] == ':') sscanf(&_cmd[3], "%x", &len);
				else {
					sscanf(&_cmd[2], "%x:%x", &mp, &len);
					snprintf(cmd_cache, 256, "m :%x", len);
				}
			}
			memdmp(mp, len);
			mp += len;
			break;}
		default:
			puts("unknown command");
		}
	}
}

void debug_init(void)
{
	int i;
	for (i = 0; i < MAX_BP; i++) break_point[i];
}

void debug_exit(void)
{
}

void debug_main(void)
{
	int i;
	unsigned long pc = LONG(i86_get_pc());
	if (step_flag) {
		printf("*step* ");
		break_main(pc);
		return;
	}
	for (i = 0; i < MAX_BP; i++) {
		if (break_point[i] != pc) continue;
		break_main(pc);
		return;
	}
}

int debug_break_add(int no, unsigned long addr)
{
	if (no >= MAX_BP) return 1;
	break_point[no] = addr;
	return 0;
}

void debug_memory_dump(int address, int len)
{
	memdmp(address, len);
}

void debug_step_on(void)
{
	step_flag = 1;
}

