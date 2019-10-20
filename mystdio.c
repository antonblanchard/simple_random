#include <string.h>
#include <stdint.h>
#include "backend.h"
#include "stdio.h"

void puthex(uint64_t n)
{
	for (long i = 15; i >= 0; i--) {
		uint8_t x = (n >> (i*4)) & 0xf;

		if (x > 9)
			putchar_unbuffered(x-10+'a');
		else
			putchar_unbuffered(x+'0');
	}
}

void putlong(uint64_t n)
{
	char str[32];
	unsigned long i = 0;

	if (!n) {
		putchar_unbuffered('0');
		return;
	}

	while (n != 0) {
		int rem = n % 10;

		str[i++] = rem + '0';
		n = n / 10;
	}

	while (i--)
		putchar_unbuffered(str[i]);
}

void print(const char *str)
{
	unsigned long len = strlen(str);

	while (len--)
		putchar_unbuffered(*str++);
}


