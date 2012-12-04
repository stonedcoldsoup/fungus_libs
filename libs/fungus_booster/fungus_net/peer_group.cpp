#include "fungus_net_peer_internal.h"

namespace fungus_net
{
    class __send_enum: public peer::enumerator
    {
    public:
        bool success;
        const message *m_message;

        inline __send_enum(const message *m_message):
            success(true),
            m_message(m_message)
        {}

        inline ~__send_enum()
        {
            delete m_message;
        }

        virtual void operator()(peer *m_peer)
        {
            if (!m_peer->send_message(m_message->copy(), nullptr))
                success = false;
        }
    };

    class __disconnect_enum: public peer::enumerator
    {
    public:
        bool success;
        uint32_t data;

        inline __disconnect_enum(uint32_t data):
            success(true),
            data(data)
        {}

        virtual void operator()(peer *m_peer)
        {
            peer_base *m_peer_base = static_cast<peer_base *>(m_peer);
            if (!m_peer_base->disconnect(data, nullptr))
                success = false;
        }
    };

    peer_group::peer_group(message_intercept::map &m_intercept_map, peer_id m_id):
        peer_base(m_intercept_map, m_id, state::group)
    {}

    peer_group::~peer_group() = default;

    bool peer_group::send_message(const message *m_message, const peer *m_exclusion)
    {
        __send_enum m_enum(m_message);
        enumerate_peers(m_enum, m_exclusion);

        return m_enum.success;
    }

    bool peer_group::disconnect(uint32_t data, const peer *m_exclusion)
    {
        __disconnect_enum m_enum(data);
        enumerate_peers(m_enum, m_exclusion);

        return m_enum.success;
    }
}
