#ifndef FUNGUSNET_PEER_INTERNAL_H
#define FUNGUSNET_PEER_INTERNAL_H

#include "fungus_net_common.h"
#include "fungus_net_peer.h"
#include "fungus_net_unity.h"

#include "fungus_net_authenticator_internal.h"
#include "fungus_net_message_intercept.h"

#include <queue>

namespace fungus_net
{
    using namespace fungus_util;

    class peer_base: public peer, public multi_tree_node<peer_base>
    {
    protected:
        message_intercept::map &m_intercept_map;

        state    m_state;
        peer_id  m_id;
        any_type m_data;

        std::queue<incoming_message> m_message_queue_in;
    public:
        peer_base(peer_base &&m_peer)                  = delete;
        peer_base(const peer_base &m_peer)             = delete;

        peer_base &operator =(peer_base &&m_peer)      = delete;
        peer_base &operator =(const peer_base &m_peer) = delete;

        peer_base(message_intercept::map &m_intercept_map, peer_id m_id, state m_state);

        virtual ~peer_base();

        virtual void push_incoming_message(const incoming_message &m_in);

        virtual peer_id  get_id()        const;
        virtual state    get_state()     const;

        virtual void     set_user_data(any_type &&data);
        virtual void     set_user_data(const any_type &data);
        virtual any_type get_user_data() const;

        virtual size_t   count_groups()  const;
        virtual size_t   count_members() const;
        virtual size_t   count_peers()   const;

        virtual void     enumerate_groups (enumerator &m_enum);
        virtual void     enumerate_members(enumerator &m_enum);
        virtual void     enumerate_peers  (enumerator &m_enum, const peer *m_exclusion = nullptr);

        virtual void     enumerate_groups (const_enumerator &m_enum) const;
        virtual void     enumerate_members(const_enumerator &m_enum) const;
        virtual void     enumerate_peers  (const_enumerator &m_enum, const peer *m_exclusion = nullptr) const;

        virtual bool     add_member(peer *m_peer);
        virtual bool     remove_member(peer *m_peer);

        virtual bool     has_member(peer *m_peer) const;
        virtual bool     has_peer(peer *m_peer)   const;

        virtual bool     receive_message(incoming_message &m_in);
        virtual bool     disconnect(uint32_t data, const peer *m_exclusion = nullptr) = 0;

        virtual void     discard_message();
        virtual void     discard_all_messages();
    };

    class peer_group: public peer_base
    {
    public:
        peer_group(peer_group &&m_group)                  = delete;
        peer_group(const peer_group &m_group)             = delete;

        peer_group &operator =(peer_group &&m_group)      = delete;
        peer_group &operator =(const peer_group &m_group) = delete;

        peer_group(message_intercept::map &m_intercept_map, peer_id m_id);

        virtual ~peer_group();

        virtual bool send_message(const message *m_message, const peer *m_exclusion = nullptr);
        virtual bool disconnect(uint32_t data, const peer *m_exclusion = nullptr);
    };

    class peer_concrete: public peer_base
    {
    public:
        enum class mode
        {
            server,
            client
        };

    protected:
        mode m_mode;
        unified_host::peer *m_unified_peer;

        timestamp m_timeout_start;

        std::queue<auth_payload *> m_payloads_in;
        std::queue<auth_payload *> m_payloads_out;

        const payload_factory &m_payload_factory;

        message_to_payload_converter m_message_to_payload_converter;
        payload_to_message_converter m_payload_to_message_converter;

    public:
        peer_concrete(peer_concrete &&m_peer)                  = delete;
        peer_concrete(const peer_concrete &m_peer)             = delete;

        peer_concrete &operator =(peer_concrete &&m_peer)      = delete;
        peer_concrete &operator =(const peer_concrete &m_peer) = delete;

        peer_concrete(const endian_converter &endian,
                      const payload_factory &m_payload_factory,
                      message_intercept::map &m_intercept_map,
                      peer_id m_id,
                      unified_host::peer *m_unified_peer,
                      mode m_mode);

        virtual ~peer_concrete();

        FUNGUSUTIL_ALWAYS_INLINE inline
        sec_duration_t auth_timeout_period()
        {
            return m_state == state::authenticating                                           ?
                   usec_duration_to_sec(timestamp(timestamp::current_time) - m_timeout_start) :
                   0.0;
        }

        virtual void push_incoming_message(const incoming_message &m_in);

        virtual bool send_message(const message *m_message, const peer *m_exclusion = nullptr);
        virtual bool disconnect(uint32_t data, const peer *m_exclusion = nullptr);

        FUNGUSUTIL_ALWAYS_INLINE inline
        unified_host::peer *get_unified_peer()
        {
            return m_unified_peer;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        void set_state(state m_state)
        {
            if (this->m_state == state::connecting &&
                      m_state == state::authenticating)
            {
                while (!m_payloads_out.empty())
                {
                    m_unified_peer->send(m_payload_to_message_converter(m_payloads_out.front()));
                    m_payloads_out.pop();
                }
            }

            this->m_state = m_state;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        auth_payload *next_payload()
        {
            auth_payload *m_payload = nullptr;
            if (!m_payloads_in.empty())
            {
                m_payload = m_payloads_in.front();
                m_payloads_in.pop();
            }

            return m_payload;
        }

        FUNGUSUTIL_ALWAYS_INLINE inline
        bool send_payload(auth_payload *m_payload)
        {
            bool success = true;
            switch (m_state)
            {
            case state::connected:
            case state::authenticating:
                m_unified_peer->send(m_payload_to_message_converter(m_payload));
                break;
            case state::connecting:
                m_payloads_out.push(m_payload);
                break;
            default:
                success = false;
                break;
            };

            return success;
        }
    };
}

#endif
