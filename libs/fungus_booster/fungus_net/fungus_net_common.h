#ifndef FUNGUSNET_COMMON_H
#define FUNGUSNET_COMMON_H

#include "../fungus_util/fungus_util.h"
#include "fungus_net_defs.h"

namespace fungus_net
{
    /** @defgroup fungus_net Global Initialization
      * @{
      */

    /** Initialize fungus_net.  This must be called before
      * calling host::start() on any host.
      *
      * @retval true  on success
      * @retval false on failure
      */
    FUNGUSNET_API bool initialize();

    /** Deinitialize fungus_net.  This must be called before
      * the library is unloaded (usually at exit time), but
      * not until every host has been stopped (host::stop()).
      *
      * @retval true  on success
      * @retval false on failure
      */
    FUNGUSNET_API bool deinitialize();

    /** @} */
}

#endif
