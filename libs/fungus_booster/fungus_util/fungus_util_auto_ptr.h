#ifndef FUNGUSUTIL_AUTO_PTR_H
#define FUNGUSUTIL_AUTO_PTR_H

#include "fungus_util_common.h"

namespace fungus_util
{
    using namespace fungus_util;

    enum dereference_mode_e
    {
        dereference_mode_value,
        dereference_mode_ref
    };

    template <typename T, dereference_mode_e dereference_mode = dereference_mode_ref>
    class auto_ptr
    {
    private:
        class container
        {
        private:
            T *object;
            int nrefs;

        public:
            container(T *object):
                object(object), nrefs(object ? 1 : 0)
            {
                fungus_util_assert(object, "A auto_ptr::container was generated with nullptr object!");
            }

            ~container()
            {
                fungus_util_assert(nrefs == 0, "A auto_ptr::container was deleted while still referenced!");
            }

            inline T *get()
            {
                return object;
            }

            inline container *grab()
            {
                if (nrefs == 0)
                    return nullptr;
                else
                {
                    ++nrefs;
                    return this;
                }
            }

            inline bool drop()
            {
                bool done;

                if ((done = (0 == --nrefs)))
                {
                    delete object;
                    object = nullptr;
                }

                return done;
            }
        };

        mutable container *c;

        inline T *get() const
        {
            return c ? c->get() : nullptr;
        }

        inline void drop()
        {
            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }
        }

        inline void grab(auto_ptr &p)
        {
            c = p.c ? p.c->grab() : nullptr;
        }

        inline void xchg(auto_ptr &p)
        {
            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            c = p.c ? p.c->grab() : nullptr;
        }

        inline void mov(auto_ptr &&p)
        {
            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            c   = p.c;
            p.c = nullptr;
        }

        inline void xchg(T *p)
        {
            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            c = p ? new container(p) : nullptr;
        }
    public:
        auto_ptr():
            c(nullptr)
        {}

        auto_ptr(T *p):
            c(nullptr)
        {
            c = p ? new container(p) : nullptr;
        }

        auto_ptr(auto_ptr &&p):
            c(nullptr)
        {
            mov(std::move(p));
        }

        auto_ptr(const auto_ptr &p)
        {
            grab(const_cast<auto_ptr &>(p));
        }

        ~auto_ptr()
        {
            drop();
        }

        inline auto_ptr &operator =(T *p)
        {
            xchg(p);
            return *this;
        }

        inline auto_ptr &operator =(auto_ptr &&p)
        {
            mov(std::move(p));
            return *this;
        }

        inline auto_ptr &operator =(const auto_ptr &p)
        {
            xchg(const_cast<auto_ptr &>(p));
            return *this;
        }

        inline bool operator ==(const auto_ptr &p) const
        {
            return this->c == p.c;
        }

        inline T *operator->()
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return p;
        }

        inline const T *operator->() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return p;
        }

        inline T &ref()
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return *p;
        }

        inline const T &ref() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return *p;
        }

        inline T value() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            T v  = *p;
            return  v;
        }

        inline operator T *()
        {
            return get();
        }

        inline operator const T *() const
        {
            return get();
        }
    };

    template <typename T>
    static inline T operator *(const auto_ptr<T, dereference_mode_value> &p)
    {
        return p.value();
    }

    template <typename T>
    static inline T &operator *(const auto_ptr<T, dereference_mode_ref> &p)
    {
        return p.ref();
    }

    template <typename T>
    static inline const T &operator *(const auto_ptr<T, dereference_mode_ref> &p)
    {
        return p.ref();
    }
}

#endif
