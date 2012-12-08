#include "fungus_net_packet.h"

// included for test purposes
#include <set>

namespace fungus_net
{
    using namespace fungus_util;

    uint16_t packet::guard_word = 0xFFBE;

    static inline packet::stream_mode from_m_message_stream_mode(message::stream_mode smode)
    {
        switch (smode)
        {
        case message::stream_mode::sequenced:    return packet::stream_mode::sequenced;    break;
        case message::stream_mode::unsequenced:  return packet::stream_mode::unsequenced;  break;
        default:
            break;
        };
        return packet::stream_mode::invalid;
    }

    static inline message::stream_mode from_packet_stream_mode(packet::stream_mode smode)
    {
        switch (smode)
        {
        case packet::stream_mode::sequenced:    return message::stream_mode::sequenced;    break;
        case packet::stream_mode::unsequenced:  return message::stream_mode::unsequenced;  break;
        default:
            break;
        };
        return message::stream_mode::invalid;
    }

    static inline std::string packet_stream_mode_to_string(packet::stream_mode smode)
    {
        switch (smode)
        {
            case packet::stream_mode::sequenced:    return "packet::sequenced";             break;
            case packet::stream_mode::unsequenced:  return "packet::unsequenced";           break;
            default:                                return "packet::<UNKNOWN STREAM MODE>"; break;
        };
    }

    packet::packet():
        smode(stream_mode::sequenced),  channel(0),
        dest(destination::outgoing),    initialized(false),
        ds(nullptr), s(nullptr), buf()
        {}

    packet::~packet()
    {
        if (initialized)
        {
            switch (dest)
            {
            case destination::incoming:
                delete ds;
                break;
            case destination::outgoing:
                delete s;
                break;
            default:
                fungus_util_assert(false, "Attempted to destroy packet with unknown destination type!");
                break;
            };
        }
    }

    bool packet::initialize_outgoing(const message *m_message, const endian_converter &endian)
    {
        if (initialized) return false;

        dest    = destination::outgoing;
        smode   = from_m_message_stream_mode(m_message->get_stream_mode());
        channel = m_message->get_channel();

        fungus_util_assert(smode != stream_mode::invalid, "packet::initialize_outgoing(): invalid stream mode!\n");

        s = new serializer(endian);

        *s << any_type(guard_word) << any_type(m_message->get_type());

        m_message->serialize_data(*s);
        buf.set(s->buf, s->size);

        return initialized = true;
    }

    bool packet::initialize_incoming(serializer_buf &buf,
                                     stream_mode smode, uint8_t channel,
                                     const endian_converter &endian)
    {
        if (initialized) return false;

        dest          = destination::incoming;
        this->smode   = smode;
        this->channel = channel;

        this->buf.set(buf.buf, buf.size);
        ds = new deserializer(endian, this->buf.buf, this->buf.size);

        uint16_t guard_word_test = 0;
        *ds >> guard_word_test;

        if (guard_word == guard_word_test)
            initialized = true;
        else
        {
            delete ds;
            initialized = false;
        }

        return initialized;
    }

    bool packet::switch_destination(const endian_converter &endian)
    {
        if (!initialized) return false;

        bool     success = true;
        uint16_t guard_word_test = 0;

        switch (dest)
        {
        case destination::incoming:
            delete ds;
            ds = nullptr;
            dest = destination::outgoing;
            break;
        case destination::outgoing:
            delete s;
            s = nullptr;
            dest = destination::incoming;

            ds = new deserializer(endian, buf.buf, buf.size);
            *ds >> guard_word_test;

            if (guard_word_test != guard_word)
            {
                initialized = false;
                success     = false;
            }

            break;
        default:
            fungus_util_assert(false, "Attempted to switch destination on a packet with unknown destination type!");
            break;
        };

        return success;
    }

    packet::destination packet::get_destination() const {return dest;}
    bool packet::is_initialized() const                 {return initialized;}

    packet::stream_mode packet::get_stream_mode() const {return smode;}
    uint8_t packet::get_channel() const                 {return channel;}

    message *packet::make_message(message_factory_manager *factory_manager)
    {
        if (!initialized || dest != destination::incoming) return nullptr;

        message_type type = 0;
        *ds >> type;

        const message::factory *factory = factory_manager->get_factory(type);

        if (!factory)
        {
            ds->reset();

            uint16_t guard_word_test;
            *ds >> guard_word_test;

            return nullptr;
        }

        message *m_message = factory->create();

        if (m_message)
            m_message->deserialize_data(*ds);

        return m_message;
    }

    void packet::set_guard_word(int word)
    {
        guard_word = word;
    }

    inline constexpr uint32_t __enet_flags<packet::stream_mode::sequenced>::get_flags()
    {
        return ENET_PACKET_FLAG_RELIABLE;
    }

