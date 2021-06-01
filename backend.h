#include <stdint.h>

void init_console(void);
void *init_testcase(unsigned long max_insns);
void *init_memory(void);
long execute_testcase(void *insn, void *gprs, void *mem);
void putchar_unbuffered(const char c);
int getchar_unbuffered(void);

#define MEMPAGE_BASE (64*1024)
#define MEMPAGE_SIZE (64*1024)

#define INSNS_BASE (64*1024)

#define MEM_BASE (112*1024)
#define MEM_SIZE 1024

#define NGPRS	(38+128)
