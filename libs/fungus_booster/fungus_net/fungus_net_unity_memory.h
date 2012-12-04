#ifndef FUNGUSNET_UNITY_MEMORY_H
#define FUNGUSNET_UNITY_MEMORY_H

#include "fungus_net_unity_base.h"
#include "../fungus_concurrency/fungus_concurrency_comm_internal.h"
#include <cfloat>

namespace fungus_net
{
    class __memory_host
    {
    public:
        struct __discard_functor
        {
            FUNGUSUTIL_ALWAYS_INLINE inline void operator()(message *&m_message)
            {
                delete m_message;
                m_message = nullptr;
            }
        };

        typedef fungus_concurrency::inlined::comm_tplt<message *, __discard_functor> comm_type;
        typedef comm_type::channel_id                                                channel_id;
        typedef comm_type::event                                                     event;

        enum {null_channel_id = comm_type::null_channel_id};
    private:
        fungus_concurrency::inlined::comm_tplt<message *, __discard_functor> impl_;

    public:
        FUNGUSUTIL_ALWAYS_INLINE inline __memory_host(size_t max_channels):
            impl_(max_channels, max_channels * 2, LDBL_MAX, __discard_functor()) {}

        FUNGUSUTIL_ALWAYS_INLINE inline channel_id open_channel(__memory_host *m_host, int data)       {return impl_.open_channel(&(m_host->impl_), data);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       close_channel(channel_id id, int data)            {return impl_.close_channel(id, data);}
        FUNGUSUTIL_ALWAYS_INLINE inline void       close_all_channels(int data)                      {       impl_.close_all_channels(data);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       does_channel_exist(channel_id id) const           {return impl_.does_channel_exist(id);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       is_channel_open(channel_id id) const              {return impl_.is_channel_open(id);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       channel_send(channel_id id, message *m_message)         {return impl_.channel_send(id, m_message);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       channel_receive(channel_id id, message *&m_message)     {return impl_.channel_receive(id, m_message);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       channel_discard(channel_id id, size_t n = 1)      {return impl_.channel_discard(id, n);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       channel_discard_all(channel_id id)                {return impl_.channel_discard_all(id);}
        FUNGUSUTIL_ALWAYS_INLINE inline void       all_channels_discard_all()                        {return impl_.all_channels_discard_all();}
        FUNGUSUTIL_ALWAYS_INLINE inline void       dispatch()                                        {       impl_.dispatch();}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       peek_event(event &m_event) const                  {return impl_.peek_event(m_event);}
        FUNGUSUTIL_ALWAYS_INLINE inline bool       get_event(event &m_event)                         {return impl_.get_event(m_event);}
        FUNGUSUTIL_ALWAYS_INLINE inline void       clear_events()                                    {       impl_.clear_events();}
    };

    unified_host_instance<unified_host_type::memory> *__unified_memory_host(unified_host_base *m_base);

    template <>
    class unified_host_instance<unified_host_type::memory>: public unified_host_base
    {
    protected:
        class peer;

        typedef block_allocator<unified_host_instance::peer, 64> peer_block_allocator;

        class peer: unified_host_base::peer
        {
        protected:
            unified_host_instance   *host_parent;

            __memory_host             *m_host;
            __memory_host::channel_id  m_id;

            std::queue<message *> m_in_messages;

            timestamp begin_period;

            inline peer(unified_host_base *parent):
                unified_host_base::peer(parent),
                m_host(nullptr), m_id(__memory_host::null_channel_id),
                m_in_messages()
            {
                host_parent = static_cast
                    <unified_host_instance
                    <unified_host_type::memory> *>
                    (parent);

                m_host = host_parent->m_host;
            }

            inline peer(unified_host_base *parent, __memory_host::channel_id m_id):
                unified_host_base::peer(parent),
                m_host(nullptr), m_id(m_id),
                m_in_messages()
            {
                host_parent = static_cast
                    <unified_host_instance
                    <unified_host_type::memory> *>
                    (parent);

                m_host = host_parent->m_host;

                if (host_parent->m_peer_map.insert(std::move(peer_map_entry(m_id, this)))
                    != host_parent->m_peer_map.end())
                    m_state = state::connected;
                else
                {
                    m_state = state::none;
                    m_id    = __memory_host::null_channel_id;
                }
            }

            virtual ~peer()
            {
                reset();
            }

            inline void update()
            {
                message *m_message;
                while (m_host->channel_receive(m_id, m_message))
                    m_in_messages.push(m_message);
            }

            inline __memory_host::channel_id get_channel_id()
            {
                return m_id;
            }

            friend class unified_host_instance;
            friend class block_allocator<unified_host_instance::peer, 64>;
        public:
            virtual unified_host_type get_host_type() const {return unified_host_type::memory;}

            virtual bool send(const message *m_message)
            {
                m_host->channel_send(m_id, const_cast<message *>(m_message));
                return true;
            }

            virtual message *receive()
            {
                if (m_in_messages.empty())
                    return nullptr;
                else
                {
                    message *m_message = m_in_messages.front();
                    m_in_messages.pop();

                    return m_message;
                }
            }

            virtual bool connect(unified_host_base *o_host, uint32_t data)
            {
                if (!can_connect(get_state()) ||
                    m_host == nullptr            ||
                    m_id   != __memory_host::null_channel_id) return false;

                unified_host_instance *o_host_inst = __unified_memory_host(o_host);

                m_id = m_host->open_channel(o_host_inst->m_host, data);
                if (m_id == __memory_host::null_channel_id) return false;

                begin_period = timestamp::current_time;
                m_state      = state::connecting;

                return host_parent->m_peer_map.insert(std::move(peer_map_entry(m_id, this)))
                    != host_parent->m_peer_map.end();
            }

            virtual bool disconnect(uint32_t data)
            {
                bool success = can_disconnect(get_state()) &&
                               m_host->close_channel(m_id, data);

                if (success)
                {
                    begin_period = timestamp::current_time;
                    m_state      = state::disconnecting;
                }

                return success;
            }

            virtual bool reset()
            {
                if (m_id != __memory_host::null_channel_id)
                {
                    disconnect(0);

                    m_id    = __memory_host::null_channel_id;
                    m_state = state::none;
                }

                return true;
            }
        };

        typedef block_allocator_object_hash<__memory_host::channel_id, unified_host_instance::peer,
                                            peer_block_allocator> peer_hash_type;

        typedef hash_map<peer_hash_type>                    peer_map_type;
        typedef typename peer_map_type::entry               peer_map_entry;

        peer_block_allocator m_allocator;

        __memory_host      *m_host;
        peer_map_type     m_peer_map;
        std::queue<event> event_queue;

        virtual peer *new_peer(__memory_host::channel_id m_id)
        {
            peer *m_peer = m_common_data.get_policy().grab_peer() ? m_allocator.create(this, m_id) : nullptr;
            return m_peer;
        }

        inline void handle_host_event(__memory_host::event &m_host_event)
        {
            auto it = m_peer_map.find(m_host_event.id);

            switch (m_host_event.t)
            {
            case __memory_host::event::channel_open:
                if (it != m_peer_map.end())
                {
                    peer *m_peer = it->value;
                    m_peer->m_id = m_host_event.id;
                    m_peer->m_state = peer::state::connected;

                    event_queue.push(event(event::type::connected, m_host_event.data, m_peer));
                }
                else
                {
                    peer *m_peer = new_peer(m_host_event.id);
                    if (m_peer != nullptr)
                        event_queue.push(event(event::type::connected, m_host_event.data, m_peer));
                    else
                        m_host->close_channel(m_host_event.id, -reject_reason_host_deny);
                }
                break;
            case __memory_host::event::channel_clos:
                if (it != m_peer_map.end())
                {
                    peer *m_peer = it->value;
                    switch (m_host_event.data)
                    {
                    case -reject_reason_host_deny:
                        m_peer->m_state = peer::state::rejected;
                        event_queue.push(event(event::type::rejected, reject_reason_host_deny, m_peer));
                        break;
                    default:
                        m_peer->m_state = peer::state::disconnected;
                        event_queue.push(event(event::type::disconnected, m_host_event.data, m_peer));
                        break;
                    };
                }

                break;
            default:
                break;
            };
        }

        inline void check_for_timeouts()
        {
            for (auto &it: m_peer_map)
            {
                peer *m_peer = it.value;
                bool timed_out = false;
                event m_event;

                switch (m_peer->m_state)
                {
                case peer::state::connecting:
                    if (m_common_data.
                        get_policy().
                        timed_out(connection_timeout,
                                  timestamp(timestamp::current_time)
                                  - m_peer->begin_period))
                    {
                        timed_out = true;
                        m_event = event(event::type::rejected, reject_reason_host_timeout, m_peer);
                    }
                    break;
                case peer::state::disconnecting:
                    if (m_common_data.
                        get_policy().
                        timed_out(disconnect_timeout,
                                  timestamp(timestamp::current_time)
                                  - m_peer->begin_period))
                    {
                        timed_out = true;
                        m_event = event(event::type::disconnected, disconnect_reason_timeout, m_peer);
                    }
                    break;
                default:
                    continue;
                    break;
                };

                if (timed_out)
                {
                    event_queue.push(m_event);
                    m_peer->reset();
                }
            }
        }
    public:
        inline unified_host_instance(common_data &m_common_data):
            unified_host_base(m_common_data),
            m_allocator(m_common_data.get_max_peers() / 64 + 1),
            m_host(nullptr),
            m_peer_map(m_common_data.get_max_peers() * 2, peer_hash_type(m_allocator)),
            event_queue()
        {
            m_host = new __memory_host(m_common_data.get_max_peers());

            m_peer_map.clear();
        }

        virtual ~unified_host_instance()
        {
            reset_all_peers();
            m_peer_map.clear();

            delete m_host;
        }

        virtual unified_host_type get_type() const
        {
            return unified_host_type::memory;
        }

        virtual void reset_all_peers()
        {
            for (auto &entry: m_peer_map)
                entry.value->reset();
        }

        virtual void destroy_all_peers()
        {
            m_peer_map.clear();
        }

        virtual size_t count_peers() const
        {
            return m_peer_map.size();
        }

        virtual unified_host_base::peer *new_peer(unified_host_type type)
        {
            if (type != unified_host_type::memory) return nullptr;

            unified_host_base::peer *m_peer = m_common_data.get_policy().grab_peer() ? m_allocator.create(this) : nullptr;
            return m_peer;
        }

        virtual bool destroy_peer(unified_host_base::peer *m_peer)
        {
            peer *m_native_peer = dynamic_cast<peer *>(m_peer);
            bool success = m_native_peer != nullptr &&
                           m_native_peer->host_parent == this &&
                           peer::can_connect(m_native_peer->get_state());

            if (success)
            {
                m_peer_map.erase(m_native_peer->get_channel_id());
                m_common_data.get_policy().drop_peer();
            }

            return success;
        }

        virtual void dispatch()
        {
            for (auto &entry: m_peer_map)
                entry.value->update();

            m_host->dispatch();

            __memory_host::event m_host_event;
            while (m_host->get_event(m_host_event))
                handle_host_event(m_host_event);

            check_for_timeouts();
        }

        virtual bool next_event(event &m_event)
        {
            if (!event_queue.empty())
            {
                m_event = event_queue.front();
                event_queue.pop();

                return true;
            }
            else
                return false;
        }

        virtual bool peek_event(event &m_event) const
        {
            if (!event_queue.empty())
            {
                m_event = event_queue.front();
                return true;
            }
            else
                return false;
        }
    };
}

#endif
