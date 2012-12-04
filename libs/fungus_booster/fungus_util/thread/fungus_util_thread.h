#ifndef FUNGUSUTIL_THREAD_THREAD_H
#define FUNGUSUTIL_THREAD_THREAD_H

#include "fungus_util_thread_common.h"
#include "fungus_util_mutex.h"
#include "fungus_util_condition.h"
#include <iostream>

namespace fungus_util
{
    class FUNGUSUTIL_API thread
    {
    public:
#ifdef FUNGUSUTIL_WIN32
        typedef HANDLE native_id;
#else
        typedef pthread_t native_id;
#endif
    private:
        FUNGUSUTIL_NO_ASSIGN(thread);

        native_id _handle;   ///< Thread handle.
        mutable mutex _data_mutex;     ///< Serializer for access to the thread private data.
        bool _not_thread;             ///< True if this object is not a thread of execution.
#ifdef FUNGUSUTIL_WIN32
        unsigned int w32_thread_id;  ///< Unique thread ID (filled out by _beginthreadex).
#endif

        // This is the internal thread wrapper function.
#ifdef FUNGUSUTIL_WIN32
        static unsigned WINAPI wrapper_function(void *args__);
#else
        static void *wrapper_function(void *args__);
#endif
    public:
        class id;

        thread(): _handle(0), _not_thread(true)
#ifdef FUNGUSUTIL_WIN32
            ,w32_thread_id(0)
#endif
        {}

        thread(void (*fn)(void *), void *args__);
        ~thread();

        void start(void (*fn)(void *), void *args__);

        void join();
        bool joinable() const;

        id get_id() const;

        inline native_id get_native_id()
        {
            return _handle;
        }

        static unsigned hardware_concurrency();
    };

    class thread::id
    {
    public:
        id(): m_id(0) {};
        id(unsigned long o_id): m_id(o_id) {};
        id(const id &o_id): m_id(o_id.m_id) {};

        inline id &operator =(const id &o_id)
        {
            m_id = o_id.m_id;
            return *this;
        }

        inline friend bool operator ==(const id &a, const id &b)
        {
            return (a.m_id == b.m_id);
        }

        inline friend bool operator !=(const id &a, const id &b)
        {
            return (a.m_id != b.m_id);
        }

        inline friend bool operator <=(const id &a, const id &b)
        {
            return (a.m_id <= b.m_id);
        }

        inline friend bool operator <(const id &a, const id &b)
        {
            return (a.m_id < b.m_id);
        }

        inline friend bool operator >=(const id &a, const id &b)
        {
            return (a.m_id >= b.m_id);
        }

        inline friend bool operator >(const id &a, const id &b)
        {
            return (a.m_id > b.m_id);
        }

        inline friend std::ostream& operator <<(std::ostream &os, const id &a)
        {
            os << a.m_id;
            return os;
        }
    private:
        unsigned long m_id;
    };

    namespace this_thread
    {
        FUNGUSUTIL_API thread::id id();

        static inline void yield()
        {
#ifdef FUNGUSUTIL_WIN32
            Sleep(0);
#else
            sched_yield();
#endif
        }

        static inline void sleep(long long period_usec)
        {
#ifdef FUNGUSUTIL_WIN32
            Sleep(period_usec / 1000LL);
#else
            usleep(period_usec);
#endif
        }
    };
};

#endif
