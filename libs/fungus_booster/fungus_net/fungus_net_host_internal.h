#ifndef FUNGUSNET_HOST_INTERNAL_H
#define FUNGUSNET_HOST_INTERNAL_H

#include "fungus_net_host.h"
#include "fungus_net_peer_internal.h"
#include "fungus_net_authenticator_internal.h"
#include "fungus_net_message_intercept.h"

namespace fungus_net
{
    using namespace fungus_util;

    // TODO: Implement host::impl.
    //
    //       use hash_map to keep track of
    //       active authentications and message
    //       intercepts.
    //
    //       wrap unified host thinly.
    //
    //       make sure to use inlining internally
    //       wherever possible for extra speed boost;
    //       smaller call stack.
    //
    //       handle state transitions based solely on
    //       events from unified_host, except on
    //       completion of an authentication exchange,
    //       which will trigger change from
    //       authenticating to connected (authenticating
    //       is triggered on connected event from
    //       unified_host.
    //
    //       peer::state change triggers:  (<= shows a trigger)
    //
    //          peer::state                 unified_host::peer::state     <host event>
    //          none                     <= none
    //          connecting               <= connecting
    //          authenticating           <= connected
    //          connected                                              <= <authentication completed>
    //          disconnecting            <= disconnecting
    //          disconnected             <= disconnected

    // TODO: Implement host to wrap host::impl.
    // TODO (non critical): Implement message intercept callbacks for added performance/flexibility.
    //                      Also implement similar authentication callbacks that can be mapped by the uint32 data
    //                      sent through connect().

    static inline uint32_t get_callback_flag_from_event_type(host::event::type m_type)
    {
        switch (m_type)
        {
            case host::event::type::error:                  return host::callbacks::implements_on_error;
            case host::event::type::peer_connected:         return host::callbacks::implements_on_peer_connected;
            case host::event::type::peer_disconnected:      return host::callbacks::implements_on_peer_disconnected;
            case host::event::type::peer_rejected:          return host::callbacks::implements_on_peer_rejected;
            case host::event::type::received_auth_payload:  return host::callbacks::implements_on_received_auth_payload;
            case host::event::type::message_intercepted:    return host::callbacks::implements_on_message_intercepted;
        default:
            break;
        };

        return 0;
    }

    static inline host::event::type get_event_type_from_callback_flag(uint32_t flag)
    {
        switch (flag)
        {
            case host::callbacks::implements_on_error:                 return host::event::type::error;
            case host::callbacks::implements_on_peer_connected:        return host::event::type::peer_connected;
            case host::callbacks::implements_on_peer_disconnected:     return host::event::type::peer_disconnected;
            case host::callbacks::implements_on_peer_rejected:         return host::event::type::peer_rejected;
            case host::callbacks::implements_on_received_auth_payload: return host::event::type::received_auth_payload;
            case host::callbacks::implements_on_message_intercepted:   return host::event::type::message_intercepted;
        default:
            break;
        };

        return host::event::type::none;
    }

    class host::impl
    {
    private:
        struct _s_nil {};

        class event_queue
        {
        private:
            callbacks *m_callback_slots[(size_t)event::type::count];
            uint32_t all_flags;

            std::queue<event> m_waiting;
            std::queue<event> m_processed;
        public:
            inline event_queue():
                m_waiting(), m_processed()
            {
                clear_callbacks();
            }

            inline ~event_queue()
            {
                clear_callbacks();
            }

            void process_events();

            inline bool add_callbacks(callbacks *m_callbacks)
            {
                if (all_flags & m_callbacks->flags) return false;
                all_flags |= m_callbacks->flags;

                bool success = true;
                for (uint32_t x = 1; x < callbacks::__max_flag; x <<= 1)
                {
                    if (m_callbacks->flags & x)
                    {
                        callbacks *&m_callbacks_ref = m_callback_slots[(size_t)get_event_type_from_callback_flag(x)];

                        if (m_callbacks_ref != nullptr)
                        {
                            success = false;
                            break;
                        }

                        m_callbacks_ref = m_callbacks;
                    }
                }

                if (!success)
                    all_flags &= ~m_callbacks->flags;

                return success;
            }

            inline void remove_callbacks(callbacks *m_callbacks)
            {
                for (size_t i = 0; i < (size_t)event::type::count; ++i)
                {
                    if (m_callback_slots[i] == m_callbacks)
                        return;
                }

                all_flags &= ~m_callbacks->flags;

                for (uint32_t x = 1; x < callbacks::__max_flag; x <<= 1)
                {
                    if (m_callbacks->flags & x)
                        m_callback_slots[(size_t)get_event_type_from_callback_flag(x)] = nullptr;
                }
            }

            inline void clear_callbacks()
            {
                all_flags = 0;

                for (size_t i = 0; i < (size_t)event::type::count; ++i)
                    m_callback_slots[i] = nullptr;
            }

            inline void push(event &&m_event)
            {
                m_waiting.push(std::move(m_event));
            }

            inline void pop()
            {
                m_processed.pop();
            }

            inline event front() const
            {
                return m_processed.front();
            }

