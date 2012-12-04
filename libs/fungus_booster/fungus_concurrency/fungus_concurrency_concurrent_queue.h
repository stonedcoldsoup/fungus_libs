#ifndef FUNGUSCONCURRENCY_CONCURRENT_QUEUE_H
#define FUNGUSCONCURRENCY_CONCURRENT_QUEUE_H

#include "fungus_concurrency_common.h"

namespace fungus_concurrency
{
    using namespace fungus_util;

    template <typename T>
    class concurrent_queue
    {
    private:
        struct node
        {
            mutable mutex m;

            bool empty;
            T v;

            node *next;

            node():           m(), empty(true),  v(),  next(nullptr) {}
            node(const T &v): m(), empty(false), v(v), next(nullptr) {}

            FUNGUSCONCURRENCY_INLINE void set_next(node *next)
            {
                lock guard(m);
                this->next = next;
            }

            FUNGUSCONCURRENCY_INLINE node *get_next() const
            {
                lock guard(m);
                return next;
            }
        };

        node *first, *last;

        mutable mutex producer_lock;
        mutable mutex consumer_lock;

        FUNGUSCONCURRENCY_INLINE bool __pop(T &v)
        {
            node *first_p = first;
            node *next_p = first->get_next();

            if (next_p != nullptr)
            {
                v = next_p->v;
                next_p->empty = true;
                first = next_p;

                consumer_lock.unlock();
                delete first_p;

                return true;
            }
            else
            {
                consumer_lock.unlock();
                return false;
            }
        }
    public:
        concurrent_queue(): producer_lock(), consumer_lock()
        {
            first = new node();
            last  = first;
        }

        ~concurrent_queue()
        {
            while (first != nullptr)
            {
                node *t = first;
                first = (node *)t->next;
                delete t;
            }
        }

        FUNGUSCONCURRENCY_INLINE void push(const T &v)
        {
            lock guard(producer_lock);

            node *n = new node(v);

            last->set_next(n);
            last = n;
        }

        FUNGUSCONCURRENCY_INLINE bool pop_if_open(T &v)
        {
            if (consumer_lock.try_lock())
                return __pop(v);
            else
                return false;
        }

        FUNGUSCONCURRENCY_INLINE bool pop(T &v)
        {
            consumer_lock.lock();
            return __pop(v);
        }
    };
}

#endif
