#ifndef FUNGUSUTIL_COMMON_H
#define FUNGUSUTIL_COMMON_H

#include <cstdlib>
#include <cstring>

#ifdef FUNGUSUTIL_NO_ASSERT
    #define fungus_util_assert(TERM, MSG)  \
    {                                      \
        (TERM);                            \
    }
#else
    #include <cassert>
    #include <iostream>

    #define fungus_util_assert(TERM, MSG)               \
    {                                                   \
        if (!(TERM))                                    \
        {                                               \
            std::cerr << std::string(MSG) << std::endl; \
            assert(false);                              \
        }                                               \
    }
#endif

// FUNGUSUTIL_CPP11_* is depreciated, library now assumes that the C++ version in use
// is always C++11. support for C++98 and ANSI C++ will be removed by October 2012.
#if (__cplusplus > 199711L) || (defined(__STDCXX_VERSION__) && (__STDCXX_VERSION__ >= 201001L))
    #define FUNGUSUTIL_CPP11_FULL
    #define FUNGUSUTIL_CPP11_PARTIAL
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || defined(__GXX_EXPERIMENTAL_CPP0X__)
    #define FUNGUSUTIL_CPP11_PARTIAL
#endif

#ifdef FUNGUSUTIL_CPP11_PARTIAL
    #define FUNGUSUTIL_NO_ASSIGN(T)              \
        T(const T&) = delete;                    \
        T& operator=(const T&) = delete;
#else
    #include <string>
    #define FUNGUSUTIL_NO_ASSIGN(T)                                                                                                         \
        T(const T&) {fungus_util_assert(false, (std::string("Assignment disabled for object type ") + typeid(T).name() + ".").c_str());}    \
        T& operator=(const T&) {fungus_util_assert(false, (std::string("Assignment disabled for object type ") + typeid(T).name() + ".").c_str()); return *this;}
#endif

#ifndef FUNGUSUTIL_PLATFORM_DEFINED
    #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(WIN32)
        #define FUNGUSUTIL_WIN32
    #elif defined(__linux__) || defined(__linux)
        #define FUNGUSUTIL_POSIX
    #endif
    #define FUNGUSUTIL_PLATFORM_DEFINED
#endif

#ifdef DLL_FUNGUSUTIL
    #ifdef BUILD_FUNGUSUTIL
        #define FUNGUSUTIL_API __attribute__ ((dllexport))
    #else
        #define FUNGUSUTIL_API __attribute__ ((dllimport))
    #endif
#elif SO_FUNGUSUTIL
    #ifdef BUILD_FUNGUSUTIL
        #define FUNGUSUTIL_API __attribute__ ((visibility("hidden")))
    #else
        #define FUNGUSUTIL_API
    #endif
#else
    #define FUNGUSUTIL_API
#endif

namespace fungus_util
{
    template <typename T>
    constexpr bool __truth(T __v)
    {
        return (bool)__v;
    }

    struct __constexpr_assert_M {};

    template <bool b_okay, typename M>
    struct __constexpr_assert;

    template <typename M>
    struct __constexpr_assert<true, M> {};

    template <typename M>
    struct __constexpr_assert<false, M>
    {
        typename M::__bogus__ __bogus_thing__;
    };

    template <typename T>
    struct possible
    {
        typedef T type;

        bool b_success;
        T    value;

        possible():
            b_success(false),
            value(T())
        {}

        possible(const T &value):
            b_success(true),
            value(value)
        {}

        possible(T &&value):
            b_success(true),
            value(std::move(value))
        {}

        possible(const possible &m_o):
            b_success(m_o.b_success),
            value(m_o.value)
        {}

        possible(possible &&m_o):
            b_success(m_o.b_success),
            value(std::move(m_o.value))
        {}

        operator T()
        {
            fungus_util_assert(b_success, "possible was casted to type T when b_success was false.");
            return value;
        }

        bool valid() const {return b_success;}
    };

    template <typename T>
    struct possible<const T &>
    {
        typedef const T &type;

        static const T __default;

