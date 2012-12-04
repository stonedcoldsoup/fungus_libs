#ifndef FUNGUSCONCURRENCY_COMMON_H
#define FUNGUSCONCURRENCY_COMMON_H

#include "../fungus_util/fungus_util.h"

#ifdef FUNGUS_CONCURRENCY_NO_INLINE
    #define FUNGUSCONCURRENCY_INLINE
#else
    #define FUNGUSCONCURRENCY_INLINE inline
#endif

#ifdef DLL_FUNGUSUTIL
    #ifdef BUILD_FUNGUSCONCURRENCY
        #define FUNGUSCONCURRENCY_API __attribute__ ((dllexport))
    #else
        #define FUNGUSCONCURRENCY_API __attribute__ ((dllimport))
    #endif
#elif SO_FUNGUSUTIL
    #ifdef BUILD_FUNGUSCONCURRENCY
        #define FUNGUSCONCURRENCY_API __attribute__ ((visibility("hidden")))
    #else
        #define FUNGUSCONCURRENCY_API
    #endif
#else
    #define FUNGUSCONCURRENCY_API
#endif

#endif
