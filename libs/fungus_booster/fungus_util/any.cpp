#include "fungus_util_any.h"

namespace fungus_util
{
    serializer_buf::serializer_buf(const serializer_buf &b): buf(nullptr), size(b.size)
    {
        if (b.size && b.buf)
        {
            buf = new char[size];
            memcpy(buf, b.buf, size);
        }
    }

    serializer_buf::serializer_buf(const char *buf, const size_t size): buf(nullptr), size(size)
    {
        if (size && buf)
        {
            this->buf = new char[size];
            memcpy(this->buf, buf, size);
        }
    }

    serializer_buf::serializer_buf(const size_t size): buf(nullptr), size(size)
    {
        if (size)
            this->buf = new char[size];
    }

    serializer_buf::~serializer_buf()
    {
        if (buf) delete[] buf;
    }

    void serializer_buf::set(const char *buf, const size_t size)
    {
        if (this->buf) delete[] this->buf;
        this->buf  = nullptr;
        this->size = size;

        if (size && buf)
        {
            this->buf = new char[size];
            memcpy(this->buf, buf, size);
        }
    }

    void serializer_buf::set(const size_t size)
    {
        if (buf) delete[] buf;

        buf = nullptr;
        this->size = size;

        if (size) buf = new char[size];
    }

    serializer_buf &serializer_buf::operator =(const serializer_buf &b)
    {
        set(b.buf, b.size);
        return *this;
    }

    serializer::serializer(const endian_converter &endian, size_t init_cap):
        endian(endian),
        cap(init_cap), size(0), buf(nullptr),
        error(false), fail(false)
    {
        if (!is_pow2(cap))
            cap = next_pow2(cap);

        buf = new char[cap];
    }

    serializer::~serializer()
    {
        delete[] buf;
    }

    void serializer::reset()
    {
        size = 0;
        error = false;
        fail = false;
    }

    void serializer::get_buf(serializer_buf &_buf) const
    {
        _buf.set(buf, size);
    }

    void serializer::_grow()
    {
        char *nbuf = new char[cap <<= 1];

        memcpy(nbuf, buf, size);

        delete[] buf;
        buf = nbuf;
    }

    deserializer::deserializer(const endian_converter &endian, const char *buf, const size_t size):
        endian(endian), buf(buf), size(size), i(0) {}

    void deserializer::reset() {i = 0;}

    any_type::string_container::string_container(const std::string &_content): content(_content) {}
    const std::type_info &any_type::string_container::get_type() const {return typeid(std::string);}
    any_type::container_base *any_type::string_container::clone() const {return new string_container(content);}

    bool any_type::string_container::cmp_container(const container_base *h) const
    {
        const container_t *containerT = dynamic_cast<const container_t *>(h);
        if (!containerT) return false;

        return (content == containerT->content);
    }

    static const char str_marker = (const char)234;

    size_t any_type::string_container::serialize(const endian_converter &endian, char *buf, size_t buf_len) const
    {
        size_t size = content.size() + 2;

        if (size > buf_len)
            return -1;
        else
        {
            buf[0] = str_marker;
            memcpy(buf + 1, content.c_str(), content.size());
            buf[size - 1] = '\0';
        }

        return size;
    }

    size_t any_type::string_container::deserialize(const endian_converter &endian, const char *buf, size_t buf_len)
    {
        content.clear();

        if (buf_len < 2) return 0;
        if (str_marker != buf[0]) return 0;

        const char *str_ptr = buf + 1;
        size_t i, b_rem = buf_len - 1;

        for (i = 0; i < b_rem && str_ptr[i]; ++i)
            content.push_back(str_ptr[i]);

        return i >= b_rem ? i + 1 : content.size() + 2;
    }

    any_type::buf_container::buf_container(const serializer_buf &_content): content(_content) {}
    const std::type_info &any_type::buf_container::get_type() const {return typeid(serializer_buf);}
    any_type::container_base *any_type::buf_container::clone() const {return new buf_container(content);}

    bool any_type::buf_container::cmp_container(const container_base *h) const
    {
        const container_t *containerT = dynamic_cast<const container_t *>(h);
        if (!containerT) return false;

        return (content.buf == containerT->content.buf);
    }

    size_t any_type::buf_container::serialize(const endian_converter &endian, char *buf, size_t buf_len) const
    {
        if (content.size > buf_len)
            return -1;
        else
            memcpy(buf, content.buf, content.size);

        return content.size;
    }

    size_t any_type::buf_container::deserialize(const endian_converter &endian, const char *buf, size_t buf_len)
    {
        if (content.size > buf_len)
            return 0;
        else
            memcpy(content.buf, buf, content.size);

        return content.size;
    }

    any_type::any_type():                          container(nullptr) {}
    any_type::any_type(const any_type &other):     container(other.container ? other.container->clone() : 0) {}
    any_type::any_type(const char *str):           container(new string_container(str)) {}
    any_type::any_type(const std::string &str):    container(new string_container(str)) {}
    any_type::any_type(const serializer_buf &buf): container(new buf_container(buf)) {}

    any_type::~any_type() {if (container) delete container;}

    size_t any_type::serialize(serializer &s) const
    {
        if (!container) return 0;

        size_t n;
        while ((n = container->serialize(s.endian, s.buf + s.size, s.cap - s.size)) == (size_t)-1) s._grow();
        if (n == serialize_error)
            s.error = s.fail = true;
        else
            s.size += n;
        return n;
    }

    size_t any_type::deserialize(deserializer &ds)
    {
        if (!container) return 0;

        size_t n = container->deserialize(ds.endian, ds.buf + ds.i, ds.size - ds.i);
        if (n == 0 || n == serialize_error)
            ds.eof = true;
        else
            ds.i += n;

        return n;
    }

    any_type &any_type::swap_container(any_type &b)
    {
        container_base *t = this->container;

        this->container = b.container;
        b.container = t;

        return *this;
    }

    any_type &any_type::operator =(const any_type &b)
    {
        if (this->container) delete this->container;
        this->container = b.container ? b.container->clone() : nullptr;
        return *this;
    }

    bool any_type::empty() const
    {
        return !container;
    }

    const std::type_info &any_type::get_type() const
    {
        return container ? container->get_type() : typeid(void);
    }

    serializer &operator <<(serializer &s, const any_type &_any)
    {
        _any.serialize(s);
        return s;
    }

    namespace detail
    {
        deserializer &op_get(deserializer &ds, any_type &_any)
        {
            if (_any.deserialize(ds) == 0) ds.eof = true;
            return ds;
        }
    }

    bool cmp_anys(const any_type &a, const any_type &b)
    {
        if (a.get_type() != b.get_type()) return false;
        if (!a.container && b.container)  return false;
        if (a.container && !b.container)  return false;
        if (!a.container && !b.container) return true;

        return a.container->cmp_container(b.container);
    }

    template<> std::string *any_cast<std::string>(any_type *any)
    {
        return any && any->get_type() == typeid(std::string) ?
               &static_cast<any_type::string_container *>(any->container)->content
               : nullptr;
    }

    template<> serializer_buf *any_cast<serializer_buf>(any_type *any)
    {
        return any && any->get_type() == typeid(serializer_buf) ?
               &static_cast<any_type::buf_container *>(any->container)->content
               : nullptr;
    }
}
