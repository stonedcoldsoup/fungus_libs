#ifndef FUNGUSUTIL_MAKE_STRING_H
#define FUNGUSUTIL_MAKE_STRING_H

#include "fungus_util_common.h"

#ifdef FUNGUSUTIL_CPP11_PARTIAL
#include <sstream>

namespace fungus_util
{
    FUNGUSUTIL_ALWAYS_INLINE static inline void __make_string_impl(std::stringstream &ss) {}

    template <typename T, typename... U>
    FUNGUSUTIL_ALWAYS_INLINE static inline void __make_string_impl(std::stringstream &ss, T &&v, U&&... w)
    {
        ss << v;
        __make_string_impl(ss, std::forward<U>(w)...);
    }

    template <typename... T>
    FUNGUSUTIL_ALWAYS_INLINE inline std::string make_string(T&&... v)
    {
        std::stringstream ss;

        __make_string_impl(ss, std::forward<T>(v)...);
        return ss.str();
    }

    FUNGUSUTIL_ALWAYS_INLINE static inline bool __string_extract_impl(std::stringstream &ss) {return true;}

    template <typename T, typename... U>
    FUNGUSUTIL_ALWAYS_INLINE static inline bool __string_extract_impl(std::stringstream &ss, T &v, U&... w)
    {
        ss >> v;
        return ss.eof() ?
               false    :
               __string_extract_impl(ss, std::forward<U>(w)...);
    }

    template <typename... T>
    FUNGUSUTIL_ALWAYS_INLINE inline bool string_extract(const std::string &str, T&&... v)
    {
        std::stringstream ss(str);
        return __string_extract_impl(ss, std::forward<T>(v)...);
    }
}

#endif

#endif
