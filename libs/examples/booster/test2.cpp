#include "fungus_booster/fungus_booster.h"

#include <set>

namespace fnet = fungus_net;

constexpr size_t n_hosts         = 3;
constexpr size_t max_peers       = n_hosts - 1;
constexpr size_t max_connections = n_hosts * max_peers;

constexpr fnet::message_type test_message_type = 1;

class test_message: public fnet::user_message
{
public:
    class factory: public fnet::message::factory
    {
    public:
        virtual fnet::message *create() const
        {
            fnet::message *m_message = new test_message();
            return m_message;
        }

        virtual fnet::message::factory *move() const
        {
            factory *m_factory = new factory();
            return m_factory;
        }

        virtual fnet::message_type get_type() const
        {
            return test_message_type;
        }
    };

    size_t      m_host_i;
    std::string m_text;

    test_message(size_t m_host_i = 0, const std::string &m_text = ""):
        fnet::user_message(fnet::message::stream_mode::unsequenced),
        m_host_i(m_host_i), m_text(m_text)
    {}

    virtual fnet::message_type get_type() const
    {
        return test_message_type;
    }

    virtual fnet::message *copy() const
    {
        fnet::message *m_message = new test_message(m_host_i, m_text);
        return m_message;
    }
protected:
    virtual void serialize_data(fungus_util::serializer &s) const
    {
        s << m_host_i << m_text;
    }

    virtual void deserialize_data(fungus_util::deserializer &s)
    {
        s >> m_host_i >> m_text;
    }
};

enum test_connection_behavior
{
    test_peer_to_peer,
    test_server_with_clients
};

template <test_connection_behavior __behavior>
class host_usage_test;

template <>
class host_usage_test<test_peer_to_peer>: public fnet::host::callbacks
{
public:
    fnet::host              m_hosts[n_hosts];
    std::set<fnet::peer_id> m_peers[n_hosts];
    size_t                  m_auth_counts[n_hosts];
    size_t                  n_finished;
    size_t                  n_authenticated;
    size_t                  n_intercepted;
    size_t                  host_i;

    host_usage_test():
        fnet::host::callbacks(fnet::host::callbacks::implements_all_but_error)
    {}

    inline void go(bool networked)
    {
        n_finished      = 0;
        n_authenticated = 0;
        n_intercepted   = 0;

        std::cout << "testing " << (networked ? "physical networking" : "memory networking") << " usage." << std::endl;

        // start hosts
        for (size_t i = 0; i < n_hosts; ++i)
        {
            m_peers[i].clear();
            m_auth_counts[i] = 0;

            fnet::host::networked_host_args m_args;
            m_args.m_ipv4 = fnet::ipv4(fnet::ipv4::host("localhost"), fnet::ipv4::default_port + i);

            std::cout << "creating host " << i << '.' << std::endl;

            bool success = m_hosts[i].start(fnet::host::flag_support_both, max_peers, m_args);
            fungus_util_assert(success, fungus_util::make_string("failed to create host ", i, "!"));

            success = m_hosts[i].get_message_factory_manager().add_factory(test_message::factory());
            fungus_util_assert(success, fungus_util::make_string("failed to add test message factory to host ", i, "!"));

            success = m_hosts[i].set_callbacks(this);
            fungus_util_assert(success, fungus_util::make_string("could not set callbacks for host ", i, "!"));

            success = m_hosts[i].set_message_intercept(fnet::all_peer_id, test_message_type);
            fungus_util_assert(success, fungus_util::make_string("could not set message intercept for host ", i, "!"));

            for (size_t j = 0; j < i; ++j)
            {
                std::cout << "connecting host " << i << " to host " << j << '.' << std::endl;

                fnet::auth_payload *m_payload = m_hosts[i].create_auth_payload(fnet::auth_status::success);
                m_payload->get_serializer() << fungus_util::make_string("Hullo, I am host number ", i, "!");

                fnet::peer *m_peer = nullptr;

                if (networked)
                    m_peer = m_hosts[i].connect(m_payload, 0, fnet::ipv4(fnet::ipv4::host("localhost"), fnet::ipv4::default_port + j));
                else
                    m_peer = m_hosts[i].connect(m_payload, 0, &m_hosts[j]);

                fungus_util_assert(m_peer, fungus_util::make_string("failed to connect host ", i, " to ", j, "!"));

                m_peer->set_user_data("auth sent");
                m_peers[i].insert(m_peer->get_id());
            }
        }

        // run test
        while (n_finished < n_hosts)
        {
            for (size_t i = 0; i < n_hosts; ++i)
            {
                host_i = i;
                m_hosts[i].dispatch();

                fnet::host::event m_event;
                while (m_hosts[i].next_event(m_event))
                {
                    switch (m_event.m_type)
                    {
                    case fnet::host::event::type::none:
                        std::cout << "host " << i << " none." << std::endl;
                        break;
                    case fnet::host::event::type::error:
                        std::cout << "host " << i << " error." << std::endl;
                        break;
                    default:
                        break;
                    };
                }
            }
        }

        // stop hosts
        for (size_t i = 0; i < n_hosts; ++i)
        {
            bool success = m_hosts[i].stop();
            fungus_util_assert(success, fungus_util::make_string("failed to stop host ", i, "!"));
        }
    }

    virtual fnet::host::callbacks::event_action on_peer_connected(const fnet::host::event &m_event)
    {
        m_peers[host_i].insert(m_event.m_id);

        std::cout << "host " << host_i << " peer_connected:" << std::endl
                  << "     peer_id=" << m_event.m_id
                  << " user_data=" << m_event.m_user_data << std::endl;

        return fnet::host::callbacks::event_action::discard;
    }

