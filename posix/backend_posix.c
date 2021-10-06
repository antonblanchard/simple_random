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

long execute_testcase_once(void *insns, void *gprs, void *mem_ptr)
{
	testfunc func;
	unsigned long r1_before = 0, r1_after = 0;
	unsigned long r13_before = 0, r13_after = 0;
	long tb_start, tb_end;
	int dummy;

	memset(mem_ptr, 0, MEM_SIZE);
	asm volatile("stwcx. %1,0,%0" : : "r" (&dummy), "r" (0));

	asm volatile("mr %0,1" : "=r" (r1_before));
	asm volatile("mr %0,13" : "=r" (r13_before));

	func = (testfunc)insns;
	asm volatile("mfspr %0,268" : "=r" (tb_start));
	func(gprs);
	asm volatile("mfspr %0,268" : "=r" (tb_end));

	asm volatile("mr %0,1" : "=r" (r1_after));
	asm volatile("mr %0,13" : "=r" (r13_after));

	assert(r13_before == r13_after);
	assert(r1_before == r1_after);

	return tb_end - tb_start;
}

#define NTRIES	5

long execute_testcase(void *insns, void *gprs, void *mem_ptr)
{
	long int i, j;
	unsigned long results[NTRIES][NGPRS];
	unsigned long count[NTRIES];
	long int tbdiff, tbd[NTRIES];
	long int nc = 0;

	for (j = 0; j < NTRIES; ++j) {
		count[j] = 0;
		tbd[j] = 0;
	}
	for (i = 0; i < NTRIES; ++i) {
		tbdiff = execute_testcase_once(insns, gprs, mem_ptr);
		((unsigned long *)gprs)[31] = 0;
		for (j = 0; j < nc; ++j)
			if (memcmp(gprs, results[j], NGPRS * sizeof(unsigned long)) == 0)
				break;
		++count[j];
		if (j >= nc) {
			memcpy(results[j], gprs, NGPRS * sizeof(unsigned long));
			tbd[j] = tbdiff;
			++nc;
		}
	}
	/* Pick the most popular answer. */
	i = 0;
	memcpy(gprs, results[0], NGPRS * sizeof(unsigned long));
	tbdiff = tbd[0];
	for (j = 1; j < nc; ++j) {
		if (count[j] > count[i]) {
			i = j;
			memcpy(gprs, results[j], NGPRS * sizeof(unsigned long));
			tbdiff = tbd[j];
		}
	}
	return tbdiff;
}

void putchar_unbuffered(const char c)
{
	putchar(c);
}

int getchar_unbuffered(void)
{
	struct termios oldt, newt;
	int ch;

	fflush(stdout);
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	return ch;
}
