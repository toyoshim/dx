#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "process.h"
#include "int.h"
#include "dos.h"
#include "file.h"
#ifdef _DEBUG
#include "debug.h"
#endif /* _DEBUG */

extern void int_hook(int no, void *regs);

static process_info *current_process = NULL;

process_info *
process_open(const char *name, int env_seg, const unsigned char *cmd)
{
	int i;
	int nreloc;
	int reloc_size;
	int fd;
	int fsize;
	struct exehdr h;
	int psp_seg_size = 0x100 / 16;
	int head_size;
	int text_size;
	int min_memory; /* in seg */
	int max_memory; /* in seg */
	process_info *proc;
	unsigned char *psp;
	struct reloc_entry *tbl, *rp;
	int exeflag = 0;

	int name_len = strlen(name);
	char *_name = malloc(name_len + 4 + 1);
	if (NULL == name) {
		puts("dx: can not allocate memory");
		return NULL;
	}
	strcpy(_name, name);
	if ((0 != strcasecmp(&name[name_len - 4], ".com")) &&
		(0 != strcasecmp(&name[name_len - 4], ".exe"))) {
		struct stat st;
		if (stat(name, &st) < 0) {
			strcpy(&_name[name_len], ".com");
			name_len += 4;
		}
	}
	fd = open(_name, O_RDONLY);
	if (fd < 0) {
		if (0 == strcasecmp(&_name[name_len - 4], ".com")) strcpy(&_name[name_len - 4], ".exe");
		else if (0 == strcasecmp(&_name[name_len - 4], ".exe")) strcpy(&_name[name_len - 4], ".com");
		fd = open(_name, O_RDONLY);
		if (fd < 0) {
			free(_name);
			return NULL;
		}
	}

	proc = malloc(sizeof(process_info));
	if (NULL == proc) return NULL;
	fsize = read(fd, &h, sizeof(h));
	if (fsize != sizeof(h)) {
		close(fd);
		free(proc);
		free(_name);
		return NULL;
	}
	head_size = WORD(h.hdr_size) * 16;
	text_size = (WORD(h.size) - 1) * 512 +
		WORD(h.bytes_on_last_page) - head_size;
	min_memory = WORD(h.min_memory) + (text_size + 15) / 16;
	max_memory = WORD(h.max_memory) + (text_size + 15) / 16;
	exeflag = (WORD(h.magic) == WORD('ZM'))? 1: 0;
#ifdef _DEBUG
	if (exeflag) {
		printf("filename: %s\n", _name);
		printf("\n");
		printf("magic: %04x\n", WORD(h.magic));
		printf("bytes_on_last_page: %04x\n", WORD(h.bytes_on_last_page));
		printf("size: %04x\n", WORD(h.size));
		printf("nreloc: %04x\n", WORD(h.nreloc));
		printf("hdr_size: %04x\n", WORD(h.hdr_size));
		printf("min_memory: %04x\n", WORD(h.min_memory));
		printf("max_memory: %04x\n", WORD(h.max_memory));
		printf("init_ss: %04x\n", WORD(h.init_ss));
		printf("init_sp: %04x\n", WORD(h.init_sp));
		printf("checksum: %04x\n", WORD(h.checksum));
		printf("init_ip: %04x\n", WORD(h.init_ip));
		printf("init_cs: %04x\n", WORD(h.init_cs));
		printf("reloc_offset: %04x\n", WORD(h.reloc_offset));
		printf("overlay_num: %04x\n", WORD(h.overlay_num));
		printf("\n");
		printf("text_size: %08x\n", text_size);
		printf("min_memory: %08x\n", min_memory);
		printf("max_memory: %08x\n", max_memory);
		printf("\n");
	}
#endif	/* _DEBUG */

	if (!exeflag) {
		/* not EXE */
		text_size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		max_memory = (text_size + 15) / 16 + psp_seg_size;
		psp_seg_size = 0;
		h.nreloc = 0;
		h.init_ss = h.init_sp = h.init_cs = WORD(0);
		h.init_ip = WORD(0x100);
		if (max_memory < 0x0fff) max_memory = 0x0fff;
	}

	proc->psp_seg = memory_alloc(psp_seg_size + max_memory);
	if (proc->psp_seg < 0) {
		close(fd);
		free(proc);
		return NULL;
	}
	proc->parent = NULL;
	proc->text_seg = proc->psp_seg + psp_seg_size;
	proc->env_seg = env_seg;
	proc->dta_seg = proc->psp_seg;
	proc->dta_adr = 0x80;

	if (exeflag) {
		if ((head_size != lseek(fd, head_size, SEEK_SET)) ||
			(text_size != read(fd, &memory[proc->text_seg * 16], text_size))) {
			fprintf(stderr, "command can not loaded\n");
			memory_free(proc->psp_seg);
			close(fd);
			free(proc);
			return NULL;
		}
	} else {
		if (text_size != read(fd, &memory[proc->text_seg * 16 + 0x100], text_size)) {
			fprintf(stderr, "command can not loaded\n");
			memory_free(proc->psp_seg);
			close(fd);
			free(proc);
			return NULL;
		}
	}

	if (h.nreloc) {
		nreloc = WORD(h.nreloc);
		reloc_size = nreloc * sizeof(struct reloc_entry);
		tbl = rp = malloc(reloc_size);
		if ((NULL == rp) ||
			(WORD(h.reloc_offset) != lseek(fd, WORD(h.reloc_offset), SEEK_SET)) ||
			(reloc_size != read(fd, rp, reloc_size))) {
			memory_free(proc->psp_seg);
			close(fd);
			free(proc);
			return NULL;
		}
		for (i = 0; i < nreloc; i++, rp++) {
#ifdef _DEBUG
			printf("patch: %08x += %04x\n", rp->seg * 16 + rp->off, proc->text_seg);
#endif /* _DEBUG */
			*(unsigned short *)&memory[(proc->text_seg + rp->seg) * 16 + rp->off] += WORD(proc->text_seg);
		}
#ifdef _DEBUG
		printf("\n");
#endif /* _DEBUG */
		free(tbl);
	}
	close(fd);

	/* psp */
	/* TODO: not complete */
	psp = (unsigned char *)&memory[proc->psp_seg * 16];
	memset(psp, 0, 0x100);
	psp[0] = 0xcd; /* INT */
	psp[1] = 0x20; /*  21 */
	*(unsigned short *)&psp[0x2c] = WORD(proc->env_seg);
	memcpy(&memory[proc->psp_seg * 16 + 0x80], cmd, cmd[0] + 2);

	/* regs */
	memset(&proc->regs, 0, sizeof(i86_Regs));
	proc->regs.sregs[SS] = WORD(WORD(h.init_ss) + proc->text_seg);
	proc->regs.regs.w[SP] = WORD(WORD(h.init_sp));
	proc->regs.ip = WORD(WORD(h.init_ip));
	proc->regs.sregs[ES] = WORD(proc->psp_seg);
	proc->regs.sregs[CS] = WORD(WORD(h.init_cs) + proc->text_seg);
	proc->regs.sregs[DS] = WORD(proc->psp_seg);
	proc->regs.amask = 0xfffff;
	proc->regs.int_callback = int_hook;

#ifdef _DEBUG
	if (exeflag) debug_break_add(0, proc->text_seg * 16);
	else debug_break_add(0, proc->text_seg * 16 + 0x100);
#endif /* _DEBUG */

	free(_name);
	
	return proc;
}

int
process_resume(process_info *pi)
{
	i86_reset(NULL);
	i86_set_context(&pi->regs);
	current_process = pi;
	while (i86_execute(1000));
	return 0;
}

int
process_suspend(process_info *pi)
{
	i86_get_context(&pi->regs);
	return 0;
}

void
process_close(process_info *pi)
{
	memory_free(pi->psp_seg);
	free(pi);
}

process_info *
process_get_current(void)
{
	return current_process;
}

