#ifndef FUNGUSUTIL_THREAD_MUTEX_H
#define FUNGUSUTIL_THREAD_MUTEX_H

#include "fungus_util_thread_common.h"

namespace fungus_util
{
    class mutex
    {
    private:
#ifdef FUNGUSUTIL_WIN32
        CRITICAL_SECTION _handle;
#else
        pthread_mutex_t _handle;
#endif

        friend class condition;

        FUNGUSUTIL_NO_ASSIGN(mutex)
    public:
        mutex()
        {
#ifdef FUNGUSUTIL_WIN32
            InitializeCriticalSection(&_handle);
#else
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&_handle, &attr);
#endif
        }

        ~mutex()
        {
#ifdef FUNGUSUTIL_WIN32
            DeleteCriticalSection(&_handle);
#else
            pthread_mutex_destroy(&_handle);
#endif
        }

        inline void lock()
        {
#ifdef FUNGUSUTIL_WIN32
            EnterCriticalSection(&_handle);
#else
            pthread_mutex_lock(&_handle);
#endif
        }

        inline bool try_lock()
        {
#ifdef FUNGUSUTIL_WIN32
            return TryEnterCriticalSection(&_handle) ? true : false;
#else
            return (pthread_mutex_trylock(&_handle) == 0) ? true : false;
#endif
        }

        inline void unlock()
        {
#ifdef FUNGUSUTIL_WIN32
            LeaveCriticalSection(&_handle);
#else
            pthread_mutex_unlock(&_handle);
#endif
        }
    };

    // a scope lock
    class lock
    {
    private:
        mutex *_mutex;

    public:
        lock(): _mutex(nullptr) {}
        lock(mutex &__mutex)
        {
            __mutex.lock();
            _mutex = &__mutex;
        }

        ~lock()
        {
            if (_mutex) _mutex->unlock();
        }
    };
}

#endif
