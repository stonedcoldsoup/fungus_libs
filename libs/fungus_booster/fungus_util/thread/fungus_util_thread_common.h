#ifndef FUNGUSUTIL_THREAD_COMMON_H
#define FUNGUSUTIL_THREAD_COMMON_H

#include "../fungus_util_common.h"

#include <typeinfo>

#ifdef FUNGUSUTIL_WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <signal.h>
    #include <sched.h>
    #include <unistd.h>
#endif

#endif
