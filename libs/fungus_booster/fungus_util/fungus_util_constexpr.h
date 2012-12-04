#ifndef FUNGUSUTIL_CONSTEXPR_H
#define FUNGUSUTIL_CONSTEXPR_H

namespace fungus_util
{
    template <typename T>
    inline constexpr size_t constexpr_sizeof() {return sizeof(T);}
}

#endif
