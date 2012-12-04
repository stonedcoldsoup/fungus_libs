#ifndef FUNGUSUTIL_OPTIONAL_H
#define FUNGUSUTIL_OPTIONAL_H

#include "fungus_util_common.h"

#ifdef FUNGUSUTIL_CPP11_PARTIAL

namespace fungus_util
{
    template <typename T>
    class optional
    {
    public:
        typedef typename std::aligned_storage
            <sizeof(T), std::alignment_of<T>::value>::type
            storage_type;

        typedef T object_type;
    private:
        object_type  *p_object;
        storage_type  m_storage;

    public:
        FUNGUSUTIL_ALWAYS_INLINE inline optional(): p_object(nullptr) {}

        FUNGUSUTIL_ALWAYS_INLINE inline optional(const optional &m_o):
            p_object(nullptr)
        {
            if (m_o.p_object)
                create(*(m_o.p_object));
        }

        template <typename... argT>
        FUNGUSUTIL_ALWAYS_INLINE inline optional(argT&&... argV)
        {
            p_object = new(m_storage.__data) object_type(std::forward<argT>(argV)...);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline ~optional()
        {
            destroy();
        }

        /*template <typename argT>
        auto operator =(const argT &argV)
            -> decltype(p_object->operator =(argV))
        {
            if (!p_object)
                create();

            return p_object->operator =(argV);
        }

        template <typename argT>
        auto operator =(argT &&argV)
            -> decltype(p_object->operator =(std::move(argV)))
        {
            if (!p_object)
                create();

            return p_object->operator =(std::move(argV));
        }*/

        FUNGUSUTIL_ALWAYS_INLINE inline unsigned char *buf_ptr()
        {
            if (p_object)
                return nullptr;
            else
                return m_storage.__data;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool place(object_type *p_object)
        {
            return this->p_object ? false : (this->p_object = p_object) != nullptr;
        }

        template <typename... argT>
        FUNGUSUTIL_ALWAYS_INLINE inline object_type *create(argT&&... argV)
        {
            if (p_object)
                return nullptr;
            else
            {
                p_object = new(m_storage.__data) object_type(std::forward<argT>(argV)...);
                return p_object;
            }
        }

        template <typename... argT>
        FUNGUSUTIL_ALWAYS_INLINE inline object_type *replace(argT&&... argV)
        {
            if (p_object)
                destroy();

            return create(std::forward<argT>(argV)...);
        }

        FUNGUSUTIL_ALWAYS_INLINE inline bool destroy()
        {
            bool success = p_object != nullptr;

            if (success)
            {
                p_object->~object_type();
                p_object = nullptr;
            }

            return success;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline operator       object_type *()       {return p_object;}
        FUNGUSUTIL_ALWAYS_INLINE inline operator const object_type *() const {return p_object;}

        FUNGUSUTIL_ALWAYS_INLINE inline operator       object_type &()
        {
            fungus_util_assert(p_object != nullptr, "fungus_util::optional::operator object_type &(): attempted to obtain reference to nullptr object!");
            return *p_object;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline operator const object_type &() const
        {
            fungus_util_assert(p_object != nullptr, "fungus_util::optional::operator const object_type &(): attempted to obtain reference to nullptr object!");
            return *p_object;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        object_type &operator *()
        {
            return (object_type &)*this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        const object_type &operator *() const
        {
            return (const object_type &)*this;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        operator bool() const
        {
            return p_object != nullptr;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        bool operator !() const
        {
            return p_object == nullptr;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        object_type *operator ->()
        {
            fungus_util_assert(p_object != nullptr, "fungus_util::optional::operator ->(): attempted to dereference a nullptr object!");
            return p_object;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        const object_type *operator ->() const
        {
            fungus_util_assert(p_object != nullptr, "fungus_util::optional::operator ->(): attempted to dereference a nullptr object!");
            return p_object;
        }
    };
}

#endif
#endif
