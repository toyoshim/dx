#ifndef __DEBUG_H__
#define	__DEBUG_H__

int debug_flag;

void debug_init(void);
void debug_exit(void);
void debug_main(void);
void debug_memory_dump(int address, int len);
void debug_step_on(void);

#endif /* __DEBUG_H__ */

