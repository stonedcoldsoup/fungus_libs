#ifndef FUNGUSUTIL_ANY_H
#define FUNGUSUTIL_ANY_H

// Handy little doohickey by moi that makes passing random things like
// user data much easier and pain free than void pointers
// and even stores type information, making basic comparisons and
// serialization possible.  Ahhh, template magic :)  This is
// strongly based off of the Ogre::Any class, but I did not steal it,
// this was written from scratch

#include <cstdlib>
#include <iostream>
#include <exception>
#include <typeinfo>
#include <cstring>
#include "fungus_util_pow2.h"
#include "fungus_util_endian.h"
#include "fungus_util_common.h"
#include "fungus_util_sfinae.h"
#include "fungus_util_hash_map.h"

#define FUNGUSUTIL_SERIALIZER_SANITY_WORD        "fis\\b\\"
#define FUNGUSUTIL_SERIALIZER_SANITY_WORD_LENGTH 6

namespace fungus_util
{
    enum
    {
        serialize_none  = (size_t) 0,
        serialize_error = (size_t)-2
    };

    struct FUNGUSUTIL_API serializer_buf
    {
        char *buf;
        size_t size;

        inline serializer_buf(serializer_buf &&b):
            buf(b.buf), size(b.size)
        {
            b.buf  = nullptr;
            b.size = 0;
        }

        serializer_buf(const serializer_buf &b);
        serializer_buf(const char *buf = nullptr, const size_t size = 0);
        serializer_buf(const size_t size);
        ~serializer_buf();

        std::ostream &save_binary(std::ostream &m_os)
        {
            char m_size_bytes[sizeof(size_t)];
            memcpy(m_size_bytes, &size, sizeof(size_t));

            m_os.write(FUNGUSUTIL_SERIALIZER_SANITY_WORD, FUNGUSUTIL_SERIALIZER_SANITY_WORD_LENGTH);
            m_os.write(m_size_bytes, sizeof(size_t)); // write the size of the ensuing data block
            m_os.write(buf, size);             // write the data block

            return m_os;
        }

        bool load_binary(std::istream &m_is)
        {
            if (!m_is)
                return false;

            char m_sanity[FUNGUSUTIL_SERIALIZER_SANITY_WORD_LENGTH];
            static const char *_sanity_ck = FUNGUSUTIL_SERIALIZER_SANITY_WORD;

            // check the sanity word
            m_is.read(m_sanity, FUNGUSUTIL_SERIALIZER_SANITY_WORD_LENGTH);
            if (strncmp(_sanity_ck, m_sanity, FUNGUSUTIL_SERIALIZER_SANITY_WORD_LENGTH))
                return false;

            char m_size_bytes[sizeof(size_t)];
            m_is.read(m_size_bytes, sizeof(size_t)); // read the size of the ensuing data block

            memcpy(&size, m_size_bytes, sizeof(size_t));

            set(size);
            m_is.read(buf, size);             // read the data block

            return (bool)m_is;
        }

        void set(const char *buf = nullptr, const size_t size = 0);
        void set(const size_t size);

        inline serializer_buf &operator =(serializer_buf &&b)
        {
            buf  = b.buf;
            size = b.size;

            b.buf  = nullptr;
            b.size = 0;

            return *this;
        }

        serializer_buf &operator =(const serializer_buf &b);
    };

    struct FUNGUSUTIL_API serializer
    {
        const endian_converter &endian;

        size_t cap, size;
        char *buf;

        bool error, fail;

        serializer(const endian_converter &endian, size_t init_cap = 64);
        ~serializer();

        void reset();
        void get_buf(serializer_buf &_buf) const;

        void _grow();
    };

    struct FUNGUSUTIL_API deserializer
    {
        const endian_converter &endian;

        const char *buf;
        const size_t size;
        size_t i;
        bool eof;

        deserializer(const endian_converter &endian, const char *buf, const size_t size);
        void reset();
    };

