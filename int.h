#ifndef __INT_H__
#define	__INT_H__

#ifndef __MEMORY_H__
#include "memory.h"
#endif /* __MEMORY_H__ */

#ifndef __I86INFO_H__
#include "i86info.h"
#endif /* __I86INFO_H__ */

extern unsigned char *memory;

#define	REG_H(r)	(regs->regs.b[r * 2 + 1])
#define	REG_L(r)	(regs->regs.b[r * 2 + 0])
#define	REG_X(r)	(regs->regs.w[r])
#define	REG_S(r)	(regs->sregs[r])
#define	REG_AH		REG_H(AX)
#define	REG_AL		REG_L(AX)
#define	REG_BH		REG_H(BX)
#define	REG_BL		REG_L(BX)
#define	REG_CH		REG_H(CX)
#define	REG_CL		REG_L(CX)
#define	REG_DH		REG_H(DX)
#define	REG_DL		REG_L(DX)

#define	REG_AX		REG_X(AX)
#define	REG_BX		REG_X(BX)
#define	REG_CX		REG_X(CX)
#define	REG_DX		REG_X(DX)
#define	REG_SP		REG_X(SP)
#define	REG_BP		REG_X(BP)
#define	REG_SI		REG_X(SI)
#define	REG_DI		REG_X(DI)

#define	REG_ES		REG_S(ES)
#define	REG_CS		REG_S(CS)
#define	REG_SS		REG_S(SS)
#define	REG_DS		REG_S(DS)

#define	MEM_FLAG	(*(unsigned short *)&memory[WORD(REG_SS) * 16 + WORD(REG_SP) + 4])
#define	MEM_CS		(*(unsigned short *)&memory[WORD(REG_SS) * 16 + WORD(REG_SP) + 2])
#define	MEM_IP		(*(unsigned short *)&memory[WORD(REG_SS) * 16 + WORD(REG_SP) + 0])
#define	SET_CARRY	(MEM_FLAG |= WORD(0x0001))
#define	RESET_CARRY	(MEM_FLAG &= WORD(0xfffe))

#ifdef LSB_FIRST
#define WORD(w)	((w) & 0xffff)
#define LONG(l) ((l) % 0xffffffff)
#else /* !LSB_FIRST */
#define	WORD(w)	((((w) << 8) & 0xff00) | (((w) >> 8) & 0x00ff)))
#define LONG(l) ((((l) << 24) & 0xff000000) | (((l) << 8) & 0x00ff0000) | (((l) >> 8) & 0x0000ff00) | (((l) >> 24) & 0x000000ff))
#endif /* !LSB_FIRST */

#define	MEMORY(adr)	(adr & regs->amask)
#ifndef _DEBUG
#define printf
#endif /* _DEBUG */

#endif	__INT_H__