    inline constexpr uint32_t __enet_flags<packet::stream_mode::unsequenced>::get_flags()
    {
        return ENET_PACKET_FLAG_UNSEQUENCED;
    }

    packet::aggregator::aggregator(const endian_converter &endian):
        m_allocator(), endian(endian), packets()
    {}

    packet *packet::aggregator::create_packet()
    {
        return m_allocator.create();
    }

    bool packet::aggregator::destroy_packet(packet *pk)
    {
        return m_allocator.destroy(pk);
    }

    void packet::aggregator::queue_packet(packet *pk, ENetPeer *peer)
    {
        packets.push(packet_ref(pk, peer));
    }

    void packet::aggregator::send_all()
    {
        aggregate_map agg_map(endian);

        const bool b_singleton = (packets.size() == 1);

        while (!packets.empty())
        {
            packet     *pk      = packets.front().first;
            ENetPeer   *peer    = packets.front().second;

            uint8_t     channel = pk->get_channel();
            stream_mode smode   = pk->get_stream_mode();

            packets.pop();

            auto &agg = agg_map.get_aggregate(std::pair<ENetPeer *, uint8_t>(peer, channel));
            if (b_singleton)
                agg.get_serializer(smode) << any_type(pk->buf);
            else
                agg.get_serializer(smode) << any_type(pk->buf.size) << any_type(pk->buf);

            m_allocator.destroy(pk);
        };

        agg_map.send_all();
    }

    packet::separator::separator(const endian_converter &endian):
        m_allocator(), endian(endian), packets()
    {}

    packet::separator::~separator()
    {
        packet *pk;
        while ((pk = get_packet()) != nullptr)
            m_allocator.destroy(pk);
    }

    bool packet::separator::create_packet(serializer_buf &buf, stream_mode smode, uint8_t channel)
    {
        packet *pk   = m_allocator.create();
        bool success = pk->initialize_incoming(buf, smode, channel, endian);

        if (success)
            packets.push(pk);
        else
            success = m_allocator.destroy(pk) && success;

        return success;
    }

    bool packet::separator::destroy_packet(packet *pk)
    {
        return m_allocator.destroy(pk);
    }

    bool packet::separator::separate_packets(ENetPacket *source, uint8_t channel)
    {
        deserializer ds(endian, (char *)source->data, source->dataLength);
        stream_mode smode = source->flags & ENET_PACKET_FLAG_RELIABLE ?
                            stream_mode::sequenced : stream_mode::unsequenced;

        uint16_t guard_word_test = 0;

        ds >> guard_word_test;

        bool success     = true;
        bool b_get       = true;
        bool b_singleton = (guard_word_test == guard_word);

        ds.reset();

        if (b_singleton)
        {
            serializer_buf buf((char *)source->data, source->dataLength);

            if (!create_packet(buf, smode, channel))
                success = false;
        }
        else
        {
            while (b_get)
            {
                size_t size = 0;

                ds >> size;
                if (size != 0)
                {
                    serializer_buf buf(size);
                    ds >> buf;

                    if (!create_packet(buf, smode, channel))
                        success = false;
                }
                else
                    b_get = false;
            }
        }

        return success;
    }

    bool packet::separator::packets_waiting() const
    {
        return !packets.empty();
    }

    packet *packet::separator::get_packet()
    {
        packet *pk = nullptr;

        if (!packets.empty())
        {
            pk = packets.front();
            packets.pop();
        }

        return pk;
    }

    packet::aggregator::aggregate_map::aggregate_map(const endian_converter &endian):
        endian(endian), aggs()
    {
    }

    packet::aggregator::aggregate_map::~aggregate_map()
    {
        clear();
    }

    packet::aggregator::aggregate &packet::aggregator::aggregate_map::
        get_aggregate(std::pair<ENetPeer *, uint8_t> &&pair)
    {
        auto it = aggs.find((ENetPeer *&)pair.first);
        if (it == aggs.end())
            it = aggs.insert(map_peer_type::entry(pair.first, std::move(map_agg_type())));

        auto jt = it->value.find(pair.second);
        if (jt == it->value.end())
            jt = it->value.insert(map_agg_type::entry(pair.second, std::move(aggregate(pair, endian))));

        return jt->value;
    }

    void packet::aggregator::aggregate_map::send_all()
    {
        for (auto &it: aggs)
        {
            for (auto &jt: it.value)
                jt.value.send();
        }
    }

    void packet::aggregator::aggregate_map::clear()
    {
        aggs.clear();
    }

    packet::aggregator::aggregate_serializer_base::
        aggregate_serializer_base(const endian_converter &endian):
        used(false), s(endian)
    {}

