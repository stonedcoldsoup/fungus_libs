#ifndef FUNGUSNET_UNITY_ENET_H
#define FUNGUSNET_UNITY_ENET_H

#include "fungus_net_unity_base.h"

namespace fungus_net
{
    template <>
    class unified_host_instance<unified_host_type::networked>: public unified_host_base
    {
    protected:
        class peer;

        typedef block_allocator<unified_host_instance::peer, 64> peer_block_allocator;

        class peer: unified_host_base::peer
        {
        protected:
            const endian_converter &endian;
            unified_host_instance *enet_parent;

            ENetHost *enet_host;
            ENetPeer *enet_peer;

            std::queue<message *> m_in_messages;

            timestamp begin_period;

            inline peer(unified_host_base *parent):
                unified_host_base::peer(parent),
                endian(parent->get_common_data().get_endian_converter()),
                enet_host(nullptr), enet_peer(nullptr),
                m_in_messages()
            {
                enet_parent = static_cast
                    <unified_host_instance
                    <unified_host_type::networked> *>
                    (parent);

                enet_host = enet_parent->enet_host;
            }

            inline peer(unified_host_base *parent, ENetPeer *enet_peer):
                unified_host_base::peer(parent),
                endian(parent->get_common_data().get_endian_converter()),
                enet_host(nullptr), enet_peer(enet_peer),
                m_in_messages()
            {
                enet_parent = static_cast
                    <unified_host_instance
                    <unified_host_type::networked> *>
                    (parent);

                enet_host = enet_parent->enet_host;

                if (enet_parent->enet_peer_map.insert(std::move(enet_peer_map_entry(enet_peer, this)))
                    != enet_parent->enet_peer_map.end())
                    m_state = state::connected;
                else
                {
                    m_state   = state::none;
                    enet_peer = nullptr;
                }
            }

            virtual ~peer()
            {
                reset();
            }

            inline void place_in_m_message(message *m_message)
            {
                m_in_messages.push(m_message);
            }

            inline ENetPeer *get_enet_peer()
            {
                return enet_peer;
            }

            friend class unified_host_instance;
            friend class block_allocator<unified_host_instance::peer, 64>;
        public:
            virtual unified_host_type get_host_type() const {return unified_host_type::networked;}

            virtual bool send(const message *m_message)
            {
                packet *pk = enet_parent->agg.create_packet();
                if (!pk) return false;

                if (!pk->initialize_outgoing(m_message, endian))
                {
                    enet_parent->agg.destroy_packet(pk);
                    return false;
                }

                enet_parent->agg.queue_packet(pk, enet_peer);
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

            virtual bool connect(const ipv4 &m_ipv4, uint32_t data)
            {
                if (!can_connect(get_state()) ||
                    enet_host == nullptr         ||
                    enet_peer != nullptr) return false;

                ENetAddress enet_addr;
                enet_addr.host = m_ipv4.m_host.value;
                enet_addr.port = m_ipv4.port_i;

                enet_peer = enet_host_connect(enet_host, &enet_addr, UINT8_MAX, (enet_uint32)data);

                if (enet_peer == nullptr) return false;

                begin_period = timestamp::current_time;
                m_state      = state::connecting;

                return enet_parent->enet_peer_map.insert(std::move(enet_peer_map_entry(enet_peer, this)))
                    != enet_parent->enet_peer_map.end();
            }

            virtual bool disconnect(uint32_t data)
            {
                bool success = can_disconnect(get_state());

                if (success)
                {
                    enet_peer_disconnect(enet_peer, data);

                    begin_period = timestamp::current_time;
                    m_state      = state::disconnecting;
                }

                return success;
            }

            virtual bool reset()
            {
                if (enet_peer)
                {
                    enet_peer_reset(enet_peer);
                    enet_peer = nullptr;

                    m_state = state::none;
                }

                return true;
            }
        };

        typedef block_allocator_object_hash<ENetPeer *, unified_host_instance::peer,
                                          peer_block_allocator> enet_peer_hash_type;

        typedef hash_map<enet_peer_hash_type>                    enet_peer_map_type;
        typedef typename enet_peer_map_type::entry               enet_peer_map_entry;

        peer_block_allocator m_allocator;

        ENetHost *enet_host;
        enet_peer_map_type enet_peer_map;
        std::queue<event> event_queue;

        packet::aggregator agg;
        packet::separator  sep;

        virtual peer *new_peer(ENetPeer *enet_peer)
        {
            peer *m_peer = m_common_data.get_policy().grab_peer() ? m_allocator.create(this, enet_peer) : nullptr;
            return m_peer;
        }

