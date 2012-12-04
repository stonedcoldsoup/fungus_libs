#ifndef FUNGUSUTIL_ENDIAN_H
#define FUNGUSUTIL_ENDIAN_H

#include <cstddef>
#include <stdint.h>

#include "fungus_util_common.h"
#include "fungus_util_type_info_wrap.h"
#include "fungus_util_hash_map.h"
#include "thread/fungus_util_thread.h"

namespace fungus_util
{
    // FIXME: Add macros distinguishing platform endianness.

    // This is a nice little endian converter that automagically
    // Converts everything from and to big endian for you (if that
    // is appropriate on your hardware).  The serializer/deserializer
    // provided with any_type uses this to ensure cross platform
    // compatibility with networking.

    enum
    {
        big_endian = 0xFF00,
        little_endian = 0x00FF,

        // Cheating because I am only testing on Windows for now.
        // When I make this cross platform, there will be some
        // ifdefs involved in deciding which this is.
        native_endian = little_endian,
        foreign_endian = big_endian
    };

    // n_bytes should ALWAYS ALWAYS ALWAYS be a multiple of 2.
    // Why anyone would use this for anything else is beyond me.
    template <unsigned int n_bytes>
    static inline void flip_endian(const char *bytes_in, char *bytes_out)
    {
        register unsigned int i = 0, ri = n_bytes - 1;
        for (; i < n_bytes; (++i, --ri)) bytes_out[i] = bytes_in[ri];
    }

    class FUNGUSUTIL_API endian_registration
    {
    private:
        struct endian_type
        {
			virtual ~endian_type();
		
            virtual const std::type_info &get_type() const = 0;
            virtual endian_type *clone() const = 0;
        };

        template <typename T>
        struct endian_type_inst: endian_type
        {
            virtual const std::type_info &get_type() const {return typeid(T);}
            virtual endian_type *clone() const {return new endian_type_inst<T>();}
        };

        endian_type *et;

        endian_registration(endian_type *et = nullptr);

#ifdef FUNGUSUTIL_CPP11_PARTIAL
        friend class hash_map<default_hash<type_info_wrap, endian_registration>>;
#else
        friend class std::map<type_info_wrap, endian_registration>;
#endif
    public:
        template <typename U>
        static endian_registration get()
        {
            return endian_registration(new endian_type_inst<U>());
        }

        endian_registration(const endian_registration &h);
        ~endian_registration();

        const std::type_info &get_type() const;
        const int get_net_endian() const;
    };

#ifdef FUNGUSUTIL_CPP11_PARTIAL
    typedef hash_map<default_hash<type_info_wrap, endian_registration>> __endian_reg_map_type;
#else
    typedef std::map<type_info_wrap, endian_registration> __endian_reg_map_type;
#endif

    class FUNGUSUTIL_API endian_converter
    {
    private:
        template <typename T, int endian>
        struct __smart_endian_flip
        {
            T convert(const T &v) const
            {
                return v;
            }
        };

        template <typename T>
        struct __smart_endian_flip<T, foreign_endian>
        {
            T convert(const T &v) const
            {
                T r;
                flip_endian<sizeof(T)>((const char *)&v, (char *)&r);
                return r;
            }
        };

        mutex m;

        __endian_reg_map_type type_regs;
        bool __lazy_init_numeric;
    public:
        endian_converter();
        endian_converter(endian_converter &&endian);

        void lazy_register_numeric_types();
        void register_type(const endian_registration &et);
        void unregister_type(const type_info_wrap &info);
        bool type_registered(const type_info_wrap &info, int &target_endian) const;

        template <typename T> inline void register_type()                           {register_type(endian_registration::get<T>());}
        template <typename T> inline void unregister_type()                         {unregister_type(typeid(T));}
        template <typename T> inline bool type_registered(int &target_endian) const {return type_registered(typeid(T), target_endian);}
        template <typename T> inline bool type_registered() const                   {int _dummy; return type_registered(typeid(T), _dummy);}

        template <typename T>
        inline T convert(const T &v) const
        {
            const __smart_endian_flip<T, big_endian>    __big_converter    = __smart_endian_flip<T, big_endian>();
            const __smart_endian_flip<T, little_endian> __little_converter = __smart_endian_flip<T, little_endian>();

            int target_endian;
            bool do_conv = type_registered<T>(target_endian);

            if (do_conv)
            {
                switch (target_endian)
                {
                case big_endian:    return __big_converter.convert(v);
                case little_endian: return __little_converter.convert(v);
                default:            fungus_util_assert(false, "Invalid endian type!");
                };
            }

            return v;
        }
    };
}

#endif
