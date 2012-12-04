#include "fungus_net_unity.h"

// testing purposes
#include <set>

namespace fungus_net
{
    unified_host_instance<unified_host_type::memory> *__unified_memory_host(unified_host_base *m_base)
    {
        unified_host                                     *m_host      = nullptr;
        unified_host_instance<unified_host_type::memory> *m_host_inst = nullptr;

        switch (m_base->get_type())
        {
        case unified_host_type::unified:
            m_host      = static_cast<unified_host *>(m_base);
            m_host_inst = m_host->m_host_storage.m_memory_host;
            break;
        case unified_host_type::memory:
            m_host_inst = static_cast<unified_host_instance<unified_host_type::memory> *>(m_base);
            break;
        case unified_host_type::networked:
        default:
            break;
        };

        return m_host_inst;
    }

    unified_host::host_storage::host_storage(common_data &m_common_data,
                                             uint32_t flags, const ipv4 &m_ipv4,
                                             uint32_t in_bandwidth, uint32_t out_bandwidth):
        m_common_data(m_common_data),
        m_networked_host(), m_memory_host()
    {
        if (flags & unified_host_flag_networked)
            m_networked_host.create(m_common_data, m_ipv4,
                                    in_bandwidth, out_bandwidth);

        if (flags & unified_host_flag_memory)
            m_memory_host.create(m_common_data);
    }

    unified_host::unified_host(common_data &m_common_data):
        unified_host_base(m_common_data), flags(unified_host_flag_memory),
        m_host_storage(m_common_data, unified_host_flag_memory, ipv4(), 0, 0)
    {}

    unified_host::unified_host(common_data &m_common_data,
        uint32_t flags, const ipv4 &m_ipv4,
        uint32_t in_bandwidth, uint32_t out_bandwidth):
        unified_host_base(m_common_data), flags(flags),
        m_host_storage(m_common_data, flags, m_ipv4, in_bandwidth, out_bandwidth)
    {}

    unified_host::~unified_host() {}

    void unified_host::reset_all_peers()
    {
        call_reset_all_peers m_call;
        m_host_storage.enumerate(m_call, flags);
    }

    void unified_host::destroy_all_peers()
    {
        call_destroy_all_peers m_call;
        m_host_storage.enumerate(m_call, flags);
    }

    size_t unified_host::count_peers() const
    {
        call_count_peers m_call;
        m_host_storage.enumerate(m_call, flags);
        return m_call.result;
    }

    unified_host::peer *unified_host::new_peer(unified_host_type type)
    {
        unified_host_base *m_host = m_host_storage.get_host(type);
        peer *m_peer = nullptr;

        if (m_host)
            m_peer = m_host->new_peer(type);

        return m_peer;
    }

    bool unified_host::destroy_peer(peer *m_peer)
    {
        unified_host_base *m_host =
            m_host_storage.get_host(m_peer->get_host_type());

        return m_host ? m_host->destroy_peer(m_peer) : false;
    }

    void unified_host::dispatch()
    {
        call_dispatch m_call;
        m_host_storage.enumerate(m_call, flags);
    }

    bool unified_host::next_event(event &m_event)
    {
        call_next_event m_call(m_event);
        m_host_storage.enumerate(m_call, flags);
        return m_call.b_got_event;
    }

    bool unified_host::peek_event(event &m_event) const
    {
        call_peek_event m_call(m_event);
        m_host_storage.enumerate(m_call, flags);
        return m_call.b_got_event;
    }

// TEST SHIT

    class test_m_message2: public protocol_message
    {
    public:
        class factory: public message::factory
        {
            virtual message *create() const
            {
                return new test_m_message2();
            }

            virtual factory *move() const
            {
                return new factory();
            }

            virtual message_type get_type() const
            {
                return 8;
            }
        };

        size_t host_i;
        std::string str;

        test_m_message2(size_t host_i = 0, const std::string &str = ""):
            protocol_message(), host_i(host_i), str(str) {}

        virtual message_type get_type() const
        {
            return 8;
        }

        virtual message *copy() const
        {
            return new test_m_message2(host_i, str);
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

    constexpr size_t n_test_hosts = 3;

    void test_unity(unified_host_type type)
    {
        std::cout << "testing " << (type == unified_host_type::networked ? "networked" : "memory") << " hosts..." << std::endl;

        if (type == unified_host_type::networked)
            enet_initialize();

        unified_host::common_data data[n_test_hosts] =
        {
            n_test_hosts - 1,
            n_test_hosts - 1,
            n_test_hosts - 1
        };

        unified_host *hosts[n_test_hosts];
        std::set<unified_host::peer *> peers[n_test_hosts];

        for (size_t i = 0; i < n_test_hosts; ++i)
        {
            data[i].get_message_factory_manager().add_factory(test_m_message2::factory());

            if (type == unified_host_type::memory)
                hosts[i] = new unified_host(data[i]);
            else
                hosts[i] = new unified_host(data[i], unified_host_flag_all, ipv4(ipv4::host("localhost"), 8999 + i));

            for (size_t j = 0; j < i; ++j)
            {
                unified_host::peer *m_peer = hosts[i]->new_peer(type);
                if (m_peer)
                {
                    bool success;
                    if (type == unified_host_type::memory)
                        success = m_peer->connect(hosts[j], 0);
                    else
                        success = m_peer->connect(ipv4(ipv4::host("localhost"), 8999 + j), 0);

                    if (!success)
                        std::cout << "cannot connect peer!\n";
                }
                else
                    std::cout << "cannot create peer!\n";
            }
        }

        size_t n_connected = 0, n_received = 0, n_dead = 0;
        while (n_dead < n_test_hosts)
        {
            unified_host::event event;
            for (size_t i = 0; i < n_test_hosts; ++i)
            {
                hosts[i]->dispatch();
                if (hosts[i]->next_event(event))
                {
                    switch (event.m_type)
                    {
                    case unified_host::event::type::connected:
                        std::cout << "OH HAI!\n";
                        ++n_connected;
                        event.m_peer->send(new test_m_message2(i, "hello!"));
                        peers[i].insert(event.m_peer);
                        break;
                    case unified_host::event::type::disconnected:
                        std::cout << "GTFO N00B!\n";
                        ++n_dead;
                        peers[i].erase(event.m_peer);
                        break;
                    case unified_host::event::type::rejected:
                        std::cout << "BUTTZ!\n";
                        ++n_dead;
                        break;
                    default:
                        break;
                    };
                }

                for (auto m_peer: peers[i])
                {
                    message *m_message = m_peer->receive();
                    if (m_message)
                    {
                        test_m_message2 *tm_message = dynamic_cast<test_m_message2 *>(m_message);
                        fungus_util_assert(tm_message, "BUTTZ");
                        std::cout << tm_message->str << ", " << tm_message->host_i << std::endl;
                        ++n_received;
                    }
                }

                if (n_received == n_test_hosts * (n_test_hosts - 1))
                {
                    for (size_t j = 0; j < n_test_hosts; ++j)
                    {
                        for (auto m_peer: peers[j])
                            m_peer->disconnect(0);
                    }

                    ++n_received;
                }
            }
        }

        for (size_t i = 0; i < n_test_hosts; ++i)
            delete hosts[i];

        if (type == unified_host_type::networked)
            enet_deinitialize();

        std::cout << "finished test!" << std::endl;
    }

    FUNGUSNET_API void test_unity()
    {
        test_unity(unified_host_type::memory);
        test_unity(unified_host_type::networked);
    }
}
