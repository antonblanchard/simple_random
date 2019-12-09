#include <stdint.h>

void init_console(void);
void *init_testcase(unsigned long max_insns);
void *init_memory(void);
void execute_testcase(void *insn, void *gprs);
void putchar_unbuffered(const char c);
char getchar_unbuffered(void);

#define MEMPAGE_BASE (64*1024)
#define MEMPAGE_SIZE (64*1024)

#define INSNS_BASE (64*1024)

#define MEM_BASE (112*1024)
#define MEM_SIZE 64
