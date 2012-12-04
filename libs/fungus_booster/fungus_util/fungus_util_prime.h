#ifndef FUNGUSUTIL_PRIME_H
#define FUNGUSUTIL_PRIME_H

#include "fungus_util_common.h"

namespace fungus_util
{
    // http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
    static constexpr uint32_t primes[] =
    {
        5,            13,            23,
        47,           97,            193,
        389,          769,           1543,
        3079,         6151,          12289,
        24593,        49157,         98317,
        196613,       393241,        786433,
        1572869,      3145739,       6291469,
        12582917,     25165843,      50331653,
        100663319,    201326611,     402653189,
        805306457,    1610612741,    0
    };

#ifdef FUNGUSUTIL_CPP11_PARTIAL
    static inline constexpr
    uint32_t _recursive_next_prime(uint32_t v, size_t i)
    {
        return primes[i] == 0 ?
               primes[--i] :
               (v > primes[i] ?
                    _recursive_next_prime(v, ++i) :
                    primes[i]);
    }

    static inline constexpr
    uint32_t next_prime(uint32_t v)
    {
        return _recursive_next_prime(v, 0);
    }
#else
    static inline uint32_t next_prime(uint32_t v)
    {
        size_t i = 0;
        for (; v > primes[i]; ++i)
        {
            if (primes[i] == 0)
            {
                --i;
                break;
            }
        }

        return primes[i];
    }
#endif
}

#endif