    virtual fnet::host::callbacks::event_action on_peer_disconnected(const fnet::host::event &m_event)
    {
        m_peers[host_i].erase(m_event.m_id);

        std::cout << "host " << host_i << " peer_disconnected:" << std::endl
                  << "     peer_id=" << m_event.m_id
                  << " user_data=" << m_event.m_user_data << std::endl;

        if (m_peers[host_i].size() == 0)
            ++n_finished;

        return fnet::host::callbacks::event_action::discard;
    }

    virtual fnet::host::callbacks::event_action on_peer_rejected(const fnet::host::event &m_event)
    {
        std::cout << "host " << host_i << " peer_rejected:" << std::endl
                  << "     peer_id=" << m_event.m_id
                  << " user_data=" << m_event.m_user_data << std::endl
                  << "     <break!>" << std::endl;

        n_finished = n_hosts;

        return fnet::host::callbacks::event_action::discard;
    }

    virtual fnet::host::callbacks::event_action on_received_auth_payload(const fnet::host::event &m_event)
    {
        fnet::auth_payload *m_payload = m_event.m_content.m_payload;
        bool success = m_payload->get_status() == fnet::auth_status::success;

        std::string m_string;
        m_payload->get_deserializer() >> m_string;

        std::cout << "host " << host_i << " received_auth_payload:" << std::endl
                  << "     peer_id=" << m_event.m_id
                  << " user_data=" << m_event.m_user_data << std::endl
                  << "     success=" << success
                  << " message=\"" << m_string << '\"' << std::endl;

        m_hosts[host_i].destroy_auth_payload(m_payload);
        ++m_auth_counts[host_i];

        std::cout << "     " << m_auth_counts[host_i] << " of "
                  << max_peers << " peers authenticated." << std::endl;

        if (m_auth_counts[host_i] >= max_peers)
        {
            ++n_authenticated;
            /*if (++n_authenticated >= n_hosts)
            {
                std::cout << "     disconnecting all." << std::endl;
                for (size_t j = 0; j < n_hosts; ++j)
                    m_hosts[j].disconnect(fnet::all_peer_id, 0, fnet::null_peer_id);
            }*/

            test_message *m_test_message = new test_message(host_i, fungus_util::make_string("I am a message from ", host_i, "!"));

            fnet::peer *m_all_peer = m_hosts[host_i].get_peer(fnet::all_peer_id);
            fungus_util_assert(m_all_peer, fungus_util::make_string("Could not get all_peer from host ", host_i, "!"));

            success = m_all_peer->send_message(m_test_message);
            fungus_util_assert(success, fungus_util::make_string("Could not send message through all_peer for ", host_i, "!"));
        }

        fnet::peer *m_peer = m_hosts[host_i].get_peer(m_event.m_id);
        if (m_peer->get_user_data() != "auth sent")
        {
            m_payload = m_hosts[host_i].create_auth_payload(fnet::auth_status::success);
            m_payload->get_serializer() << fungus_util::make_string("Reply from host number ", host_i, ".");

            m_hosts[host_i].send_auth_payload(m_peer, m_payload);
            m_peer->set_user_data("auth sent");
        }

        return fnet::host::callbacks::event_action::discard;
    }

    fnet::host::callbacks::event_action on_message_intercepted(const fnet::host::event &m_event)
    {
        std::cout << "intercepted message from " << m_event.m_sender_id << " on host " << host_i << ": ";

        if (m_event.m_content.m_message)
        {
            test_message *m_test_message =
                dynamic_cast<test_message *>(m_event.m_content.m_message);

            std::cout << '\"' << m_test_message->m_text << "\", " << m_test_message->m_host_i << std::endl;
            delete m_test_message;
        }
        else
            std::cout << "<INVALID>" << std::endl;

        if (++n_intercepted >= max_connections)
        {
            std::cout << "     disconnecting all." << std::endl;
            for (size_t j = 0; j < n_hosts; ++j)
                m_hosts[j].disconnect(fnet::all_peer_id, 0, fnet::null_peer_id);
        }

        return fnet::host::callbacks::event_action::discard;
    }
};

void test_host_move_semantics()
{
    std::cout << "testing host move semantics." << std::endl;

    fnet::host m_host_a;
    fnet::host m_host_b;

    std::cout << "m_host_a.is_moved() == " << m_host_a.is_moved() << std::endl
              << "m_host_b.is_moved() == " << m_host_b.is_moved() << std::endl
              << "moving..." << std::endl;

    m_host_b = std::move(m_host_a);

    std::cout << "m_host_a.is_moved() == " << m_host_a.is_moved() << std::endl
              << "m_host_b.is_moved() == " << m_host_b.is_moved() << std::endl;
}

int main(int argc, char *argv[])
{
    if (fungus_common::version_match())
    {
        std::cout << "using fungus_net version " << fungus_common::get_version_info().m_str << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.val==" << fungus_common::get_version_info().m_version.val << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.part.maj==" << (int)fungus_common::get_version_info().m_version.part.maj << std::endl;
        std::cout << "fungus_common::get_version_info().m_version.part.min==" << (int)fungus_common::get_version_info().m_version.part.min << std::endl;
    }
    else
    {
        std::cout << "version mismatch between API and lib! O_o" << std::endl;
        return 1;
    }

    if (fnet::initialize())
        std::cout << "fnet::initialize() succeeded." << std::endl;
    else
    {
        std::cout << "fnet::initialize() failed." << std::endl;
        return 1;
    }

    test_host_move_semantics();

    host_usage_test<test_peer_to_peer> m_test_a;
    m_test_a.go(true);
    m_test_a.go(false);

    std::cout << "done testing." << std::endl;

    if (fnet::deinitialize())
        std::cout << "fnet::deinitialize() succeeded." << std::endl;
    else
        std::cout << "fnet::deinitialize() failed." << std::endl;

    return 0;
}
