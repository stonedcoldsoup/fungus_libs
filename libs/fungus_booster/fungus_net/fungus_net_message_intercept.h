#ifndef FUNGUS_NET_MESSAGE_INTERCEPT_H
#define FUNGUS_NET_MESSAGE_INTERCEPT_H

#include "fungus_net_common.h"
#include "fungus_net_message.h"

#include <queue>

namespace fungus_net
{
    using namespace fungus_util;

    class peer;

    struct message_intercept
    {
        message_type m_type;
        peer_id      m_id;

        class map;

        inline message_intercept(): m_type(0), m_id(null_peer_id) {}
        inline message_intercept(message_type m_type, peer_id m_id):
            m_type(m_type), m_id(m_id)
        {}

        inline message_intercept(const message_intercept &m_intercept):
            m_type(m_intercept.m_type),
              m_id(m_intercept.m_id)
        {}
    };

    class message_intercept::map
    {
    public:
        struct intercepted_message
        {
            message_intercept m_intercept;
            message  *m_message;
            peer_id   sender_id;
            any_type  m_user_data;

            intercepted_message():
                m_intercept(), m_message(nullptr), sender_id(null_peer_id), m_user_data()
            {}

            intercepted_message(const message_intercept &m_intercept, message *m_message, peer_id sender_id, any_type &&m_user_data):
                m_intercept(m_intercept), m_message(m_message), sender_id(sender_id), m_user_data(std::move(m_user_data))
            {}

            intercepted_message(intercepted_message &&m_intercepted):
                m_intercept(m_intercepted.m_intercept),
                m_message(m_intercepted.m_message),
                sender_id(m_intercepted.sender_id),
                m_user_data(std::move(m_intercepted.m_user_data))
            {}

            intercepted_message(const intercepted_message &m_intercepted):
                m_intercept(m_intercepted.m_intercept),
                m_message(m_intercepted.m_message),
                sender_id(m_intercepted.sender_id),
                m_user_data(m_intercepted.m_user_data)
            {}

            intercepted_message &operator =(intercepted_message &&m_intercepted)
            {
                m_intercept = m_intercepted.m_intercept;
                m_message   = m_intercepted.m_message;
                sender_id   = m_intercepted.sender_id;
                m_user_data = std::move(m_intercepted.m_user_data);

                return *this;
            }

            intercepted_message &operator =(const intercepted_message &m_intercepted)
            {
                m_intercept = m_intercepted.m_intercept;
                m_message   = m_intercepted.m_message;
                sender_id   = m_intercepted.sender_id;
                m_user_data = m_intercepted.m_user_data;

                return *this;
            }
        };
    private:
        typedef hash_map<default_hash_no_replace<message_type, message_intercept>> little_map_type;
        typedef hash_map<default_hash_no_replace<peer_id,      little_map_type>>   big_map_type;

        big_map_type m_big_map;

        std::queue<intercepted_message> m_intercepted_messages;
    public:
        map();
        ~map();

        void clear();

        bool    add(const message_intercept &m_intercept);
        bool remove(const message_intercept &m_intercept);

        bool    has(const message_intercept &m_intercept) const;

        bool intercept(message *m_message, peer_id sender_id, peer *m_peer);
        bool next_intercepted_message(intercepted_message &m_intercepted);
    };
}

#endif
