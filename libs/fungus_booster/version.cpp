#include "fungus_booster.h"

namespace fungus_common
{
    const version_info &get_version_info()
    {
        static version_info m_vinfo{FUNGUSBOOSTER_VERSION_STRING,
                                  {{FUNGUSBOOSTER_VERSION_MINOR,
                                    FUNGUSBOOSTER_VERSION_MAJOR}}};
        return m_vinfo;
    }
}
