#ifndef __PROCESS_H__
#define	__PROCESS_H__

#ifndef __I86INFO_H__
#include "i86info.h"
#endif /* __I86INFO_H__ */

typedef struct _process_info {
	struct _process_info *parent;
	i86_Regs regs;
	int psp_seg;
	int text_seg;	/* psp + 0x10 */
	int env_seg;
	int dta_seg;
	int dta_adr;
} process_info;

process_info *process_open(const char *name, int env_seg, const unsigned char *cmd);
int process_resume(process_info *pi);
int process_suspend(process_info *pi);
void process_close(process_info *pi);
process_info *process_get_current(void);

#endif /* __PROCESS_H__ */

