#ifndef FUNGUSUTIL_SFINAE_H
#define FUNGUSUTIL_SFINAE_H

#include "fungus_util_common.h"
#ifdef FUNGUSUTIL_CPP11_PARTIAL
    #include <type_traits>
    #include <iostream>

    #define FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_STRUCTS(Name, Op)    \
    struct __return##Name {};                                           \
                                                                        \
    template <typename T>                                               \
    __return##Name operator Op(const T &, const T &) {return __return##Name();}

    #define FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(Name, Op)      \
            FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_STRUCTS(Name, Op)    \
                                                                        \
    template <typename T>                                               \
    struct supports_##Name:                                             \
        std::integral_constant                                          \
        <                                                               \
            bool,                                                       \
            !std::is_same                                               \
            <                                                           \
                decltype(std::declval<T>() Op std::declval<T>()),       \
                __return##Name                                          \
            >::value                                                    \
        >                                                               \
    {};
#else
    #define FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(Name, Op)      \
    template <typename T>                                               \
    struct supports_##Name {static const bool value = true;};
#endif

namespace fungus_util
{
    namespace sfinae
    {
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(greater_than, >)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(less_than,    <)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(equal_to,    ==)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(nequal_to,   !=)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(gteq,        >=)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(lteq,        <=)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(plus,         +)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(minus,        -)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(mult,         *)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(divide,       /)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(shift_left,  <<)
        FUNGUSUTIL_DEF_BINARY_OPERATOR_SUPPORT_CHECK(shift_right, >>)

// BROKEN as of GCC 4.7.x
/*#ifdef FUNGUSUTIL_CPP11_PARTIAL
        namespace detail
        {
            struct no  {};
            struct yes {};

            struct any_t
            {
                template<typename T> any_t(T &);
            };

            namespace __sosi_impl
            {
                no operator <<(const std::ostream &, const any_t &);

                yes &defined(std::ostream &);
                no   defined(no);

                template<typename T>
                struct _impl
                {
                    static std::ostream &s;
                    static const T &t;
                    static constexpr bool value =
                        std::is_same<decltype(defined(s << t)), yes>::value;
                };
            }

            namespace __sise_impl
            {
                no operator >>(const std::istream &, const any_t &);

                yes &defined(std::istream &);
                no   defined(no);

                template<typename T>
                struct _impl
                {
                    static std::istream &s;
                    static const T &t;
                    static constexpr bool value =
                        std::is_same<decltype(defined(s >> t)), yes>::value;
                };
            }
        }

        template<typename T>
        struct supports_ostream_insertion:
        detail::__sosi_impl::_impl<T> {};

        template<typename T>
        struct supports_istream_extraction:
        detail::__sise_impl::_impl<T> {};
		
		#define FUNGUSUTIL_USE_SFINAE \
			using ::fungus_util::sfinae::detail::yes;						\
			using ::fungus_util::sfinae::detail::no;						\
			using ::fungus_util::sfinae::detail::__sosi_impl::defined;		\
			using ::fungus_util::sfinae::detail::__sise_impl::defined;		\
			using ::fungus_util::sfinae::detail::__sosi_impl::operator <<;	\
			using ::fungus_util::sfinae::detail::__sise_impl::operator >>; \
			
#else
        template<typename T>
        struct supports_ostream_insertion
        {
            static const bool value = true;
        };

        template<typename T>
        struct supports_istream_extraction
        {
            static const bool value = true;
        };
#endif*/
		namespace detail
        {
			// we use sizeof tricks instead.
			namespace __sosi_impl
			{
				typedef char no;
				typedef char yes[2];

				struct any_t
				{
					template <typename T> any_t(T const &);
				};

				no operator <<(std::ostream const&, any_t const &);

				yes &test(std::ostream &);
				no test(no);

				template <typename T>
				struct _impl
				{
					static std::ostream &s;
					static T const &t;
					static bool const value = sizeof(test(s << t)) == sizeof(yes);
				};
			}
			
			namespace __sise_impl
			{
				typedef char no;
				typedef char yes[2];

				struct any_t
				{
					template <typename T> any_t(T const &);
				};

				no operator >>(std::istream const&, any_t const &);

				yes &test(std::istream &);
				no test(no);

				template <typename T>
				struct _impl
				{
					static std::istream &s;
					static T const &t;
					static bool const value = sizeof(test(s >> t)) == sizeof(yes);
				};
			}
		}

		template<typename T>
        struct supports_ostream_insertion:
        detail::__sosi_impl::_impl<T> {};

        template<typename T>
        struct supports_istream_extraction:
        detail::__sise_impl::_impl<T> {};
    }
}

#endif
