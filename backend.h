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

#ifndef VECTOR_INSNS
#define VECTOR_INSNS	false
#endif

#ifdef __powerpc64__
#define SIXTYFOUR_INSNS	true
#else
#define SIXTYFOUR_INSNS	false
#endif

#if VECTOR_INSNS
#define NGPRS	(36+128)
#else
#define NGPRS	(36)
#endif
