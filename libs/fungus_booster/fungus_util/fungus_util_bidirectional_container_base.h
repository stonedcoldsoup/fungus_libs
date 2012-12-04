#ifndef FUNGUSUTIL_BIDIRECTIONAL_CONTAINER_BASE_H
#define FUNGUSUTIL_BIDIRECTIONAL_CONTAINER_BASE_H

#include "fungus_util_common.h"

namespace fungus_util
{
    class FUNGUSUTIL_API __container_list_base
    {
    protected:
        class FUNGUSUTIL_API elem
        {
        private:
            bool no_rem;

            __container_list_base *__base;
            friend class __container_list_base;
        public:
            mutable elem *__base_next;
            mutable elem *__base_prev;

            elem(__container_list_base *__base);
            virtual ~elem();
        };

        mutable elem *__base_first;
        mutable elem *__base_last;

        size_t __base_n_elems;

        __container_list_base();
        virtual ~__container_list_base();
    private:

        virtual void add(elem *_e);
        virtual void rem(elem *_e);
    public:
        virtual void clear();

        virtual size_t size()  const;
        virtual bool   empty() const;
    };
}

#endif
