#include <string.h>

void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d, *s;

	d = dest;
	s = (void *)src;

	while (n--)
		*d++ = *s++;

	return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
	/* Do the buffers overlap in a bad way? */
	if (src < dest && src + n >= dest) {
		char *cdest;
		const char *csrc;
		int i;

		/* Copy from end to start */
		cdest = dest + n - 1;
		csrc = src + n - 1;
		for (i = 0; i < n; i++)

			*cdest-- = *csrc--;
		return dest;
	} else {
		/* Normal copy is possible */
		return memcpy(dest, src, n);
	}
}

void *memset(void *s, int c, size_t n)
{
	unsigned char *x = s;

	while (n--)
		*x++ = c;

	return s;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}

	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dst, const char *src)
{
	char *ptr = dst;

	do {
		*ptr++ = *src;
	} while (*src++ != 0);

	return dst;
}

size_t strlen(const char *s)
{
	size_t len = 0;

	while (*s++)
		len++;

	return len;
}

static int compare(const char *a, const char *b)
{
	while (*a && *b) {
		if (*a != *b)
			return 0;

		a++;
		b++;
	}

	return *b == '\0';
}

char *strstr(const char *haystack, const char *needle)
{
	if (*needle == 0x0 && *haystack != 0x0)
		return (char *)haystack;

	while (*haystack != 0x0) {
		if ((*haystack == *needle) && compare(haystack, needle))
			return (char *)haystack;
		haystack++;
	}

	return NULL;
}