    serializer &packet::aggregator::aggregate_serializer_base::get()
    {
        used = true;
        return s;
    }

    template <packet::stream_mode __smode>
    packet::aggregator::aggregate_serializer<__smode>::
        aggregate_serializer(const endian_converter &endian):
        aggregate_serializer_base(endian)
    {}

    template <packet::stream_mode __smode>
    void packet::aggregator::aggregate_serializer<__smode>::
        send(ENetPeer *peer, uint8_t channel)
    {
        if (used)
        {
            s << any_type((size_t)0);

            ENetPacket *pk = enet_packet_create(s.buf, s.size,
                                                __enet_flags<__smode>::
                                                get_flags());
            enet_peer_send(peer, channel, pk);

            s.reset();
            used = false;
        }
    }

    packet::aggregator::aggregate::aggregate(std::pair<ENetPeer *, uint8_t> &pair,
                                             const endian_converter &endian):
        endian(endian), peer(pair.first), channel(pair.second)
    {
          seq = new aggregate_serializer<stream_mode::sequenced>(endian);
        unseq = new aggregate_serializer<stream_mode::unsequenced>(endian);
    }

    packet::aggregator::aggregate::aggregate(const aggregate &agg):
        endian(agg.endian),
        seq(nullptr), unseq(nullptr),
        peer(agg.peer), channel(agg.channel)
    {
          seq = new aggregate_serializer<stream_mode::sequenced>(endian);
        unseq = new aggregate_serializer<stream_mode::unsequenced>(endian);
    }

    packet::aggregator::aggregate::aggregate(aggregate &&agg):
        endian(agg.endian),
        seq(agg.seq), unseq(agg.unseq),
        peer(agg.peer), channel(agg.channel)
    {
        agg.seq   = nullptr;
        agg.unseq = nullptr;
    }

    packet::aggregator::aggregate::~aggregate()
    {
        send();

        if (seq && unseq)
        {
            delete   seq;
            delete unseq;
        }
    }

    packet::aggregator::aggregate &packet::aggregator::aggregate::
        operator =(const aggregate &agg)
    {
        if (seq && unseq)
        {
            delete seq;
            delete unseq;
        }

        peer    = agg.peer;
        channel = agg.channel;

          seq = agg.seq;
        unseq = agg.unseq;

        return *this;
    }

    packet::aggregator::aggregate &packet::aggregator::aggregate::
        operator =(aggregate &&agg)
    {
        if (seq && unseq)
        {
            delete seq;
            delete unseq;
        }

        peer    = agg.peer;
        channel = agg.channel;

          seq = agg.seq;
        unseq = agg.unseq;

        agg.seq   = nullptr;
        agg.unseq = nullptr;

        return *this;
    }

    serializer &packet::aggregator::aggregate::get_serializer(stream_mode smode)
    {
        switch (smode)
        {
        case stream_mode::sequenced:
            return seq->get();
            break;
        case stream_mode::unsequenced:
            return unseq->get();
            break;
        default:
            fungus_util_assert(false,
                "packet::aggregator::aggregate::get_serializer(): unknown stream mode!\n");
				
			// keep the compiler happy.
			return unseq->get();
            break;
        };
    }

    void packet::aggregator::aggregate::send()
    {
        if (seq && unseq)
        {
              seq->send(peer, channel);
            unseq->send(peer, channel);
        }
    }

// TEST SHIT, HIDE ME ON DISTRO!

    class test_m_message: public protocol_message
    {
    public:
        class factory: public message::factory
        {
            virtual message *create() const
            {
                return new test_m_message();
            }

            virtual factory *move() const
            {
                return new factory();
            }

            virtual message_type get_type() const
            {
                return 7;
            }
        };

        size_t host_i;
        std::string str;

        test_m_message(size_t host_i = 0, const std::string &str = ""):
            protocol_message(), host_i(host_i), str(str) {}

        virtual message_type get_type() const
        {
            return 7;
        }

        virtual message *copy() const
        {
            return new test_m_message(host_i, str);
        }
    protected:
        virtual void serialize_data(serializer &s) const
        {
            s << any_type(str) << any_type(host_i);
        }

        virtual void deserialize_data(deserializer &s)
        {
            s >> str >> host_i;
        }
    };

    enum {TESTHOSTCOUNT = 2};

    struct test_host
    {
        ENetHost *host;
        std::set<ENetPeer *> peers;

        packet::aggregator agg;
        packet::separator  sep;

        size_t connected_count;
        size_t received_count;

        test_host(const endian_converter &endian):
            host(nullptr), peers(),
            agg(endian), sep(endian),
            connected_count(0),
            received_count(0)
        {}
    };

