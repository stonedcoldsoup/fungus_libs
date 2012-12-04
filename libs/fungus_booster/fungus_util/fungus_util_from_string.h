#ifndef FUNGUSUTIL_FROM_STRING_H
#define FUNGUSUTIL_FROM_STRING_H

#include "fungus_util_common.h"
#include <sstream>

namespace fungus_util
{
    namespace detail
    {
        template <typename... T>
        struct from_string_impl;

        template <typename T, typename... U>
        struct from_string_impl<T, U...>
        {
            static inline size_t __impl(std::stringstream &m_ss, size_t ct, T &v, U&... w)
            {
                if (!m_ss)
                    return ct;

                m_ss >> v;
                return from_string_impl<U...>::__impl(m_ss, ct + 1, w...);
            }
        };

        template <>
        struct from_string_impl<>
        {
            static inline size_t __impl(std::stringstream &m_ss, size_t ct) {return ct;}
        };
    }

    template <typename... T>
    static inline size_t from_string(const std::string &m_s, T&... v)
    {
        std::stringstream m_ss;
        m_ss << m_s;
        return detail::from_string_impl<T...>::__impl(m_ss, 0, v...);
    }
};

#endif
