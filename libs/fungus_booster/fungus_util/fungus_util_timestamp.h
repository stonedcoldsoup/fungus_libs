#ifndef FUNGUSUTIL_TIMESTAMP_H
#define FUNGUSUTIL_TIMESTAMP_H

#include "fungus_util_common.h"

#include <iostream>
#include <ctime>

#ifdef FUNGUSUTIL_WIN32
#	include <windows.h>
#	include <mmsystem.h>
#   include <winsock.h> // timeval is defined in here.

#ifndef FUNGUSUTIL_HAVE_gettimeofday
    /* FILETIME of Jan 1 1970 00:00:00. */
    //static const unsigned __int64 epoch = UINT64CONST(116444736000000000);
    static const unsigned __int64 epoch = 11644473600000000ULL;

    struct timezone
    {
        int  tz_minuteswest; /* minutes W of Greenwich */
        int  tz_dsttime;     /* type of dst correction */
    };

    /*
     * timezone information is stored outside the kernel so tzp isn't used anymore.
     *
     * Note: this function is not for Win32 high precision timing purpose. See
     * elapsed_time().
     */
    FUNGUSUTIL_API int gettimeofday(struct timeval *tp, struct timezone *tzp);
#endif
#else
#   include <sys/time.h>
#endif

namespace fungus_util
{
    typedef long long   usec_duration_t;
    typedef long double  sec_duration_t;

    struct timestamp
    {
        enum initializer
        {
            current_time,
            zero_time
        };

        unsigned long sec;
        unsigned long usec;

        timestamp(initializer init = zero_time): sec(0), usec(0) {if (init == current_time) get();}
        timestamp(const timestamp &stamp): sec(stamp.sec), usec(stamp.usec) {}

        inline timestamp &operator =(const timestamp &stamp)
        {
            sec  = stamp.sec;
            usec = stamp.usec;

            return *this;
        }

        inline timestamp &operator =(initializer init)
        {
            if (init == current_time)
                get();
            else
                (sec = 0, usec = 0);

            return *this;
        }

        timestamp(usec_duration_t duration)
        {
            if (duration == 0)
            {
                sec  = 0;
                usec = 0;
            }
            else
            {
                sec  = duration / 1000000LL;
                usec = duration % 1000000LL;
            }
        }

        inline timestamp &get()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);

            sec = tv.tv_sec;
            usec = tv.tv_usec;

            return *this;
        }
    };

    static inline usec_duration_t operator -(const timestamp &a, const timestamp &b)
    {
        usec_duration_t sdiff = ((usec_duration_t)a.sec - (usec_duration_t)b.sec) * 1000000LL;
        return sdiff + (usec_duration_t)a.usec - (usec_duration_t)b.usec;
    }

    static inline sec_duration_t usec_duration_to_sec(usec_duration_t usecs)
    {
        return (sec_duration_t)usecs / 1000000.0;
    }

    static inline usec_duration_t sec_duration_to_usec(sec_duration_t secs)
    {
        return (usec_duration_t)(secs * 1000000.0);
    }

    static inline std::ostream &operator <<(std::ostream &o, const timestamp &t)
    {
        return o << t.sec << '\'' << t.usec << "\"\"";
    }
}

#endif