    void test_push_message(packet::aggregator *agg, size_t host_i, const char *str, ENetPeer *peer, const endian_converter &endian)
    {
        packet   *pk  = agg->create_packet();
        test_m_message *m_message = new test_m_message(host_i, str);

        fungus_util_assert(pk->initialize_outgoing(m_message, endian), "cannot initialize outgoing packet!");

        agg->queue_packet(pk, peer);

        delete m_message;
    }

    FUNGUSNET_API bool test_aggregation()
    {
        enet_initialize();

        message_factory_manager mgr;
        endian_converter endian;

        test_host *hosts[TESTHOSTCOUNT];

        endian.lazy_register_numeric_types();
        mgr.add_factory(test_m_message::factory());

        for (size_t i = 0; i < TESTHOSTCOUNT; ++i)
        {
            ENetAddress addr;

            enet_address_set_host(&addr, "localhost");
            addr.port = 2345 + i;

            hosts[i] = new test_host(endian);
            hosts[i]->host = enet_host_create(&addr, TESTHOSTCOUNT, 10, 0, 0);
            fungus_util_assert(hosts[i]->host, "could not create enet host!\n");

            std::cout << "created enet host " << i << "...\n";

            for (size_t j = 0; j < i; ++j)
            {
                addr.port = 2345 + j;

                ENetPeer *peer = enet_host_connect(hosts[i]->host, &addr, 10, 0);
                fungus_util_assert(peer, "could not create enet peer!");

                std::cout << "  connecting " << i << " to " << j << '\n';

                hosts[i]->peers.insert(peer);
            }
        }

        bool success = true;

        size_t finished_count = 0, connected_count = 0;
        while (finished_count < TESTHOSTCOUNT)
        {
            for (size_t i = 0; i < TESTHOSTCOUNT; ++i)
            {
                ENetEvent event;
                if (enet_host_service(hosts[i]->host, &event, 0) > 0)
                {
                    std::cout << "host " << i << " generated event:\n";

                    switch (event.type)
                    {
                    case ENET_EVENT_TYPE_CONNECT:
                        std::cout << "  ENET_EVENT_TYPE_CONNECT\n";

                        if (hosts[i]->peers.find(event.peer) == hosts[i]->peers.end())
                            hosts[i]->peers.insert(event.peer);

                        if (++hosts[i]->connected_count >= TESTHOSTCOUNT - 1)
                        {
                            ++connected_count;

                            for (std::set<ENetPeer *>::iterator it = hosts[i]->peers.begin(); it != hosts[i]->peers.end(); ++it)
                            {
                                test_push_message(&hosts[i]->agg, i, "I",         *it, endian);
                                test_push_message(&hosts[i]->agg, i, "am",        *it, endian);
                                test_push_message(&hosts[i]->agg, i, "a",         *it, endian);
                                test_push_message(&hosts[i]->agg, i, "test.",      *it, endian);

                                //test_push_message(&(hosts[i]->agg), i, "I am a test.", *it, endian);
                            }

                            hosts[i]->agg.send_all();

                            std::cout << "  host " << i << " connected to all!\n";
                        }

                        std::cout << "  host " << i << ' ' << hosts[i]->connected_count
                                  << " hosts connected, " << connected_count << " hosts finished connecting.\n";

                        break;
                    case ENET_EVENT_TYPE_RECEIVE:
                        std::cout << "  ENET_EVENT_TYPE_RECEIVE\n";

                        fungus_util_assert(hosts[i]->sep.separate_packets(event.packet, event.channelID),
                            "failed to separate packets!\n");

                        enet_packet_destroy(event.packet);

                        std::cout << "  receiving messages: ";
                        while (hosts[i]->sep.packets_waiting())
                        {
                            packet *pk = hosts[i]->sep.get_packet();
                            test_m_message *m_message = dynamic_cast<test_m_message *>(pk->make_message(&mgr));

                            if (m_message)
                            {
                                std::cout << m_message->host_i << " \"" << m_message->str << "\", ";
                                delete m_message;
                            }

                            hosts[i]->sep.destroy_packet(pk);
                        }
                        std::cout << '\n';

                        if (++hosts[i]->received_count >= TESTHOSTCOUNT - 1)
                            ++finished_count;

                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        finished_count = TESTHOSTCOUNT;
                        success = false;
                        break;
                    case ENET_EVENT_TYPE_NONE:
                    default:
                        break;
                    };
                }

                if (finished_count >= TESTHOSTCOUNT)
                    break;
            }
        }

        std::cout << "finished test!\n";

        for (size_t i = 0; i < TESTHOSTCOUNT; ++i)
        {
            enet_host_destroy(hosts[i]->host);
            delete hosts[i];
        }

        enet_deinitialize();

        return success;
    }
}
