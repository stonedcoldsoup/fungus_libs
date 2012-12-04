#include "fungus_util_thread.h"
#include "fungus_util_condition.h"
#include "fungus_util_mutex.h"

#ifdef FUNGUSUTIL_WIN32
#include <process.h>
#endif

#ifdef FUNGUSUTIL_WIN32
#define CONDITION_EVENT_ONE 0
#define CONDITION_EVENT_ALL 1
#endif

namespace fungus_util
{
#ifdef FUNGUSUTIL_WIN32
condition::condition() : _wait_ctr(0)
{
    _events[CONDITION_EVENT_ONE] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    _events[CONDITION_EVENT_ALL] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    InitializeCriticalSection(&_wait_ctr_lock);
}

condition::~condition()
{
    CloseHandle(_events[CONDITION_EVENT_ONE]);
    CloseHandle(_events[CONDITION_EVENT_ALL]);
    DeleteCriticalSection(&_wait_ctr_lock);
}

void condition::_wait()
{
    int result = WaitForMultipleObjects(2, _events, FALSE, INFINITE);

    EnterCriticalSection(&_wait_ctr_lock);
    -- _wait_ctr;
    bool lastWaiter = (result == (WAIT_OBJECT_0 + CONDITION_EVENT_ALL)) &&
                      (_wait_ctr == 0);
    LeaveCriticalSection(&_wait_ctr_lock);

    if(lastWaiter)
        ResetEvent(_events[CONDITION_EVENT_ALL]);
}

void condition::notify_one()
{
    EnterCriticalSection(&_wait_ctr_lock);
    bool are_waiting = (_wait_ctr > 0);
    LeaveCriticalSection(&_wait_ctr_lock);

    if (are_waiting)
        SetEvent(_events[CONDITION_EVENT_ONE]);
}

void condition::notify_all()
{
    EnterCriticalSection(&_wait_ctr_lock);
    bool are_waiting = (_wait_ctr > 0);
    LeaveCriticalSection(&_wait_ctr_lock);

    if (are_waiting)
        SetEvent(_events[CONDITION_EVENT_ALL]);
}
#endif

#ifdef FUNGUSUTIL_POSIX
} // close the namespace to include another header.

#include "../fungus_util_hash_map.h"

// open it again.
namespace fungus_util
{

static thread::id pthread_handle_to_id(const pthread_t &_ahandle)
{
    typedef
        hash_map<default_hash<pthread_t, unsigned long>>
        hash_map_type;

    static mutex idm_m;
    static hash_map_type ids;
    static unsigned long id_ctr = 0;

    lock guard(idm_m);
    auto it = ids.find(_ahandle);
    if (it == ids.end())
        it = ids.insert(hash_map_type::entry(_ahandle, ++id_ctr));

    return thread::id(it->value);
}
#endif

struct _thread_start_info
{
    void (*_fn)(void *);
    void *args__;
    thread * _m_thread;
};

#ifdef FUNGUSUTIL_WIN32
unsigned WINAPI thread::wrapper_function(void * args__)
#elif defined(FUNGUSUTIL_POSIX)
void *thread::wrapper_function(void * args__)
#endif
{
    _thread_start_info * ti = (_thread_start_info *) args__;

    try
        {ti->_fn(ti->args__);}
    catch(...)
        {std::terminate();}

    lock guard(ti->_m_thread->_data_mutex);
    ti->_m_thread->_not_thread = true;

    delete ti;

    return 0;
}

thread::thread(void (*fn)(void *), void * args__):
    _handle(0), _not_thread(true)
#ifdef FUNGUSUTIL_WIN32
    , w32_thread_id(0)
#endif
{
    start(fn, args__);
}

thread::~thread()
{
    if(joinable())
        std::terminate();
}

void thread::join()
{
    if (joinable())
    {
#ifdef FUNGUSUTIL_WIN32
        WaitForSingleObject(_handle, INFINITE);
#elif defined(FUNGUSUTIL_POSIX)
        pthread_join(_handle, nullptr);
#endif
    }
}

void thread::start(void (*fn)(void *), void *args__)
{
    lock guard(_data_mutex);

    if (joinable()) join();

    _thread_start_info * ti = new _thread_start_info;
    ti->_fn = fn;
    ti->args__ = args__;
    ti->_m_thread = this;

    _not_thread = false;

#ifdef FUNGUSUTIL_WIN32
    _handle = (HANDLE) _beginthreadex(0, 0, wrapper_function, (void *) ti, 0, &w32_thread_id);
#elif defined(FUNGUSUTIL_POSIX)
    if(pthread_create(&_handle, nullptr, wrapper_function, (void *) ti) != 0)
        _handle = 0;
#endif

    if (!_handle)
    {
        _not_thread = true;
        delete ti;
    }
}

bool thread::joinable() const
{
    _data_mutex.lock();
    bool result = !_not_thread;
    _data_mutex.unlock();
    return result;
}

thread::id thread::get_id() const
{
    if(!joinable())
        return id();
#ifdef FUNGUSUTIL_WIN32
    return id((unsigned long) w32_thread_id);
#elif defined(FUNGUSUTIL_POSIX)
    return pthread_handle_to_id(_handle);
#endif
}

unsigned thread::hardware_concurrency()
{
#ifdef FUNGUSUTIL_WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    return (int)sysconf(_SC_NPROC_ONLN);
#else
    return 0;
#endif
}

thread::id this_thread::id()
{
#ifdef FUNGUSUTIL_WIN32
    return thread::id((unsigned long)GetCurrentThreadId());
#elif defined(FUNGUSUTIL_POSIX)
    return pthread_handle_to_id(pthread_self());
#endif
}

}
