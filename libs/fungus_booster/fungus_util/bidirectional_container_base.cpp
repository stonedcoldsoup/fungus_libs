#include "fungus_util_bidirectional_container_base.h"

namespace fungus_util
{
    __container_list_base::elem::elem(__container_list_base *__base):
        no_rem(false), __base(__base), __base_next(nullptr), __base_prev(nullptr)
    {
        __base->add(this);
    }

    __container_list_base::elem::~elem()
    {
        if (!no_rem) __base->rem(this);
    }

    void __container_list_base::add(elem *_e)
    {
        if (__base_first == nullptr)
            __base_first = __base_last = _e;
        else
        {
            __base_last->__base_next = _e;
            _e->__base_prev = __base_last;
            __base_last = _e;
        }

        ++__base_n_elems;
    }

    void __container_list_base::rem(elem *_e)
    {
        if (_e == __base_first) __base_first = _e->__base_next;
        if (_e == __base_last)  __base_last  = _e->__base_prev;

        if (_e->__base_next) _e->__base_next->__base_prev = _e->__base_prev;
        if (_e->__base_prev) _e->__base_prev->__base_next = _e->__base_next;

        --__base_n_elems;
    }

    __container_list_base::__container_list_base():
        __base_first(nullptr), __base_last(nullptr), __base_n_elems(0) {}

    __container_list_base::~__container_list_base()
    {
        clear();
    }

    void   __container_list_base::clear()
    {
        while (__base_first)
        {
            elem *_next = __base_first->__base_next;
            __base_first->no_rem = true;
            delete __base_first;
            __base_first = _next;
        }

        __base_first = __base_last = nullptr;
        __base_n_elems = 0;
    }

    size_t __container_list_base::size()  const
    {
        return __base_n_elems;
    }

    bool   __container_list_base::empty() const
    {
        return __base_n_elems == 0;
    }
}
