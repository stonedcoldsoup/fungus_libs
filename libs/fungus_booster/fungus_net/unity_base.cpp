#include "fungus_net_unity_base.h"
#include "fungus_net_unity_enet.h"
#include "fungus_net_unity_memory.h"

namespace fungus_net
{
    unified_host_base::default_policy::factory::factory(size_t max_peers, const sec_duration_t *p_timeout_table):
        max_peers(max_peers), p_timeout_table(p_timeout_table)
    {}

    unified_host_base::policy *unified_host_base::default_policy::factory::create() const
    {
        return new default_policy(max_peers, p_timeout_table);
    }

    unified_host_base::default_policy::default_policy(size_t max_peers, const sec_duration_t *p_timeout_table):
        max_peers(max_peers), n_peers(0), p_timeout_table(p_timeout_table)
    {}

    bool unified_host_base::default_policy::grab_peer()
    {
        if (++n_peers > max_peers)
        {
            --n_peers;
            return false;
        }
        else
            return true;
    }

    void unified_host_base::default_policy::drop_peer()
    {
        --n_peers;
    }

    size_t unified_host_base::default_policy::get_max_peers() const
    {
        return max_peers;
    }

    bool unified_host_base::default_policy::timed_out(timeout_period_type period_type, sec_duration_t duration) const
    {
        switch (period_type)
        {
        case connection_timeout:
        case disconnect_timeout:
        case auth_timeout:
            return p_timeout_table[period_type] < duration;
            break;
        default:
            return false;
            break;
        };
    }

    unified_host_base::peer::peer(unified_host_base *parent):
        parent(parent), m_state(state::none)
    {}

    unified_host_base::peer::~peer()
    {
        fungus_util_assert(can_connect(m_state),
            "fungus_net::unified_host_base::peer::~peer(): attempted to destroy peer before disconnecting or resetting!");
    }

    unified_host_base *unified_host_base::peer::get_parent()            const {return parent;}
    unified_host_base::peer::state unified_host_base::peer::get_state() const {return m_state;}

    bool unified_host_base::peer::connect(const ipv4 &m_ipv4,        uint32_t data) {return false;}
    bool unified_host_base::peer::connect(unified_host_base *m_host, uint32_t data) {return false;}

    bool unified_host_base::peer::reset() {return false;}

    unified_host_base::unified_host_base(common_data &m_common_data):
        m_common_data(m_common_data)
    {}

    unified_host_base::~unified_host_base() {}

    unified_host_base::common_data &unified_host_base::get_common_data()
    {
        return m_common_data;
    }

    const unified_host_base::common_data &unified_host_base::get_common_data() const
    {
        return m_common_data;
    }
};