        inline void handle_enet_event(ENetEvent &enet_event)
        {
            auto it = enet_peer_map.find(enet_event.peer);

            switch (enet_event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                if (it != enet_peer_map.end())
                {
                    peer *m_peer = it->value;
                    m_peer->enet_peer = enet_event.peer;
                    m_peer->m_state = peer::state::connected;

                    event_queue.push(event(event::type::connected, enet_event.data, m_peer));
                }
                else
                {
                    peer *m_peer = new_peer(enet_event.peer);
                    if (m_peer != nullptr)
                        event_queue.push(event(event::type::connected, enet_event.data, m_peer));
                    else
                        enet_peer_disconnect(enet_event.peer, (enet_uint32)-reject_reason_host_deny);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (it != enet_peer_map.end())
                {
                    peer *m_peer = it->value;

                    sep.separate_packets(enet_event.packet, enet_event.channelID);
                    enet_packet_destroy(enet_event.packet);

                    while (sep.packets_waiting())
                    {
                        packet *pk = sep.get_packet();
                        message *m_message = pk->make_message(&m_common_data.get_message_factory_manager());

                        sep.destroy_packet(pk);

                        if (m_message)
                            m_peer->place_in_m_message(m_message);
                    }
                }

                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (it != enet_peer_map.end())
                {
                    peer *m_peer = it->value;
                    switch (enet_event.data)
                    {
                    case (uint32_t)-reject_reason_host_deny:
                        m_peer->m_state = peer::state::rejected;
                        event_queue.push(event(event::type::rejected, reject_reason_host_deny, m_peer));
                        break;
                    default:
                        m_peer->m_state = peer::state::disconnected;
                        event_queue.push(event(event::type::disconnected, enet_event.data, m_peer));
                        break;
                    };
                }

                break;
            case ENET_EVENT_TYPE_NONE:
            default:
                break;
            };
        }

        inline void check_for_timeouts()
        {
            for (auto &it: enet_peer_map)
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
        inline unified_host_instance(common_data &m_common_data, const ipv4 &m_ipv4, uint32_t in_bandwidth = 0, uint32_t out_bandwidth = 0):
            unified_host_base(m_common_data),
            m_allocator(m_common_data.get_max_peers() / 64 + 1),
            enet_host(nullptr),
            enet_peer_map(m_common_data.get_max_peers() * 2, enet_peer_hash_type(m_allocator)),
            event_queue(),
            agg(m_common_data.get_endian_converter()),
            sep(m_common_data.get_endian_converter())
        {
            ENetAddress enet_addr;
            enet_addr.host = m_ipv4.m_host.value;
            enet_addr.port = m_ipv4.port_i;

            enet_host = enet_host_create(&enet_addr,
                                         m_common_data.get_max_peers() + internal_defs::aux_peer_slot_count,
                                         internal_defs::all_channel_count, in_bandwidth, out_bandwidth);

            fungus_util_assert(enet_host != nullptr,
                "fungus_util::unified_host_instance<networked>::unified_host_instance(): could not create enet host!");

            enet_peer_map.clear();
        }

        virtual ~unified_host_instance()
        {
            reset_all_peers();
            enet_peer_map.clear();

            enet_host_destroy(enet_host);
        }

        virtual unified_host_type get_type() const
        {
            return unified_host_type::networked;
        }

        virtual void reset_all_peers()
        {
            for (auto &entry: enet_peer_map)
                entry.value->reset();
        }

        virtual void destroy_all_peers()
        {
            enet_peer_map.clear();
        }

        virtual size_t count_peers() const
        {
            return enet_peer_map.size();
        }

        virtual unified_host_base::peer *new_peer(unified_host_type type)
        {
            if (type != unified_host_type::networked) return nullptr;

            unified_host_base::peer *m_peer = m_common_data.get_policy().grab_peer() ? m_allocator.create(this) : nullptr;
            return m_peer;
        }

        virtual bool destroy_peer(unified_host_base::peer *m_peer)
        {
            peer *m_native_peer = dynamic_cast<peer *>(m_peer);
            bool success = m_native_peer != nullptr &&
                           m_native_peer->enet_parent == this &&
                           peer::can_connect(m_native_peer->get_state());

            if (success)
            {
                enet_peer_map.erase(m_native_peer->get_enet_peer());
                m_common_data.get_policy().drop_peer();
            }

            return success;
        }

        virtual void dispatch()
        {
            agg.send_all();

            ENetEvent enet_event;
            if (enet_host_service(enet_host, &enet_event, 0) > 0)
                handle_enet_event(enet_event);

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
