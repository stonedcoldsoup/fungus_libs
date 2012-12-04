#include "fungus_net_peer_internal.h"

namespace fungus_net
{
    peer::incoming_message::incoming_message(): m_message(nullptr), sender_id(null_peer_id) {}
    peer::incoming_message::incoming_message(const incoming_message &m_incoming_m_message):
        m_message(m_incoming_m_message.m_message), sender_id(m_incoming_m_message.sender_id)
    {}

    peer::incoming_message::incoming_message(message *m_message, peer_id sender_id):
        m_message(m_message), sender_id(sender_id)
    {}

    peer::peer()  = default;
    peer::~peer() = default;

    peer_base::peer_base(message_intercept::map &m_intercept_map, peer_id m_id, state m_state):
        multi_tree_node<peer_base>(this,
                                   m_state == state::group ?
                                   multi_tree_node_group   :
                                   multi_tree_node_leaf),
        m_intercept_map(m_intercept_map),
        m_state(m_state), m_id(m_id), m_data(),
        m_message_queue_in()
    {}

    peer_base::~peer_base()
    {
        discard_all_messages();
    }

    void peer_base::discard_message()
    {
        incoming_message m_in;
        if (receive_message(m_in))
            delete m_in.m_message;
    }

    void peer_base::discard_all_messages()
    {
        incoming_message m_in;
        while (receive_message(m_in))
            delete m_in.m_message;
    }

    void peer_base::push_incoming_message(const incoming_message &m_in)
    {
        if (!m_intercept_map.intercept(m_in.m_message, m_in.sender_id, this))
        {
            m_message_queue_in.push(m_in);

            auto it_factory =
                multi_tree_iterator_factory
                <
                    peer_base,
                    multi_tree_iterator_target::groups
                >(this);

            for (auto &entry: it_factory)
            {
                entry.key->push_incoming_message
                (
                    incoming_message(m_in.m_message->clone(),
                                     m_in.sender_id)
                );
            }
        }
    }

    void peer_base::set_user_data(any_type &&data)
    {
        m_data = std::move(data);
    }

    void peer_base::set_user_data(const any_type &data)
    {
        m_data = data;
    }

    any_type peer_base::get_user_data() const
    {
        return m_data;
    }

    peer_id peer_base::get_id()        const {return m_id;}
    peer::state peer_base::get_state() const {return m_state;}

    size_t peer_base::count_groups()   const {return multi_tree_node<peer_base>::count_groups();}
    size_t peer_base::count_members()  const {return multi_tree_node<peer_base>::count_nodes();}
    size_t peer_base::count_peers()    const {return multi_tree_node<peer_base>::count_leaves();}

    void peer_base::enumerate_groups(enumerator &m_enum)
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::groups
            >(this);

        for (auto &entry: it_factory)
            m_enum(entry.key);
    }

    void peer_base::enumerate_members(enumerator &m_enum)
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::nodes
            >(this);

        for (auto &entry: it_factory)
            m_enum(entry.key);
    }

    void peer_base::enumerate_peers(enumerator &m_enum, const peer *m_exclusion)
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::leaves
            >(this);

        if (m_exclusion != nullptr)
        {
            for (auto &entry: it_factory)
            {
                if (!m_exclusion->has_peer(entry.key))
                    m_enum(entry.key);
            }
        }
        else
        {
            for (auto &entry: it_factory)
                m_enum(entry.key);
        }
    }

    void peer_base::enumerate_groups(const_enumerator &m_enum) const
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::groups
            >(this);

        for (const auto &entry: it_factory)
            m_enum(entry.key);
    }

    void peer_base::enumerate_members(const_enumerator &m_enum) const
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::nodes
            >(this);

        for (const auto &entry: it_factory)
            m_enum(entry.key);
    }

    void peer_base::enumerate_peers(const_enumerator &m_enum, const peer *m_exclusion) const
    {
        auto it_factory =
            multi_tree_iterator_factory
            <
                peer_base,
                multi_tree_iterator_target::leaves
            >(this);

        if (m_exclusion != nullptr)
        {
            for (const auto &entry: it_factory)
            {
                if (!m_exclusion->has_peer(entry.key))
                    m_enum(entry.key);
            }
        }
        else
        {
            for (const auto &entry: it_factory)
                m_enum(entry.key);
        }
    }

    bool peer_base::add_member(peer *m_peer)
    {
        return multi_tree_node<peer_base>::add_node(static_cast<peer_base *>(m_peer));
    }

    bool peer_base::remove_member(peer *m_peer)
    {
        return multi_tree_node<peer_base>::remove_node(static_cast<peer_base *>(m_peer));
    }

    bool peer_base::has_member(peer *m_peer) const
    {
        return multi_tree_node<peer_base>::has_node(static_cast<peer_base *>(m_peer));
    }

    bool peer_base::has_peer(peer *m_peer)   const
    {
        return multi_tree_node<peer_base>::has_leaf(static_cast<peer_base *>(m_peer));
    }

    bool peer_base::receive_message(incoming_message &m_in)
    {
        bool success   = false;
        m_in.m_message = nullptr;
        m_in.sender_id = null_peer_id;

        while (!m_message_queue_in.empty())
        {
            m_in = m_message_queue_in.front();
            m_message_queue_in.pop();

            if (m_in.m_message->marked_as_read())
            {
                delete m_in.m_message;

                m_in.m_message = nullptr;
                m_in.sender_id = null_peer_id;
            }
            else
            {
                success = true;
                break;
            }
        }

        return success;
    }
}
