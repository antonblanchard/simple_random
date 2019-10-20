#include <stdint.h>

void init_console(void);
void *init_testcase(unsigned long max_insns);
void *init_memory(void);
void execute_testcase(void *insn, void *gprs);
void putchar_unbuffered(const char c);
char getchar_unbuffered(void);

#define MEM_BASE (96*1024)
#define MEM_BASE_ALIGNED (64*1024)
#define MEM_SIZE 64