        bool b_success;
        const T &value;

        possible():
            b_success(false),
            value(__default)
        {}

        possible(const T &value):
            b_success(true),
            value(value)
        {}

        possible(const possible &m_o):
            b_success(m_o.b_success),
            value(m_o.value)
        {}

        possible &operator =(const T &value)
        {
            b_success = true;
            this->value = value;

            return *this;
        }

        possible &operator =(const possible &m_o)
        {
            operator =(m_o.value);
            b_success = m_o.b_success;
            return *this;
        }

        operator const T &()
        {
            fungus_util_assert(b_success, "possible was casted to type T when b_success was false.");
            return value;
        }

        bool valid() const {return b_success;}
    };

    template <typename T>
    struct possible<T &>
    {
        typedef T &type;

        static T __default;

        bool b_success;
        T &value;

        possible():
            b_success(false),
            value(__default)
        {}

        possible(T &value):
            b_success(true),
            value(value)
        {}

        possible(const possible &m_o):
            b_success(m_o.b_success),
            value(m_o.value)
        {}

        possible &operator =(T &value)
        {
            b_success = true;
            this->value = value;

            return *this;
        }

        possible &operator =(const possible &m_o)
        {
            operator =(m_o.value);
            b_success = m_o.b_success;

            return *this;
        }

        operator T &()
        {
            fungus_util_assert(b_success, "possible was casted to type T when b_success was false.");
            return value;
        }

        bool valid() const {return b_success;}
    };

    template <typename T> const T possible<const T &>::__default = T();
    template <typename T>       T possible<T &>::__default       = T();

    namespace detail
    {
        template <size_t _ctr, typename... T>
        struct __count_types_helper;

        template <size_t _ctr, typename T, typename... U>
        struct __count_types_helper<_ctr, T, U...>
        {
            static constexpr size_t __ctr = _ctr;
            static constexpr size_t value = __count_types_helper<__ctr + 1, U...>::value;
        };

        template <size_t _ctr>
        struct __count_types_helper<_ctr>
        {
            static constexpr size_t __ctr = _ctr;
            static constexpr size_t value = __ctr;
        };
    }

    template <typename... T>
    struct count_types:
        detail::__count_types_helper<0, T...>
    {};

    namespace detail
    {
        template <typename... boolT>
        struct __recursive_or_type;

        template <typename boolT, typename... boolU>
        struct __recursive_or_type<boolT, boolU...>
        {
            static inline constexpr bool __impl(const boolT m_a, const boolU... m_b)
            {
                return m_a  ?
                       true :
                       __recursive_or_type<boolU...>::__impl(m_b...);
            }
        };

        template <>
        struct __recursive_or_type<>
        {
            static inline constexpr bool __impl()
            {
                return false;
            }
        };

        template <typename... boolT>
        struct __recursive_and_type;

        template <typename boolT, typename... boolU>
        struct __recursive_and_type<boolT, boolU...>
        {
            static inline constexpr bool __impl(const boolT m_a, const boolU... m_b)
            {
                return m_a && __recursive_or_type<boolU...>::__impl(m_b...);
            }
        };

        template <>
        struct __recursive_and_type<>
        {
            static inline constexpr bool __impl()
            {
                return true;
            }
        };
    }

    template <typename... bool_type>
    static inline constexpr bool recursive_or(const bool_type... m_bools)
    {
        return detail::__recursive_or_type<bool_type...>::__impl(m_bools...);
    }

    template <typename... bool_type>
    static inline constexpr bool recursive_and(const bool_type... m_bools)
    {
        return detail::__recursive_and_type<bool_type...>::__impl(m_bools...);
    }
}

#define fungus_util_constexpr_assert(TERM, ID)  \
fungus_util::__constexpr_assert<fungus_util::__truth((TERM)), fungus_util::__constexpr_assert_M> __check##ID##__;

#ifdef __GNUC__
#define FUNGUSUTIL_ALWAYS_INLINE __attribute__((always_inline))
#else
#define FUNGUSUTIL_ALWAYS_INLINE
#endif

#endif
