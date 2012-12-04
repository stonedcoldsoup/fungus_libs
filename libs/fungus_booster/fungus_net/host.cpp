#include "fungus_net_host_internal.h"

namespace fungus_net
{
    void host::impl::event_queue::process_events()
    {
        while (!m_waiting.empty())
        {
            event m_event = m_waiting.front();
            m_waiting.pop();

            bool b_keep = true;

            callbacks *m_callbacks = m_callback_slots[(size_t)m_event.m_type];
            if (m_callbacks)
            {
                switch (m_event.m_type)
                {
                case event::type::error:
                    if (m_callbacks->flags & callbacks::implements_on_error)
                    {
                        switch (m_callbacks->on_error(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                case event::type::peer_connected:
                    if (m_callbacks->flags & callbacks::implements_on_peer_connected)
                    {
                        switch (m_callbacks->on_peer_connected(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                case event::type::peer_disconnected:
                    if (m_callbacks->flags & callbacks::implements_on_peer_disconnected)
                    {
                        switch (m_callbacks->on_peer_disconnected(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                case event::type::peer_rejected:
                    if (m_callbacks->flags & callbacks::implements_on_peer_rejected)
                    {
                        switch (m_callbacks->on_peer_rejected(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                case event::type::received_auth_payload:
                    if (m_callbacks->flags & callbacks::implements_on_received_auth_payload)
                    {
                        switch (m_callbacks->on_received_auth_payload(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                case event::type::message_intercepted:
                    if (m_callbacks->flags & callbacks::implements_on_message_intercepted)
                    {
                        switch (m_callbacks->on_message_intercepted(m_event))
                        {
                        case callbacks::event_action::discard:
                            b_keep = false;
                        default:
                            break;
                        };
                    }
                    break;
                default:
                    break;
                };
            }

            if (b_keep)
                m_processed.push(std::move(m_event));
        }
    }

    inline peer_id host::impl::alloc_peer_id()
    {
        if (m_free_ids.empty())
            return ++m_id_ctr;
        else
        {
            peer_id m_id = m_free_ids.front();
            m_free_ids.pop();

            return m_id;
        }
    }

    inline void host::impl::free_peer_id(peer_id m_id)
    {
        m_recycled_ids.push(m_id);
    }

    inline void host::impl::garbage_collect_peer_ids()
    {
        while (!m_recycled_ids.empty())
        {
            m_free_ids.push(m_recycled_ids.front());
            m_recycled_ids.pop();
        }
    }

    inline peer_concrete *host::impl::create_peer_concrete(unified_host::peer *m_unified_peer, peer_concrete::mode m_mode)
    {
        peer_id m_id = alloc_peer_id();

        peer_concrete *m_peer_concrete =
            m_peer_concrete_allocator.create
            (
                m_common_data.get_endian_converter(),
                m_payload_factory,
                m_intercept_map,
                m_id,
                m_unified_peer,
                m_mode
            );

        m_peers_by_id.insert(peer_by_id_map::entry(m_id, m_peer_concrete));
        m_peers_by_unified_peer.insert(peer_by_unified_peer_map::entry(m_unified_peer, m_peer_concrete));

        if (m_peer_concrete->get_state() == peer::state::authenticating)
            m_peers_authenticating.insert(peer_concrete_map::entry(m_peer_concrete, _s_nil()));

        m_virtual_peers_by_id.insert(virtual_peer_by_id_map::entry(m_id, m_peer_concrete));

        m_group_all->add_member(m_peer_concrete);
        return m_peer_concrete;
    }

    inline void host::impl::destroy_peer_concrete(peer_concrete *m_peer_concrete)
    {
        peer_id m_id                       = m_peer_concrete->get_id();
        unified_host::peer *m_unified_peer = m_peer_concrete->get_unified_peer();

        m_group_all->remove_member(m_peer_concrete);

        m_peers_by_id.erase(m_id);
        m_peers_by_unified_peer.erase(m_peer_concrete->get_unified_peer());

        if (m_peer_concrete->get_state() == peer::state::authenticating)
            m_peers_authenticating.erase(m_peer_concrete);

        m_virtual_peers_by_id.erase(m_id);

        free_peer_id(m_id);

        m_peer_concrete_allocator.destroy(m_peer_concrete);
        m_unified_host->destroy_peer(m_unified_peer);
    }

    inline peer_group *host::impl::create_group_internal(peer_id m_id)
    {
        peer_group *m_peer_group =
            m_peer_group_allocator.create
            (
                m_intercept_map,
                m_id
            );

        m_groups_by_id.insert(group_by_id_map::entry(m_id, m_peer_group));
        m_virtual_peers_by_id.insert(virtual_peer_by_id_map::entry(m_id, m_peer_group));

        return m_peer_group;
    }

    inline void host::impl::process_unified_host_connected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete)
    {
        if (m_peer_concrete)
        {
            m_peer_concrete->set_state(peer::state::authenticating);
            m_peers_authenticating.insert(peer_concrete_map::entry(m_peer_concrete, _s_nil()));
        }
        else
            m_peer_concrete = create_peer_concrete(m_unified_host_event.m_peer, peer_concrete::mode::server);

        m_event_queue.push(event(event::type::peer_connected,
                                 m_peer_concrete->get_id(),
                                 m_peer_concrete->get_user_data(),
                                 m_unified_host_event.data));
    }

    inline void host::impl::process_unified_host_disconnected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete)
    {
        if (m_peer_concrete)
        {
            m_event_queue.push(event(event::type::peer_disconnected,
                                     m_peer_concrete->get_id(),
                                     m_peer_concrete->get_user_data(),
                                     m_unified_host_event.data));

            destroy_peer_concrete(m_peer_concrete);
        }
        else
            m_event_queue.push(event(event::type::error,
                                     null_peer_id,
                                     any_type(),
                                     event::unknown_peer_disconnected));
    }

    inline void host::impl::process_unified_host_rejected(const unified_host::event &m_unified_host_event, peer_concrete *m_peer_concrete)
    {
        if (m_peer_concrete)
        {
            event::rejection_reason rejection_reason = event::rejection_timed_out;
            switch (m_unified_host_event.data)
            {
            case reject_reason_host_deny:
                rejection_reason = event::rejection_denied;
            default:
                break;
            };

            m_event_queue.push(event(event::type::peer_rejected,
                                     m_peer_concrete->get_id(),
                                     m_peer_concrete->get_user_data(),
                                     rejection_reason));

            destroy_peer_concrete(m_peer_concrete);
        }
        else
            m_event_queue.push(event(event::type::error,
                                     null_peer_id,
                                     any_type(),
                                     event::unknown_peer_rejected));
    }

    inline void host::impl::process_unified_host_event(const unified_host::event &m_unified_host_event)
    {
        peer_concrete *m_peer_concrete = nullptr;

        auto it = m_peers_by_unified_peer.find(m_unified_host_event.m_peer);
        if (it != m_peers_by_unified_peer.end())
            m_peer_concrete = static_cast<peer_concrete *>(it->value);

        switch (m_unified_host_event.m_type)
        {
        case unified_host::event::type::connected:
            process_unified_host_connected(m_unified_host_event, m_peer_concrete);
            break;
        case unified_host::event::type::disconnected:
            process_unified_host_disconnected(m_unified_host_event, m_peer_concrete);
            break;
        case unified_host::event::type::rejected:
            process_unified_host_rejected(m_unified_host_event, m_peer_concrete);
            break;
        default:
            break;
        };
    }

    template <unified_host_type __type, typename... argT>
    inline peer_concrete *host::impl::connect_internal(auth_payload *m_payload, argT&&... argV)
    {
        unified_host::peer *m_unified_peer = m_unified_host->new_peer(__type);

        if (!m_unified_peer->connect(std::forward<argT>(argV)...))
        {
            m_unified_host->destroy_peer(m_unified_peer);
            return nullptr;
        }

        peer_concrete *m_peer_concrete = create_peer_concrete(m_unified_peer, peer_concrete::mode::client);

        if (m_peer_concrete && !m_peer_concrete->send_payload(m_payload))
        {
            delete m_peer_concrete;
            m_payload_factory.destroy(m_payload);
            m_peer_concrete = nullptr;
        }

        return m_peer_concrete;
    }

    inline void host::impl::check_for_auth_payloads()
    {
        std::queue<peer_concrete *> m_peers_finished_authenticating;

        for (auto &entry: m_peers_authenticating)
        {
            peer_concrete *m_peer_concrete = entry.key;

            if (m_common_data.get_policy().timed_out(auth_timeout, m_peer_concrete->auth_timeout_period()))
            {
                m_event_queue.push(event(event::type::auth_timeout,
                                         m_peer_concrete->get_id(),
                                         m_peer_concrete->get_user_data(), 0));

                destroy_peer_concrete(m_peer_concrete);
            }
            else
            {
                auth_payload *m_payload;
                while ((m_payload = m_peer_concrete->next_payload()))
                {
                    m_event_queue.push(event(m_peer_concrete->get_id(),
                                             m_peer_concrete->get_user_data(),
                                             m_payload));

                    switch (m_payload->get_status())
                    {
                    case auth_status::success:
                        m_peer_concrete->set_state(peer::state::connected);
                        m_peers_finished_authenticating.push(m_peer_concrete);
                        break;
                    case auth_status::failure:
                        destroy_peer_concrete(m_peer_concrete);
                        break;
                    default:
                        break;
                    };
                }
            }
        }

        while (!m_peers_finished_authenticating.empty())
        {
            m_peers_authenticating.erase(m_peers_finished_authenticating.front());
            m_peers_finished_authenticating.pop();
        }
    }

    inline void host::impl::check_for_intercepted_messages()
    {
        message_intercept::map::intercepted_message m_intercepted;
        while (m_intercept_map.next_intercepted_message(m_intercepted))
        {
            m_event_queue.push(event(m_intercepted.m_intercept.m_id,
                                     m_intercepted.m_user_data,
                                     peer::incoming_message(m_intercepted.m_message,
                                                            m_intercepted.sender_id)));
        }
    }

    host::impl::impl():
        // allocators
        m_peer_concrete_allocator(),
        m_peer_group_allocator(),

        // low level unified host
        m_unified_host(),
        m_common_data(0),

        // special group "all" (automatically managed)
        m_group_all(nullptr),

        // peer/group maps
        m_peers_by_unified_peer(),
        m_peers_by_id(),
        m_peers_authenticating(),
        m_groups_by_id(),
        m_virtual_peers_by_id(),

        // message intercept map
        m_intercept_map(),

        // event queue
        m_event_queue(),

        // payload factory
        m_payload_factory(m_common_data.get_endian_converter()),

        // peer id allocation stuff
        m_id_ctr(0),
        m_recycled_ids(),
        m_free_ids()
    {}

    host::impl::~impl()
    {
        if (m_unified_host) stop();
    }

    bool host::impl::start(uint32_t flags, size_t max_peers, const networked_host_args &m_net_args, const timeout_period_args &m_time_args)
    {
        if (m_unified_host) return false;

        m_timeout_periods[connection_timeout] = m_time_args.connection_timeout_period;
        m_timeout_periods[disconnect_timeout] = m_time_args.disconnect_timeout_period;
        m_timeout_periods[auth_timeout]       = m_time_args.auth_timeout_period;

        m_common_data.get_message_factory_manager().clear_factories();
        m_common_data.get_message_factory_manager().add_factory(payload_message::factory());
        m_common_data.get_endian_converter().lazy_register_numeric_types();
        m_common_data.set_policy(unified_host::default_policy::factory(max_peers, m_timeout_periods));

        uint32_t m_unified_host_flags =
            ((flags & flag_support_memory_connection)    ? unified_host_flag_memory    : 0)|
            ((flags & flag_support_networked_connection) ? unified_host_flag_networked : 0);

        bool success = m_unified_host.create(m_common_data,
                                             m_unified_host_flags, m_net_args.m_ipv4,
                                             m_net_args.in_bandwidth, m_net_args.out_bandwidth);

        if (success)
            m_group_all = create_group_internal(all_peer_id);

        m_peers_by_id.insert(peer_by_id_map::entry(null_peer_id, nullptr));
        m_groups_by_id.insert(group_by_id_map::entry(null_peer_id, nullptr));
        m_virtual_peers_by_id.insert(virtual_peer_by_id_map::entry(null_peer_id, nullptr));

        return success;
    }

    bool host::impl::stop()
    {
        bool success = m_unified_host;

        if (success)
        {
            for (auto &entry: m_groups_by_id)
            {
                if (entry.value != nullptr)
                    m_peer_group_allocator.destroy(entry.value);
            }

            for (auto &entry: m_peers_by_id)
            {
                if (entry.value != nullptr)
                    m_peer_concrete_allocator.destroy(entry.value);
            }

            m_peers_by_unified_peer.clear();
            m_peers_by_id.clear();
            m_peers_authenticating.clear();
            m_groups_by_id.clear();
            m_virtual_peers_by_id.clear();
            m_intercept_map.clear();

            event m_event;
            while (next_event(m_event));
            m_event_queue.clear_callbacks();

            m_unified_host->destroy_all_peers();
            m_unified_host.destroy();

            m_group_all = nullptr;

            garbage_collect_peer_ids();
        }

        return success;
    }

    bool host::impl::is_running() const
    {
        return m_unified_host;
    }

    size_t host::impl::get_max_peers() const
    {
        return m_common_data.get_max_peers();
    }

    peer *host::impl::get_peer(peer_id m_id)
    {
        if (!m_unified_host) return NULL;

        auto it = m_virtual_peers_by_id.find(m_id);
        return it != m_virtual_peers_by_id.end() ? it->value : nullptr;
    }

    const peer *host::impl::get_peer(peer_id m_id) const
    {
        if (!m_unified_host) return NULL;

        auto it = m_virtual_peers_by_id.find(m_id);
        return it != m_virtual_peers_by_id.end() ? it->value : nullptr;
    }

    message_factory_manager &host::impl::get_message_factory_manager()
    {
        return m_common_data.get_message_factory_manager();
    }

    const message_factory_manager &host::impl::get_message_factory_manager() const
    {
        return m_common_data.get_message_factory_manager();
    }

    endian_converter &host::impl::get_endian_converter()
    {
        return m_common_data.get_endian_converter();
    }

    const endian_converter &host::impl::get_endian_converter() const
    {
        return m_common_data.get_endian_converter();
    }

    peer *host::impl::create_group()
    {
        if (!m_unified_host) return NULL;

        return create_group_internal(alloc_peer_id());
    }

    bool host::impl::destroy_group(peer *m_peer)
    {
        if (!m_unified_host) return false;
        if (!m_peer)         return false;

        bool success = false;

        peer_id m_id;
        peer_group *m_peer_group;
        switch (m_peer->get_state())
        {
        case peer::state::group:
            m_id = m_peer->get_id();

            m_groups_by_id.erase(m_id);
            m_virtual_peers_by_id.erase(m_id);

            free_peer_id(m_id);

            m_peer_group = static_cast<peer_group *>(m_peer);
            m_peer_group_allocator.destroy(m_peer_group);

            success = true;
        default:
            break;
        };

        return success;
    }

    bool host::impl::destroy_group(peer_id m_id)
    {
        auto it = m_groups_by_id.find(m_id);
        return it != m_groups_by_id.end() ?
               destroy_group(it->value)   :
               false;
    }

    auth_payload *host::impl::create_auth_payload(auth_status m_status) const
    {
        return m_payload_factory.create(m_status);
    }

    void host::impl::destroy_auth_payload(auth_payload *m_payload) const
    {
        m_payload_factory.destroy(m_payload);
    }

    peer *host::impl::connect(auth_payload *m_payload, uint32_t data, const ipv4 &m_ip)
    {
        if (m_unified_host)
            return connect_internal<unified_host_type::networked>(m_payload, m_ip, data);
        else
            return nullptr;
    }

    peer *host::impl::connect(auth_payload *m_payload, uint32_t data, impl *pimpl_)
    {
        if (m_unified_host)
            return connect_internal<unified_host_type::memory>(m_payload, pimpl_->m_unified_host, data);
        else
            return nullptr;
    }

    bool host::impl::send_auth_payload(peer *m_peer, auth_payload *m_payload)
    {
        if (!m_unified_host)                           return false;
        if (!m_peer)                                   return false;
        if (m_peer->get_state() == peer::state::group) return false;

        peer_concrete *m_peer_concrete = static_cast<peer_concrete *>(m_peer);
        bool success = m_peer_concrete->send_payload(m_payload);

        if (!success)
            m_payload_factory.destroy(m_payload);

        return success;
    }

    bool host::impl::send_auth_payload(peer_id m_id, auth_payload *m_payload)
    {
        if (!m_unified_host) return false;

        auto it = m_peers_by_id.find(m_id);
        return it != m_peers_by_id.end()               ?
               send_auth_payload(it->value, m_payload) :
               false;
    }

    bool host::impl::disconnect(peer *m_peer, uint32_t data, const peer *m_exclusion)
    {
        static const std::string _m_assert_string =
            make_string
            (
                "host::impl::disconnect(): The uint32_t sent on disconnect must not be equal to ",
                (uint32_t)-reject_reason_host_deny,
                ".\n                          It is reserved for internal use."
            );

        if (!m_unified_host) return false;
        if (!m_peer)         return false;

        fungus_util_assert(data != (uint32_t)-reject_reason_host_deny, _m_assert_string);

        peer_base *m_peer_base = static_cast<peer_base *>(m_peer);
        return m_peer_base->disconnect(data, m_exclusion);
    }

    bool host::impl::disconnect(peer_id m_id, uint32_t data, peer_id m_exclusion_id)
    {
        if (!m_unified_host) return false;

        auto it = m_virtual_peers_by_id.find(m_id);
        auto jt = m_virtual_peers_by_id.find(m_exclusion_id);

        return it != m_virtual_peers_by_id.end()     &&
               jt != m_virtual_peers_by_id.end()      ?
               disconnect(it->value, data, jt->value) :
               false;
    }

    bool host::impl::dispatch()
    {
        bool success = false;

        if (m_unified_host)
        {
            garbage_collect_peer_ids();

            m_unified_host->dispatch();

            unified_host::event m_unified_host_event;
            while (m_unified_host->next_event(m_unified_host_event))
                process_unified_host_event(m_unified_host_event);

            for (auto &entry: m_peers_by_id)
            {
                peer_concrete *m_peer_concrete = entry.value;
                if (m_peer_concrete == nullptr) continue;

                unified_host::peer *m_unified_peer = m_peer_concrete->get_unified_peer();

                message *m_message;
                peer_id  m_id = m_peer_concrete->get_id();
                while ((m_message = m_unified_peer->receive()))
                    m_peer_concrete->push_incoming_message(peer::incoming_message(m_message, m_id));
            }

            check_for_auth_payloads();
            check_for_intercepted_messages();

            m_event_queue.process_events();

            success = true;
        }

        return success;
    }

    bool host::impl::next_event(event &m_event)
    {
        bool success = peek_event(m_event);
        if (success) m_event_queue.pop();

        return success;
    }

    bool host::impl::peek_event(event &m_event) const
    {
        if (!m_event_queue.empty())
        {
            m_event = m_event_queue.front();
            return true;
        }
        else
            return false;
    }

    bool host::impl::set_message_intercept(peer_id m_id, message_type m_type)
    {
        return m_unified_host && m_intercept_map.add(message_intercept(m_type, m_id));
    }

    bool host::impl::unset_message_intercept(peer_id m_id, message_type m_type)
    {
        return m_unified_host && m_intercept_map.remove(message_intercept(m_type, m_id));
    }

    bool host::impl::is_message_intercept_set(peer_id m_id, message_type m_type) const
    {
        return m_unified_host && m_intercept_map.has(message_intercept(m_type, m_id));
    }

    bool host::impl::set_callbacks(callbacks *m_callbacks)
    {
        return m_unified_host && m_event_queue.add_callbacks(m_callbacks);
    }

    void host::impl::remove_callbacks(callbacks *m_callbacks)
    {
        if (m_unified_host) m_event_queue.remove_callbacks(m_callbacks);
    }

    void host::impl::clear_callbacks()
    {
        if (m_unified_host) m_event_queue.clear_callbacks();
    }

    host::event::content::content() = default;

    host::event::content::content(uint32_t data):           data(data)           {}
    host::event::content::content(message *m_message):      m_message(m_message) {}
    host::event::content::content(auth_payload *m_payload): m_payload(m_payload) {}

    host::event::event():
        m_type(type::none), m_id(null_peer_id), m_user_data(), m_content()
    {}

    host::event::event(event &&m_event)
    {
        *this = std::move(m_event);
    }

    host::event::event(const event &m_event)
    {
        *this = m_event;
    }

    host::event &host::event::operator =(host::event &&m_event)
    {
        m_type      = m_event.m_type;
        m_id        = m_event.m_id;
        m_sender_id = m_event.m_sender_id;
        m_user_data = std::move(m_event.m_user_data);
        m_content   = m_event.m_content;

        return *this;
    }

    host::event &host::event::operator =(const host::event &m_event)
    {
        m_type      = m_event.m_type;
        m_id        = m_event.m_id;
        m_sender_id = m_event.m_sender_id;
        m_user_data = m_event.m_user_data;
        m_content   = m_event.m_content;

        return *this;
    }

    host::event::event(peer_id m_id, const any_type &m_user_data, auth_payload *m_payload):
        m_type(type::received_auth_payload), m_id(m_id), m_sender_id(null_peer_id), m_user_data(m_user_data), m_content(m_payload)
    {}

    host::event::event(peer_id m_id, const any_type &m_user_data, const peer::incoming_message &m_in_message):
        m_type(type::message_intercepted), m_id(m_id), m_sender_id(m_in_message.sender_id),
        m_user_data(m_user_data), m_content(m_in_message.m_message)
    {}

    host::event::event(type m_type, peer_id m_id, const any_type &m_user_data, uint32_t data):
        m_type(m_type), m_id(m_id), m_sender_id(null_peer_id), m_user_data(m_user_data), m_content(data)
    {}

    host::callbacks::callbacks(uint32_t flags):
        flags(flags)
    {}

    host::callbacks::event_action host::callbacks::on_error(const event &m_event)                 {return event_action::keep;}
    host::callbacks::event_action host::callbacks::on_peer_connected(const event &m_event)        {return event_action::keep;}
    host::callbacks::event_action host::callbacks::on_peer_disconnected(const event &m_event)     {return event_action::keep;}
    host::callbacks::event_action host::callbacks::on_peer_rejected(const event &m_event)         {return event_action::keep;}
    host::callbacks::event_action host::callbacks::on_received_auth_payload(const event &m_event) {return event_action::keep;}
    host::callbacks::event_action host::callbacks::on_message_intercepted(const event &m_event)   {return event_action::keep;}

    host::host():
        pimpl_(nullptr), m()
    {
        pimpl_ = new impl();
    }

    host::host(host &&m_host):
        pimpl_(nullptr), m()
    {
        *this = std::move(m_host);
    }

    host &host::operator =(host &&m_host)
    {
        lock guard0(m);
        lock guard1(m_host.m);

        if (pimpl_)
            delete pimpl_;

        pimpl_ = m_host.pimpl_;
        m_host.pimpl_ = nullptr;

        return *this;
    }

    host::~host()
    {
        if (pimpl_)
            delete pimpl_;
    }

    bool host::is_moved() const
    {
        lock guard(m);
        return pimpl_ == nullptr;
    }

    bool host::start(uint32_t flags, size_t max_peers,
                     const networked_host_args &m_net_args,
                     const timeout_period_args &m_time_args)                                {lock guard(m); return pimpl_ && pimpl_->start(flags, max_peers, m_net_args, m_time_args);}
    bool host::stop()                                                                       {lock guard(m); return pimpl_ && pimpl_->stop();}

    bool   host::is_running()    const                                                      {lock guard(m); return pimpl_ && pimpl_->is_running();}
    size_t host::get_max_peers() const                                                      {lock guard(m); return pimpl_ && pimpl_->get_max_peers();}

          peer *host::get_peer(peer_id m_id)                                                {lock guard(m); return pimpl_ ? pimpl_->get_peer(m_id) : nullptr;}
    const peer *host::get_peer(peer_id m_id) const                                          {lock guard(m); return pimpl_ ? pimpl_->get_peer(m_id) : nullptr;}

          message_factory_manager &host::get_message_factory_manager()                      {lock guard(m); static message_factory_manager __temp_mfm; return pimpl_ ? pimpl_->get_message_factory_manager() : __temp_mfm;}
    const message_factory_manager &host::get_message_factory_manager() const                {lock guard(m); static message_factory_manager __temp_mfm; return pimpl_ ? pimpl_->get_message_factory_manager() : __temp_mfm;}

          endian_converter &host::get_endian_converter()                                    {lock guard(m); static endian_converter __temp_ec; return pimpl_ ? pimpl_->get_endian_converter() : __temp_ec;}
    const endian_converter &host::get_endian_converter() const                              {lock guard(m); static endian_converter __temp_ec; return pimpl_ ? pimpl_->get_endian_converter() : __temp_ec;}

    peer *host::create_group()                                                              {lock guard(m); return pimpl_ ?  pimpl_->create_group() : nullptr;}
    bool  host::destroy_group(peer *m_peer)                                                 {lock guard(m); return pimpl_ && pimpl_->destroy_group(m_peer);}
    bool  host::destroy_group(peer_id m_id)                                                 {lock guard(m); return pimpl_ && pimpl_->destroy_group(m_id);}

    auth_payload *host::create_auth_payload(auth_status m_status) const                     {lock guard(m); return pimpl_ ? pimpl_->create_auth_payload(m_status) : nullptr;}
    void host::destroy_auth_payload(auth_payload *m_payload)      const                     {lock guard(m);    if (pimpl_)  pimpl_->destroy_auth_payload(m_payload);}

    peer *host::connect(auth_payload *m_payload, uint32_t data, const ipv4 &m_ip)
    {
        lock guard(m);
        return pimpl_ ? pimpl_->connect(m_payload, data, m_ip) : nullptr;
    }

    peer *host::connect(auth_payload *m_payload, uint32_t data, host *m_host)
    {
        lock guard0(m);
        lock guard1(m_host->m);
        return (pimpl_ && m_host->pimpl_)                        ?
                pimpl_->connect(m_payload, data, m_host->pimpl_) :
                nullptr;
    }

    bool host::send_auth_payload(peer *m_peer, auth_payload *m_payload)                     {lock guard(m); return pimpl_ && pimpl_->send_auth_payload(m_peer, m_payload);}
    bool host::send_auth_payload(peer_id m_id, auth_payload *m_payload)                     {lock guard(m); return pimpl_ && pimpl_->send_auth_payload(m_id, m_payload);}

    bool host::disconnect(peer *m_peer, uint32_t data, const peer *m_exclusion)             {lock guard(m); return pimpl_ && pimpl_->disconnect(m_peer, data, m_exclusion);}
    bool host::disconnect(peer_id m_id, uint32_t data, peer_id m_exclusion_id)              {lock guard(m); return pimpl_ && pimpl_->disconnect(m_id, data, m_exclusion_id);}

    bool host::dispatch()                                                                   {lock guard(m); return pimpl_ && pimpl_->dispatch();}

    bool host::next_event(event &m_event)                                                   {lock guard(m); return pimpl_ && pimpl_->next_event(m_event);}
    bool host::peek_event(event &m_event) const                                             {lock guard(m); return pimpl_ && pimpl_->peek_event(m_event);}

    bool    host::set_message_intercept(peer_id m_id, message_type m_type)                  {lock guard(m); return pimpl_ && pimpl_->set_message_intercept(m_id, m_type);}
    bool  host::unset_message_intercept(peer_id m_id, message_type m_type)                  {lock guard(m); return pimpl_ && pimpl_->unset_message_intercept(m_id, m_type);}
    bool host::is_message_intercept_set(peer_id m_id, message_type m_type) const            {lock guard(m); return pimpl_ && pimpl_->is_message_intercept_set(m_id, m_type);}

    bool host::set_callbacks(callbacks *m_callbacks)                                        {lock guard(m); return pimpl_ && pimpl_->set_callbacks(m_callbacks);}
    void host::remove_callbacks(callbacks *m_callbacks)                                     {lock guard(m);    if (pimpl_)   pimpl_->remove_callbacks(m_callbacks);}
    void host::clear_callbacks()                                                            {lock guard(m);    if (pimpl_)   pimpl_->clear_callbacks();}
}
