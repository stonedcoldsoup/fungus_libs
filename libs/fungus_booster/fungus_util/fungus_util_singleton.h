#ifndef FUNGUSUTIL_SINGLETON_H
#define FUNGUSUTIL_SINGLETON_H

#include "fungus_util_common.h"
#include "fungus_util_optional.h"

namespace fungus_util
{
    template <typename T>
    class singleton
    {
    protected:
        static T *_instance;

        singleton() = default;
        ~singleton() = default;
    public:
        template <typename... argT>
        static inline T *create(argT&&... argV)
        {
            if (!_instance)
                _instance = new T(std::forward<argT>(argV)...);

            return _instance;
        }

        static inline void destroy()
        {
            if (_instance)
            {
                delete _instance;
                _instance = nullptr;
            }
        }

        static inline T *instance()
        {
            return _instance;
        }
    };

#define FUNGUSUTIL_SINGLETON_INSTANCE(_Type) \
    template<> _Type *fungus_util::singleton<_Type>::_instance = nullptr;
}

#endif
