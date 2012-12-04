#ifndef FUNGUSCONCURRENCY_CONCURRENT_AUTO_PTR_H
#define FUNGUSCONCURRENCY_CONCURRENT_AUTO_PTR_H

#include "fungus_concurrency_common.h"

namespace fungus_concurrency
{
    using namespace fungus_util;

    template <typename T, dereference_mode_e dereference_mode = dereference_mode_ref>
    class concurrent_auto_ptr
    {
    private:
        class container
        {
        private:
            mutex m;
            T *object;
            int nrefs;

        public:
            container(T *object):
                m(), object(object), nrefs(object ? 1 : 0)
            {
                fungus_util_assert(object, "A concurrent_auto_ptr::container was generated with nullptr object!");
            }

            ~container()
            {
                fungus_util_assert(nrefs == 0, "A concurrent_auto_ptr::container was deleted while still referenced!");
            }

            FUNGUSCONCURRENCY_INLINE T *get()
            {
                lock guard(m);
                T *p = object;
                return p;
            }

            FUNGUSCONCURRENCY_INLINE container *grab()
            {
                lock guard(m);
                if (nrefs == 0)
                    return nullptr;
                else
                {
                    ++nrefs;
                    return this;
                }
            }

            FUNGUSCONCURRENCY_INLINE bool drop()
            {
                bool done;
                lock guard(m);

                if ((done = (0 == --nrefs)))
                {
                    delete object;
                    object = nullptr;
                }

                return done;
            }
        };

        mutable mutex m;
        mutable container *c;

        FUNGUSCONCURRENCY_INLINE T *get() const
        {
            lock guard(m);
            return c ? c->get() : nullptr;
        }

        FUNGUSCONCURRENCY_INLINE void drop()
        {
            lock guard(m);
            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }
        }

        FUNGUSCONCURRENCY_INLINE void grab(concurrent_auto_ptr &p)
        {
            lock guard0(m);
            lock guard1(p.m);

            c = p.c ? p.c->grab() : nullptr;
        }

        FUNGUSCONCURRENCY_INLINE void xchg(concurrent_auto_ptr &p)
        {
            lock guard0(m);

            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            lock guard1(p.m);
            c = p.c ? p.c->grab() : nullptr;
        }

        FUNGUSCONCURRENCY_INLINE void mov(concurrent_auto_ptr &&p)
        {
            lock guard0(m);

            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            lock guard1(p.m);
            c   = p.c;
            p.c = nullptr;
        }

        FUNGUSCONCURRENCY_INLINE void xchg(T *p)
        {
            lock guard(m);

            if (c && c->drop())
            {
                delete c;
                c = nullptr;
            }

            c = p ? new container(p) : nullptr;
        }
    public:
        concurrent_auto_ptr():
            m(), c(nullptr)
        {}

        concurrent_auto_ptr(T *p):
            m(), c(nullptr)
        {
            c = p ? new container(p) : nullptr;
        }

        concurrent_auto_ptr(concurrent_auto_ptr &&p):
            m(), c(nullptr)
        {
            mov(std::move(p));
        }

        concurrent_auto_ptr(const concurrent_auto_ptr &p)
        {
            grab(const_cast<concurrent_auto_ptr &>(p));
        }

        ~concurrent_auto_ptr()
        {
            drop();
        }

        FUNGUSCONCURRENCY_INLINE concurrent_auto_ptr &operator =(T *p)
        {
            xchg(p);
            return *this;
        }

        FUNGUSCONCURRENCY_INLINE concurrent_auto_ptr &operator =(concurrent_auto_ptr &&p)
        {
            mov(std::move(p));
            return *this;
        }

        FUNGUSCONCURRENCY_INLINE concurrent_auto_ptr &operator =(const concurrent_auto_ptr &p)
        {
            xchg(const_cast<concurrent_auto_ptr &>(p));
            return *this;
        }

        FUNGUSCONCURRENCY_INLINE bool operator ==(const concurrent_auto_ptr &p) const
        {
            lock guard0(m);
            lock guard1(p.m);
            return this->c == p.c;
        }

        FUNGUSCONCURRENCY_INLINE T *operator->()
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return p;
        }

        FUNGUSCONCURRENCY_INLINE const T *operator->() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return p;
        }

        FUNGUSCONCURRENCY_INLINE T &ref()
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return *p;
        }

        FUNGUSCONCURRENCY_INLINE const T &ref() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            return *p;
        }

        FUNGUSCONCURRENCY_INLINE T value() const
        {
            T *p = get();
            fungus_util_assert(p, "Attempted to dereference a smart pointer when no object was present!");

            T v  = *p;
            return  v;
        }

        FUNGUSCONCURRENCY_INLINE operator T *()
        {
            return get();
        }

        FUNGUSCONCURRENCY_INLINE operator const T *() const
        {
            return get();
        }
    };

    template <typename T>
    static FUNGUSCONCURRENCY_INLINE T operator *(concurrent_auto_ptr<T, dereference_mode_value> p)
    {
        return p.value();
    }

    template <typename T>
    static FUNGUSCONCURRENCY_INLINE T &operator *(concurrent_auto_ptr<T, dereference_mode_ref> p)
    {
        return p.ref();
    }

    template <typename T>
    static FUNGUSCONCURRENCY_INLINE const T &operator *(const concurrent_auto_ptr<T, dereference_mode_ref> p)
    {
        return p.ref();
    }
};

#endif
