#include <stdint.h>
#include <string.h>
#include "backend.h"
#include "uart.h"

void init_console(void)
{
	potato_uart_init();
}

#define ALIGN_UP(VAL, SIZE)	(((VAL) + ((SIZE)-1)) & ~((SIZE)-1))

#define INSNS_START (32*1024)

void *init_testcase(unsigned long max_insns)
{
	return (void *)INSNS_START;
}

void *init_memory(void)
{
	return (void *)MEM_BASE;
}

typedef uint64_t (*testfunc)(void *gprs);

void execute_testcase(void *insns, void *gprs)
{
	testfunc func;

	func = (testfunc)insns;
	func(gprs);
}
