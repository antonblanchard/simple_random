#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "backend.h"
#include "console.h"

void init_console(void)
{
	console_init();
}

void *init_testcase(unsigned long max_insns)
{
	return (void *)INSNS_BASE;
}

void *init_memory(void)
{
	return (void *)MEM_BASE;
}

typedef uint64_t (*testfunc)(void *gprs);

long execute_testcase(void *insns, void *gprs, void *mem_ptr)
{
	testfunc func;
	long tb_start, tb_end;
	int dummy;

	memset(mem_ptr, 0, MEM_SIZE);
	func = (testfunc)insns;
	asm volatile("stwcx. %1,0,%0" : : "r" (&dummy), "r" (0));
	asm volatile("mfspr %0,268" : "=r" (tb_start));
	func(gprs);
	asm volatile("mfspr %0,268" : "=r" (tb_end));
	return tb_end - tb_start;
}

char getchar_unbuffered(void)
{
	return getchar();
}

void putchar_unbuffered(const char c)
{
	putchar(c);
}
