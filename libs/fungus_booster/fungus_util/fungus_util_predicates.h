#ifndef FUNGUSUTIL_PREDICATES_H
#define FUNGUSUTIL_PREDICATES_H

#include <cstdlib>

namespace fungus_util
{
    // Use this with std::remove_if to remove all duplicate
    // members from the container.  Useful when using a map
    // to ensure all members are unique is a hassle and/or
    // ends up being way less efficient.
    template <typename T>
    class is_dup
    {
    private:
        size_t ls_siz, ls_cap;
        T *ls;

    public:
        is_dup(size_t ls_cap): ls_siz(0), ls_cap(ls_cap)
        {
            ls = new T[ls_cap];
        }

        ~is_dup()
        {
            delete[] ls;
        }

        bool operator()(const T &val)
        {
            for (size_t i = 0; i < ls_siz; ++i)
            {
                if (ls[i] == val) return true;
            }

            ls[ls_siz++] = val;
            fungus_util_assert(ls_siz <= ls_cap, "is_dup: size exceeded capacity!");
            return false;
        }
    };
}

#endif
