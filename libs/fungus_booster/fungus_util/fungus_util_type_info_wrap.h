#ifndef FUNGUSUTIL_TYPE_INFO_WRAP_H
#define FUNGUSUTIL_TYPE_INFO_WRAP_H

#include <typeinfo>
#include "fungus_util_common.h"

namespace fungus_util
{
    // this is a simple type info wrapper
    // that allows you to actually replicate
    // std::type_info, something which is
    // missing from C++ and has always baffled me.
    // With this, you could for example use
    // type_info as a key for a map, because it
    // also has a full set of comparison operators
    // that just wrap the ==, !=, and before()
    // members of type_info.

    // It's used internally by endian_converter
    // as a key value in a map it uses to store
    // the types registered for conversion.

    class type_info_wrap
    {
    private:
        struct container
        {
            const std::type_info &info;

            container():                           info(typeid(void)) {}
            container(const std::type_info &info): info(info)         {}
            container(const container &wrap):      info(wrap)         {}

            operator const std::type_info &() const
            {
                return info;
            }

            bool operator ==(const container &wrap) const
            {
                return info == wrap.info;
            }

            bool operator !=(const container &wrap) const
            {
                return info != wrap.info;
            }

            bool operator <(const container &wrap) const
            {
                return info.before(wrap.info);
            }

            bool operator >=(const container &wrap) const
            {
                return !info.before(wrap.info);
            }

            bool operator >(const container &wrap) const
            {
                return *this >= wrap && *this != wrap;
            }

            bool operator <=(const container &wrap) const
            {
                return *this < wrap && *this == wrap;
            }
        };

        container *m_container;
    public:
        type_info_wrap():
            m_container(nullptr)
        {
            m_container = new container();
        }

        type_info_wrap(const std::type_info &info):
            m_container(nullptr)
        {
            m_container = new container(info);
        }

        type_info_wrap(const type_info_wrap &wrap):
            m_container(nullptr)
        {
            m_container = new container(*wrap.m_container);
        }

        type_info_wrap(type_info_wrap &&wrap):
            m_container(nullptr)
        {
            m_container = new container();
            *this = std::move(wrap);
        }

        ~type_info_wrap()
        {
            delete m_container;
        }

        const type_info_wrap &operator =(const type_info_wrap &wrap)
        {
            delete m_container;
            m_container = new container(*wrap.m_container);

            return *this;
        }

        const type_info_wrap &operator =(type_info_wrap &&wrap)
        {
            container *__temp = wrap.m_container;
            wrap.m_container = m_container;
            m_container = __temp;

            return *this;
        }

        operator const std::type_info &() const
        {
            return *m_container;
        }

        std::string name() const
        {
            return m_container->info.name();
        }

        bool operator ==(const type_info_wrap &wrap) const
        {
            return *m_container == *(wrap.m_container);
        }

        bool operator !=(const type_info_wrap &wrap) const
        {
            return *m_container != *(wrap.m_container);
        }

        bool operator <(const type_info_wrap &wrap)  const
        {
            return *m_container < *(wrap.m_container);
        }

        bool operator >=(const type_info_wrap &wrap) const
        {
            return *m_container >= *(wrap.m_container);
        }

        bool operator >(const type_info_wrap &wrap)  const
        {
            return *m_container > *(wrap.m_container);
        }

        bool operator <=(const type_info_wrap &wrap) const
        {
            return *m_container <= *(wrap.m_container);
        }
    };
}

#endif
