#ifndef __I86INFO_H__
#define __I86INFO_H__

typedef union
{
	unsigned short w[8];
	unsigned char b[16];
} i86basicregs;

typedef struct
{
	i86basicregs regs;
	int amask;
	int ip;
	unsigned short flags;
	unsigned long base[4];
	unsigned short sregs[4];
	int (*irq_callback)(int irqline);
	int AuxVal, OverVal, SignVal, ZeroVal, CarryVal, ParityVal;
	unsigned char TF, IF, DF;
	unsigned char int_vector;
	unsigned char pending_irq;
	signed char nmi_state;
	signed char irq_state;
	void (*int_callback)(int no, void *regs);
} i86_Regs;

enum { ES, CS, SS, DS, };
enum { AX, CX, DX, BX, SP, BP, SI, DI };

#endif /* __I86INFO_H__ */
