#include "fungus_net_common.h"

#ifdef FUNGUSUTIL_WIN32
    #include "enet\enet.h"
#else
    #include <enet/enet.h>
#endif

namespace fungus_net
{
    static bool b_initialized = false;

    bool initialize()
    {
        if (!b_initialized)
        {
            return b_initialized = (enet_initialize() == 0);
        }
        else
            return false;
    }

    bool deinitialize()
    {
        if (b_initialized)
        {
            enet_deinitialize();
            b_initialized = false;
            return true;
        }
        else
            return false;
    }
}
