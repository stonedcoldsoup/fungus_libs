#ifndef FUNGUSUTIL_POW2
#define FUNGUSUTIL_POW2

#include "fungus_util_common.h"
#include <cstddef>

namespace fungus_util
{
    // nothing fancy or templatey going on here.

    // check if x is a power of two
    static inline
#ifdef FUNGUSUTIL_CPP11_PARTIAL
    constexpr
#endif
    bool is_pow2(const size_t x)
    {
        return (x & (x - 1)) == 0;
    }

    // find the next power of two
#ifdef FUNGUSUTIL_CPP11_PARTIAL
    static inline constexpr size_t _recursive_next_pow2(const size_t x, size_t i)
    {
        return i >= x ? i : _recursive_next_pow2(x, i << 1);
    }

    static inline constexpr size_t next_pow2(const size_t x)
    {
        return _recursive_next_pow2(x, 1);
    }
#else
    static inline size_t next_pow2(const size_t x)
    {
        register int i;
        for (i = 1; i < x; i <<= 1);
        return i;
    }
#endif
}

#endif
