#include "fungus_net_peer_internal.h"

namespace fungus_net
{
    peer_concrete::peer_concrete(const endian_converter &endian,
                                 const payload_factory &m_payload_factory,
                                 message_intercept::map &m_intercept_map,
                                 peer_id m_id,
                                 unified_host::peer *m_unified_peer,
                                 mode m_mode):
        peer_base(m_intercept_map,
                  m_id,
                  m_mode == mode::server          ?
                            state::authenticating :
                            state::connecting
                  ),
        m_mode(m_mode),
        m_unified_peer(m_unified_peer),
        m_timeout_start(timestamp::current_time),
        m_payloads_in(), m_payloads_out(),
        m_payload_factory(m_payload_factory),
        m_message_to_payload_converter(endian),
        m_payload_to_message_converter()
    {}

    peer_concrete::~peer_concrete()
    {
        m_unified_peer->reset();

        while (!m_payloads_in.empty())
        {
            m_payload_factory.destroy(m_payloads_in.front());
            m_payloads_in.pop();
        }

        while (!m_payloads_out.empty())
        {
            m_payload_factory.destroy(m_payloads_out.front());
            m_payloads_out.pop();
        }
    }

    void peer_concrete::push_incoming_message(const incoming_message &m_in)
    {
        bool b_is_payload = false;

        if (m_in.m_message->get_type() == internal_defs::payload_message_type)
        {
            payload_message *m_message = dynamic_cast<payload_message *>(m_in.m_message);
            if (m_message)
            {
                m_payloads_in.push(m_message_to_payload_converter(m_message));
                m_timeout_start = timestamp::current_time;
                b_is_payload = true;
            }
        }

        if (!b_is_payload)
            peer_base::push_incoming_message(m_in);
    }

    bool peer_concrete::send_message(const message *m_message, const peer *m_exclusion)
    {
        bool b_excluded = m_exclusion && !m_exclusion->has_peer(this);

        return (!b_excluded && m_state == state::connected) ?
               m_unified_peer->send(m_message)              :
               false;
    }

    bool peer_concrete::disconnect(uint32_t data, const peer *m_exclusion)
    {
        bool success = false;
        bool b_excluded;

        switch (m_state)
        {
        case state::connecting:
        case state::authenticating:
        case state::connected:
            b_excluded = m_exclusion && m_exclusion->has_peer(this);
            if (!b_excluded)
                success = m_unified_peer->disconnect(data);
        default:
            break;
        };

        return success;
    }
}
