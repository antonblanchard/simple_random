#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <assert.h>
#include <sys/mman.h>
#include "backend.h"

void init_console(void)
{
}

#define ALIGN_UP(VAL, SIZE)	(((VAL) + ((SIZE)-1)) & ~((SIZE)-1))

void *init_testcase(unsigned long max_insns)
{
	void *p;

	p = mmap((void *)MEMPAGE_BASE, MEMPAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
		 MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);

	if (p == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	memset(p, 0, MEMPAGE_SIZE);

	return (void *)INSNS_BASE;
}

void *init_memory(void)
{
	return (void *)MEM_BASE;
}

typedef uint64_t (*testfunc)(void *gprs);

long execute_testcase(void *insns, void *gprs)
{
	testfunc func;
	unsigned long r1_before = 0, r1_after = 0;
	unsigned long r13_before = 0, r13_after = 0;
	long tb_start, tb_end;

	asm volatile("mr %0,1" : : "r" (r1_before));
	asm volatile("mr %0,13" : : "r" (r13_before));

	func = (testfunc)insns;
	asm volatile("mfspr %0,268" : "=r" (tb_start));
	func(gprs);
	asm volatile("mfspr %0,268" : "=r" (tb_end));

	asm volatile("mr %0,1" : : "r" (r1_after));
	asm volatile("mr %0,13" : : "r" (r13_after));

	assert(r13_before == r13_after);
	assert(r1_before == r1_after);

	return tb_end - tb_start;
}

void putchar_unbuffered(const char c)
{
	putchar(c);
}

char getchar_unbuffered(void)
{
	struct termios oldt, newt;
	int ch;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return ch;
}
