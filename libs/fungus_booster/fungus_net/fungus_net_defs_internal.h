#ifndef FUNGUSNET_DEFS_INTERNAL_H
#define FUNGUSNET_DEFS_INTERNAL_H

#include "fungus_net_defs.h"
#include "fungus_net_packet.h"
#include <cstdint>

namespace fungus_net
{
    enum timeout_period_type: uint8_t
    {
        connection_timeout = 0,
        disconnect_timeout = 1,
        auth_timeout       = 2,
        count
    };

    constexpr sec_duration_t timeout_period_map[timeout_period_type::count] =
    {
        default_connection_timeout_period,
        default_disconnect_timeout_period,
        default_auth_timeout_period
    };

    namespace internal_defs
    {
        constexpr uint8_t  i_protocol_channel     = 0;
        constexpr uint8_t  i_user_channel_base    = 1;

        constexpr size_t   aux_peer_slot_count    = 128;
        constexpr size_t   max_peers_limit        = UINT16_MAX - aux_peer_slot_count;
        constexpr size_t   max_user_channel_count = UINT8_MAX  - i_user_channel_base;
        constexpr size_t   all_channel_count      = UINT8_MAX;

        constexpr message_type payload_message_type = INT32_MIN;
    }
};

#endif
