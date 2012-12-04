#include "fungus_net_message_intercept.h"
#include "fungus_net_peer.h"

namespace fungus_net
{
    message_intercept::map::map():
        m_big_map(), m_intercepted_messages()
    {}

    message_intercept::map::~map()
    {
        m_big_map.clear();
    }

    void message_intercept::map::clear()
    {
        m_big_map.clear();
    }

    bool message_intercept::map::add(const message_intercept &m_intercept)
    {
        auto it = m_big_map.find(m_intercept.m_id);
        if (it == m_big_map.end())
            it = m_big_map.insert(big_map_type::entry(m_intercept.m_id, little_map_type()));

        auto &m_little_map = it->value;

        auto jt = m_little_map.find(m_intercept.m_type);
        if (jt == m_little_map.end())
        {
            jt = m_little_map.insert(little_map_type::entry(m_intercept.m_type, m_intercept));
            return jt != m_little_map.end();
        }
        else
            return false;
    }

    bool message_intercept::map::remove(const message_intercept &m_intercept)
    {
        auto it = m_big_map.find(m_intercept.m_id);
        if (it == m_big_map.end())
            return false;

        auto &m_little_map = it->value;

        auto jt = m_little_map.find(m_intercept.m_type);
        if (jt == m_little_map.end())
            return false;

        m_little_map.erase(jt);
        return true;
    }

    bool message_intercept::map::has(const message_intercept &m_intercept) const
    {
        auto it = m_big_map.find(m_intercept.m_id);
        if (it == m_big_map.end())
            return false;

        const auto &m_little_map = it->value;
        return m_little_map.find(m_intercept.m_type) != m_little_map.end();
    }

    bool message_intercept::map::intercept(message *m_message, peer_id sender_id, peer *m_peer)
    {
        peer_id  m_id        = m_peer->get_id();
        any_type m_user_data = m_peer->get_user_data();

        auto it = m_big_map.find(m_id);
        if (it == m_big_map.end())
            return false;

        auto &m_little_map = it->value;

        bool b_intercepted = m_little_map.find(m_message->get_type()) != m_little_map.end();

        if (b_intercepted && !m_message->marked_as_read())
        {
            m_message->mark_as_read();
            m_intercepted_messages.push
            (
                intercepted_message
                (
                    message_intercept
                    (
                        m_message->get_type(),
                        m_id
                    ),
                    m_message,
                    sender_id,
                    std::move(m_user_data)
                )
            );
        }

        return b_intercepted;
    }

    bool message_intercept::map::next_intercepted_message(intercepted_message &m_intercepted)
    {
        if (!m_intercepted_messages.empty())
        {
            m_intercepted = m_intercepted_messages.front();
            m_intercepted_messages.pop();

            return true;
        }
        else
            return false;
    }
}
