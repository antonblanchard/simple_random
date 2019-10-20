extern unsigned long lfsr_taps[];

static inline unsigned long __mylfsr(unsigned long lfsr_tap, unsigned long prev)
{
	unsigned long lsb = prev & 1;

	prev >>= 1;
#if 0
	if (lsb == 1)
		prev ^= lfsr_tap;
#else
	prev ^= (-lsb) & lfsr_tap;
#endif

	return prev;
}

static inline unsigned long mylfsr(unsigned long bits, unsigned long prev)
{
	return __mylfsr(lfsr_taps[bits], prev);
}

unsigned long mylfsr(unsigned long bits, unsigned long prev);
