#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "process.h"

#define	ERR_MEMORY_CONTROL_BLOCK_DESTROYED	-7
#define	ERR_INSUFFICIENT_MEMORY			-8
#define	ERR_MEMORY_BLOCK_ADDRESS_INVALID	-9

typedef struct {
	off_t offset; /* not seg */
	size_t size; /* not seg */
	size_t used_size; /* not seg */
	void *next;
} mlist;

unsigned char *memory = NULL;
unsigned long text_top = 0;
int memory_size = 0;
mlist *list = NULL;

int
memory_alloc(size_t size) /* seg. size */
{
	mlist *nlist;
	if (NULL != list) {
		if (list->size != list->used_size) {
			memory_size -= list->size - list->used_size;
			list->size = list->used_size;
		}
	}
	nlist = malloc(sizeof(mlist));
	if (NULL == nlist) {
		puts("dx: can not allocate memory");
		return ERR_INSUFFICIENT_MEMORY;
	}
	nlist->offset = memory_size;
	nlist->size = size * 16;
	nlist->used_size = size * 16;
	nlist->next = list;
	memory_size += size * 16;
	if (NULL == memory) {
		memory = malloc(memory_size);
		text_top = memory_size + 0x100;
	} else {
		memory = realloc(memory, memory_size);
	}
	if (NULL == memory) {
		puts("dx: can not allocate memory");
		return ERR_INSUFFICIENT_MEMORY;
	}
#ifdef _DEBUG
	printf("internal memory expanded to $%08x\n", memory_size);
#endif /* _DEBUG */
	list = nlist;
	return list->offset / 16;
}

int
memory_realloc(int seg, size_t size) /* seg. size */
{
	mlist *p;
	seg *= 16;
	for (p = list; NULL != p; p = p->next) {
		if (seg != p->offset) continue;
		if (p->size < size * 16) {
			puts("dx: can not expand memory block");
			return ERR_INSUFFICIENT_MEMORY;
		} else {
			p->used_size = size * 16;
			return 0;
		}
	}
	puts("dx: invalid memory block");
	return ERR_MEMORY_BLOCK_ADDRESS_INVALID;
}

int
memory_free(int seg)
{
	return 0;
}

#ifdef _DEBUG

int
memory_read(int adr)
{
	if (adr >= 0xa0000) printf("read:\t$%08x => $%02x(%c)\n", adr, (int)memory[adr], (int)memory[adr]);
/*
	if ((adr >= 0x800) && (adr < (process_get_current()->psp_seg * 16 + 0x100)))
	printf("read:\t$%08x => $%02x(%c)\n", adr, (int)memory[adr], (int)memory[adr]);
/**/
	return memory[adr];
}

void
memory_write(int adr, int v)
{
	if (adr >= 0xa0000) printf("write:\t$%08x <= $%02x(%c)\n", adr, ((int)v) & 0xff, v);
/*
	printf("write:\t$%08x <= $%02x(%c)\n", adr, ((int)v) & 0xff, v);
/**/
	memory[adr] = (unsigned char)v;
}

#endif /* _DEBUG */

int
port_read(int port)
{
	printf("port read:\t$%08x => $%02x\n", port, 0);
/**/
	return 0;
}

void
port_write(int port, int v)
{
	printf("port write:\t$%08x <= $%02x\n", port, ((int)v) & 0xff);
/**/
}

int
change_pc20(int adr)
{
/*
	printf("change pc:\t$%08x\n", adr);
/**/
	return 0;
}

