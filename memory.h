#ifndef __MEMORY_H__
#define __MEMORY_H__

#define	cpu_readop(adr)		memory_read(adr)
#define	cpu_readop_arg(adr)	memory_read(adr)
#define	cpu_readport(port)	port_read(port)
#define	cpu_writeport(port, v)	port_write(port, v)
#define	cpu_readmem20(adr)	memory_read(adr)
#define cpu_writemem20(adr, v)	memory_write(adr, v)

int memory_alloc(size_t size); /* seg size */
int memory_realloc(int seg, size_t size); /* seg size */
int memory_free(int seg);

#ifdef _DEBUG
int memory_read(int adr);
void memory_write(int adr, int v);
#else /* !_DEBUG */
extern unsigned char *memory;
#define	memory_read(adr)	memory[adr]
#define memory_write(adr, v)	(memory[adr] = v)
#endif /* !_DEBUG */

int port_read(int port);
void port_write(int port, int v);

#endif /* __MEMORY_H__ */