            inline bool empty() const
            {
                return m_processed.empty();
            }
        };

        block_allocator<peer_concrete, 1024> m_peer_concrete_allocator;
        block_allocator<peer_group,    1024> m_peer_group_allocator;

        optional<unified_host>    m_unified_host;
        unified_host::common_data m_common_data;

        peer_group *m_group_all;

        // mapped concrete peers (directly related to 1 low level peer object).
        typedef hash_map<default_hash_no_replace<unified_host::peer *, peer_concrete *, hash_entry_ptr_no_delete>> peer_by_unified_peer_map;
        typedef hash_map<default_hash_no_replace<peer_id,              peer_concrete *, hash_entry_ptr_no_delete>> peer_by_id_map;
        typedef hash_map<default_hash_no_replace<peer_id,              peer_group *,    hash_entry_ptr_no_delete>> group_by_id_map;
        typedef hash_map<default_hash_no_replace<peer_id,              peer *,          hash_entry_ptr_no_delete>> virtual_peer_by_id_map;
        typedef hash_map<default_hash_no_replace<peer_concrete *,      _s_nil                                   >> peer_concrete_map;

        peer_by_unified_peer_map m_peers_by_unified_peer;
        peer_by_id_map           m_peers_by_id;
        peer_concrete_map        m_peers_authenticating;

        // mapped peer groups (virtual peers that represent a group of concrete peers and/or other groups.
        group_by_id_map m_groups_by_id;

        // map of all peers virtualized (concrete/group).
        virtual_peer_by_id_map m_virtual_peers_by_id;

        message_intercept::map m_intercept_map; // map of active message intercepts.
                                                // recursively stacked hash table is used for lookups.
        event_queue            m_event_queue;   // event queue.

        // auth payload factory
        payload_factory m_payload_factory;

        // peer id allocation stuff
        peer_id m_id_ctr;
        std::queue<peer_id> m_recycled_ids;
        std::queue<peer_id> m_free_ids;

        // timeout period table
        sec_duration_t m_timeout_periods[timeout_period_type::count];

        inline peer_id alloc_peer_id();
        inline void    free_peer_id(peer_id m_id);
        inline void    garbage_collect_peer_ids();

        // peer management stuff
        inline peer_concrete *create_peer_concrete(unified_host::peer *m_unified_peer, peer_concrete::mode m_mode);
        inline void destroy_peer_concrete(peer_concrete *m_peer_concrete);

        inline peer_group *create_group_internal(peer_id m_id);

        // unified host event processing
        inline void process_unified_host_connected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete);
        inline void process_unified_host_disconnected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete);
        inline void process_unified_host_rejected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete);
        inline void process_unified_host_event(const unified_host::event &m_unified_host_event);

        // connect forwarding template
        template <unified_host_type __type, typename... argT>
        inline peer_concrete *connect_internal(auth_payload *m_payload, argT&&... argV);

        // produce events
        inline void check_for_auth_payloads();
        inline void check_for_intercepted_messages();
    public:
        impl();

        impl(impl &&m_impl)                  = delete;
        impl(const impl &m_impl)             = delete;

        impl &operator =(impl &&m_impl)      = delete;
        impl &operator =(const impl &m_impl) = delete;

        ~impl();

        bool start(uint32_t flags, size_t max_peers,
                   const networked_host_args &m_net_args  = networked_host_args(),
                   const timeout_period_args &m_time_args = timeout_period_args());
        bool stop();

        bool   is_running()    const;
        size_t get_max_peers() const;

              peer *get_peer(peer_id m_id);
        const peer *get_peer(peer_id m_id) const;

              message_factory_manager &get_message_factory_manager();
        const message_factory_manager &get_message_factory_manager() const;

              endian_converter &get_endian_converter();
        const endian_converter &get_endian_converter() const;

        peer *create_group();
        bool  destroy_group(peer *m_peer);
        bool  destroy_group(peer_id m_id);

        auth_payload *create_auth_payload(auth_status m_status) const;
        void destroy_auth_payload(auth_payload *m_payload)                const;

        peer *connect(auth_payload *m_payload, uint32_t data, const ipv4 &m_ip);
        peer *connect(auth_payload *m_payload, uint32_t data, impl *pimpl_);

        bool send_auth_payload(peer *m_peer, auth_payload *m_payload);
        bool send_auth_payload(peer_id m_id, auth_payload *m_payload);

        bool disconnect(peer *m_peer, uint32_t data, const peer *m_exclusion);
        bool disconnect(peer_id m_id, uint32_t data, peer_id m_exclusion_id);

        bool dispatch();

        bool next_event(event &m_event);
        bool peek_event(event &m_event) const;

        bool    set_message_intercept(peer_id m_id, message_type m_type);
        bool  unset_message_intercept(peer_id m_id, message_type m_type);
        bool is_message_intercept_set(peer_id m_id, message_type m_type) const;

        bool set_callbacks(callbacks *m_callbacks);
        void remove_callbacks(callbacks *m_callbacks);
        void clear_callbacks();
    };
}

#endif
