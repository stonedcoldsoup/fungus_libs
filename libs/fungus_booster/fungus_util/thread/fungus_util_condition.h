#ifndef FUNGUSUTIL_THREAD_CONDITION_H
#define FUNGUSUTIL_THREAD_CONDITION_H

#include "fungus_util_thread_common.h"
#include "fungus_util_mutex.h"

namespace fungus_util
{
    class FUNGUSUTIL_API condition
    {
    private:
#ifdef FUNGUSUTIL_WIN32
        void _wait();
        HANDLE _events[2];
        unsigned int _wait_ctr;
        CRITICAL_SECTION _wait_ctr_lock;
#else
        pthread_cond_t _handle;
#endif

        FUNGUSUTIL_NO_ASSIGN(condition)
    public:
#ifdef FUNGUSUTIL_WIN32
        condition();
#else
        condition()
        {
            pthread_cond_init(&_handle, nullptr);
        }
#endif

#ifdef FUNGUSUTIL_WIN32
        ~condition();
#else
        ~condition()
        {
            pthread_cond_destroy(&_handle);
        }
#endif

        inline void wait(mutex &_mutex)
        {
#ifdef FUNGUSUTIL_WIN32
            // Increment number of waiters
            EnterCriticalSection(&_wait_ctr_lock);
            ++_wait_ctr;
            LeaveCriticalSection(&_wait_ctr_lock);

            // Release the mutex while waiting for the condition (will decrease
            // the number of waiters when done)...
            _mutex.unlock();
            _wait();
            _mutex.lock();
#else
            pthread_cond_wait(&_handle, &_mutex._handle);
#endif
        }

#ifdef FUNGUSUTIL_WIN32
        void notify_one();
#else
        inline void notify_one()
        {
            pthread_cond_signal(&_handle);
        }
#endif

#ifdef FUNGUSUTIL_WIN32
        void notify_all();
#else
        inline void notify_all()
        {
            pthread_cond_broadcast(&_handle);
        }
#endif
    };
}

#endif
