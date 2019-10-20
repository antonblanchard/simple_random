#include <stdint.h>

static long int __labs(long int n)
{
        return (n > 0) ? n : -n;
}

uint64_t __udivdi3(uint64_t numerator, uint64_t denominator)
{
        __int128 R;
        __int128 D;
        uint64_t q = 0;

        R = numerator;
        D = ((__int128)denominator) << 64;

        for (long i = 63; i >= 0; i--) {
                R = 2 * R - D;

                if (R >= 0)
                        q |= (1UL << i);
                else
                        R = R + D;
        }

        return q;
}

uint64_t __umoddi3(uint64_t numerator, uint64_t denominator)
{
        uint64_t q = __udivdi3(numerator, denominator);

        return numerator - (q * denominator);
}

int64_t __divdi3(int64_t numerator, int64_t denominator)
{
        int64_t q = __udivdi3(__labs(numerator), __labs(denominator));

        if ((numerator < 0) != (denominator < 0))
                q = -q;

        return q;
}

int64_t __moddi3(int64_t numerator, int64_t denominator)
{
        int64_t q = __divdi3(numerator, denominator);

        return numerator - (q * denominator);
}
