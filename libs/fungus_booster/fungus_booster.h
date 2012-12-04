#ifndef FUNGUSBOOSTER_API
#define FUNGUSBOOSTER_API

#include "fungus_util/fungus_util.h"
#include "fungus_net/fungus_net.h"
#include "fungus_concurrency/fungus_concurrency.h"
//#include "fungus_mail/fungus_mail.h"

#define FUNGUSBOOSTER_VERSION_MAJOR  0
#define FUNGUSBOOSTER_VERSION_MINOR  97
#define FUNGUSBOOSTER_VERSION        (FUNGUSBOOSTER_VERSION_MAJOR << 8 | \
                                      FUNGUSBOOSTER_VERSION_MINOR)

#define FUNGUSBOOSTER_VERSION_STRING "0.97 beta"

namespace fungus_common
{
    /// fungus_booster version information.
    struct version_info
    {
        std::string m_str; /**< The version string. */

        union
        {
            struct
            {
                uint8_t min; /**< Minor version part. */
                uint8_t maj; /**< Major version part. */
            }
            part; /**< Version parts. */

            uint16_t val; /**< Version value. */
        }
        m_version; /**< Numeric version data. */
    };

    /** Get the version_info for fungus_booster.
      *
      * @returns the version_info struct for the version of fungus_booster linked to.
      */
    FUNGUSUTIL_API const version_info &get_version_info();

    inline static bool version_match()
    {
        return get_version_info().m_version.val == FUNGUSBOOSTER_VERSION;
    }
};

#endif