    class FUNGUSUTIL_API serializable
    {
    public:
        virtual serializer &serialize(serializer &m_sz) const = 0;
        virtual deserializer &deserialize(deserializer &m_ds) = 0;
    };

    template <bool _b_implemented, typename container_base, typename containerT>
    struct __cmp_containers;

    template <typename container_base, typename containerT>
    struct __cmp_containers<true, container_base, containerT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline bool __impl(const container_base *a, const container_base *b)
        {
            const containerT *_a = dynamic_cast<const containerT *>(a);
            const containerT *_b = dynamic_cast<const containerT *>(b);

            if (!(_a && _b)) return false;

            return (_a->content == _b->content);
        }
    };

    template <typename container_base, typename containerT>
    struct __cmp_containers<false, container_base, containerT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline bool __impl(const container_base *a, const container_base *b)
        {
            return false;
        }
    };

    template <bool _b_implemented, typename contentT>
    struct __content_to_stream;

    template <typename contentT>
    struct __content_to_stream<true, contentT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline void __impl(const contentT &content, std::ostream &os)
        {
            os << content;
        }
    };

    template <typename contentT>
    struct __content_to_stream<false, contentT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline void __impl(const contentT &c, std::ostream &os)
        {
            os << "<stream insertion operator missing>";
        }
    };

    template <bool _b_implemented, typename contentT>
    struct __content_from_stream;

    template <typename contentT>
    struct __content_from_stream<true, contentT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline void __impl(contentT &content, std::istream &is)
        {
            is >> content;
        }
    };

    template <typename contentT>
    struct __content_from_stream<false, contentT>
    {
        FUNGUSUTIL_ALWAYS_INLINE
        static inline void __impl(contentT &c, std::istream &is) {}
    };

    class FUNGUSUTIL_API any_type
    {
    private:
        class container_base
        {
        public:
            virtual ~container_base() {}
            virtual const std::type_info &get_type() const = 0;
            virtual container_base *clone() const = 0;

            virtual bool   cmp_container(const container_base *h) const = 0;

            virtual size_t serialize(const endian_converter &endian, char *buf, size_t buf_len) const = 0;
            virtual size_t deserialize(const endian_converter &endian, const char *buf, size_t buf_len) = 0;

            virtual void   to_stream(std::ostream &os) const = 0;
            virtual void   from_stream(std::istream &is) = 0;
        };

        template <typename T> class generic_container: public container_base
        {
        public:
            typedef generic_container<T> container_t;
            T content;

            generic_container(const T &_content): content(_content) {}
            virtual const std::type_info &get_type() const {return typeid(T);}
            virtual container_base *clone() const {return new generic_container(content);}

            friend struct __cmp_containers<sfinae::supports_equal_to<T>::value, container_base, container_t>;
            friend struct __content_to_stream<sfinae::supports_ostream_insertion<T>::value, container_t>;

            virtual bool cmp_container(const container_base *h) const
            {
                return __cmp_containers<sfinae::supports_equal_to<T>::value, container_base, container_t>::__impl(this, h);
            }

            virtual size_t serialize(const endian_converter &endian, char *buf, size_t buf_len) const
            {
                if (buf_len >= sizeof(T))
                {
                    T data = endian.convert(content);
                    memcpy(buf, &data, sizeof(T));
                    return sizeof(T);
                }
                else
                    return -1;
            }

            virtual size_t deserialize(const endian_converter &endian, const char *buf, size_t buf_len)
            {
                if (buf_len >= sizeof(T))
                {
                    memcpy(&content, buf, sizeof(T));
                    content = endian.convert(content);
                    return sizeof(T);
                }
                else
                    return 0;
            }

            virtual void to_stream(std::ostream &os) const
            {
                __content_to_stream<sfinae::supports_ostream_insertion<T>::value, T>::__impl(content, os);
            }

            virtual void from_stream(std::istream &is)
            {
                __content_from_stream<sfinae::supports_istream_extraction<T>::value, T>::__impl(content, is);
            }
        };

        class string_container: public container_base
        {
        public:
            typedef string_container container_t;
            std::string content;

            string_container(const std::string &_content);
            virtual const std::type_info &get_type() const;
            virtual container_base *clone() const;

            virtual bool cmp_container(const container_base *h) const;

            virtual size_t serialize(const endian_converter &endian, char *buf, size_t buf_len) const;
            virtual size_t deserialize(const endian_converter &endian, const char *buf, size_t buf_len);

            virtual void to_stream(std::ostream &os) const
            {
                os << content;
            }

            virtual void from_stream(std::istream &is)
            {
                is >> content;
            }
        };

        class buf_container: public container_base
        {
        public:
            typedef buf_container container_t;
            serializer_buf content;

            buf_container(const serializer_buf &_content);
            virtual const std::type_info &get_type() const;
            virtual container_base *clone() const;

            virtual bool cmp_container(const container_base *h) const;

            virtual size_t serialize(const endian_converter &endian, char *buf, size_t buf_len) const;
            virtual size_t deserialize(const endian_converter &endian, const char *buf, size_t buf_len);

            virtual void to_stream(std::ostream &os) const {}
            virtual void from_stream(std::istream &is) {}
        };

        container_base *container;

        template<typename T> friend T *any_cast(any_type *);
        friend bool cmp_anys(const any_type &, const any_type &);
    public:
        any_type();
        any_type(const any_type &other);
        any_type(any_type &&other):
            container(nullptr)
        {
            swap_container(other);
        }

        any_type(const char *str);
        any_type(const std::string &str);
        any_type(const serializer_buf &buf);

        template<typename U>
        any_type(const U &data):
            container(new generic_container<U>(data)) {}

        ~any_type();

        size_t serialize(serializer &s) const;
        size_t deserialize(deserializer &ds);
        any_type &swap_container(any_type &b);

        any_type &operator =(const any_type &b);

        inline any_type &operator =(any_type &&b)
        {
            return swap_container(b);
        }

        template<typename T>
        inline any_type &operator =(const T &b)
        {
            return (*this) = std::move(any_type(b));
        }

        bool empty() const;
        const std::type_info &get_type() const;

        friend inline std::ostream &operator <<(std::ostream &os, const any_type &any)
        {
            if (any.container)
                any.container->to_stream(os);
            else
                os << "<nil>";

            return os;
        }

        friend inline std::istream &operator >>(std::istream &is, any_type &any)
        {
            if (any.container)
                any.container->from_stream(is);

            return is;
        }
    };

    FUNGUSUTIL_API serializer &operator <<(serializer &s, const any_type &_any);

    namespace detail
    {
        FUNGUSUTIL_API deserializer &op_get(deserializer &ds, any_type &_any);
    }

    template <typename T>
    deserializer &operator >>(deserializer &ds, T &o)
    {
        any_type _any(o);
        detail::op_get(ds, _any);
        o = any_cast<T>(_any);
        return ds;
    }

    template<typename T> T *any_cast(any_type *any)
    {
        return any && any->get_type() == typeid(T) ?
               &static_cast<any_type::generic_container<T> *>(any->container)->content
               : nullptr;
    }

    template<> FUNGUSUTIL_API std::string *any_cast<std::string>(any_type *any);
    template<> FUNGUSUTIL_API serializer_buf *any_cast<serializer_buf>(any_type *any);

    template<typename T> const T *any_cast(const any_type *any)
    {
        return any_cast<T>(const_cast<any_type *>(any));
    }

    template<typename T> T any_cast(const any_type &any)
    {
        const T *result = any_cast<T>(&any);
        return *result;
    }

    FUNGUSUTIL_API bool cmp_anys(const any_type &a, const any_type &b);

    inline bool operator ==(const any_type &a, const any_type &b)
    {
        return cmp_anys(a, b);
    }

    inline bool operator !=(const any_type &a, const any_type &b)
    {
        return !cmp_anys(a, b);
    }
}

#endif

