#ifndef FUNGUSNET_PEER_H
#define FUNGUSNET_PEER_H

#include "fungus_net_common.h"
#include "fungus_net_defs.h"
#include "fungus_net_message.h"

namespace fungus_net
{
    using namespace fungus_util;

    /** @addtogroup hostpeer_api Host/Peer API
      * @{
      */

    /** The peer class can represent a relationship with
      * another host (defined as a concrete peer) or a group
      * of concrete peers (defined as a peer group).  Each
      * peer has a state which specifies
      * the current status of the connection to the other
      * host represented by the peer.
      *
      * A peer can also represent a group of concrete peers or
      * peer groups.  In this case, the state of the group
      * will be the enumerator group.
      *
      * The API is exactly the same between peer groups and
      * concrete peers.  See member function docs for specific
      * minor differences between group and concrete implementations.
      */
    class FUNGUSNET_API peer
    {
    public:
        /// The current state of the connection to the other host.
        enum class state: uint32_t
        {
            none,               /**< Peer is inactive. */
            connecting,         /**< Peer is connecting, waiting for confirmation. */
            authenticating,     /**< Peer is connected, exchanging authentication information. */
            connected,          /**< Peer is connected and has been successfully authenticated. */
            disconnecting,      /**< Peer is disconnecting, waiting for confirmation. */
            disconnected,       /**< Peer has been disconnected. */
            group               /**< Peer represents a group of peer groups and/or concrete peers. */
        };

        /** Peer enumerator object functor base class.
          * An inheriting class overloading the operator()
          * can be passed to one of the enumeration functions.
          * For each peer enumerated, your operator() will be
          * called with the peer being enumerated.
          */
        class FUNGUSNET_API enumerator
        {
        public:
            /** Function operator.
              *
              * @param m_peer the peer being enumerated.
              *
              * Usage:
              * \code
              * class my_enumerator: public peer::enumerator
              * {
              * public:
              *     virtual void operator()(peer *m_peer)
              *     {
              *         ... do something with m_peer.
              *     }
              * };
              *
              * my_enumerator m_my_enumerator;
              * m_peer->enumerate_[groups|members|peers](m_my_enumerator);
              * \endcode
              */
            virtual void operator()(peer *m_peer) = 0;
        };

        /** Constant peer enumerator object functor base class.
          * Constant variant of enumerator.
          */
        class FUNGUSNET_API const_enumerator
        {
        public:
            /** Function operator.
              *
              * @param m_peer the peer being enumerated.
              *
              * Usage:
              * \code
              * class my_const_enumerator: public peer::const_enumerator
              * {
              * public:
              *     virtual void operator()(const peer *m_peer)
              *     {
              *         ... do something with m_peer.
              *     }
              * };
              *
              * my_const_enumerator m_my_const_enumerator;
              * m_peer->enumerate_[groups|members|peers](m_my_const_enumerator);
              * \endcode
              */
            virtual void operator()(const peer *m_peer) = 0;
        };

        /** Structure containing an incoming message.  Specifies
          * the concrete peer from which the message originated.
          */
        struct FUNGUSNET_API incoming_message
        {
            message *m_message; /**< a pointer to the incoming message. */
            peer_id  sender_id; /**< the concrete peer representing the host that sent the message. */

            /// Constructor
            incoming_message();

            /** Copy constructor
              *
              * @param m_incoming_message the incoming_message to copy.
              */
            incoming_message(const incoming_message &m_incoming_message);

            /** Constructor
              *
              * @param m_message    a pointer to the message.
              * @param sender_id    the concrete peer representing the host that sent the message.
              */
            incoming_message(message *m_message, peer_id sender_id);
        };

    protected:
        peer();
        virtual ~peer();

    public:
        peer(peer &&m_peer)                  = delete;
        peer(const peer &m_peer)             = delete;

        peer &operator =(peer &&m_peer)      = delete;
        peer &operator =(const peer &m_peer) = delete;

        /** @{ */

        /** Query the peer_id handle for this peer.
          *
          * @returns the peer_id handle for this peer.
          */
        virtual peer_id  get_id()        const = 0;

        /** Query the current state of this peer.
          *
          * @retval != group the current state of this concrete peer.
          * @retval == group this peer represents a peer group.
          */
        virtual state    get_state()     const = 0;

        /** @} */
        /** @{ */

        /** Set the user data associated with this peer (move semantics).
          *
          * @param data an rvalue reference to the any_type containing the data (data will be moved).
          */
        virtual void     set_user_data(any_type &&data)      = 0;

        /** Set the user data associated with this peer (copy semantics).
          *
          * @param data a reference to the any_type containing the data (data will be copied).
          */
        virtual void     set_user_data(const any_type &data) = 0;

        /** Get the user data associated with this peer.
          *
          * @returns a copy of the user data associated with this peer.
          */
        virtual any_type get_user_data() const               = 0;

        /** @} */
        /** @{ */

        /** Get the number of groups that this peer belongs to.
          *
          * @returns the number of groups that this peer belongs to.
          */
        virtual size_t   count_groups()  const = 0;

        /** Get the number of members that this peer group has.
          *
          * If this peer represents a peer group:
          * @returns the number of members that this peer group has.
          *
          * If this peer represents a concrete peer:
          * @returns always 0.  A concrete peer represents a connection to a host and cannot have members.
          */
        virtual size_t   count_members() const = 0;

        /** Get the number of concrete peers referenced by this peer.
          *
          * If this peer represents a peer group:
          * @returns the number of concrete peers referenced directly and indirectly from this group.
          *
          * If this peer represents a concrete peer:
          * @returns always 1.  A concrete peer always references itself.
          */
        virtual size_t   count_peers()   const = 0;

        /** @} */
        /** @{ */

        /** Enumerate all peer groups that this peer belongs to.
          *
          * @param m_enum the \ref peer::enumerator "enumerator" functor object to call for each peer group that this peer belongs to.
          */
        virtual void     enumerate_groups (enumerator &m_enum) = 0;

        /** Enumerate all members in this peer group.  If this peer does not represent a peer group,
          * nothing is done, and the function returns immediately.
          *
          * @param m_enum the \ref peer::enumerator "enumerator" functor object to call for each member in this peer group.
          */
        virtual void     enumerate_members(enumerator &m_enum) = 0;

        /** Enumerate all concrete peers referenced by this peer.  If this peer represents a concrete
          * peer, the enumerator is called with this concrete peer only.
          *
          * @param m_enum       the \ref peer::enumerator "enumerator" functor object to call for each concrete peer referenced by this peer.
          * @param m_exclusion  all concrete peers referenced by m_exclusion are excluded.  If m_exclusion == nullptr,
          *                     then no concrete peers are excluded.
          *
          */
        virtual void     enumerate_peers  (enumerator &m_enum, const peer *m_exclusion = nullptr) = 0;

        /** Enumerate all peer groups that this peer belongs to.
          *
          * @param m_enum the const_enumerator functor object to call for each peer group that this peer belongs to.
          */
        virtual void     enumerate_groups (const_enumerator &m_enum) const = 0;

        /** Enumerate all members in this peer group.  If this peer does not represent a peer group,
          * nothing is done, and the function returns immediately.
          *
          * @param m_enum the const_enumerator functor object to call for each member in this peer group.
          */
        virtual void     enumerate_members(const_enumerator &m_enum) const = 0;

        /** Enumerate all concrete peers referenced by this peer.  If this peer represents a concrete
          * peer, the enumerator is called with this concrete peer only.
          *
          * @param m_enum       the const_enumerator functor object to call for each concrete peer referenced by this peer.
          * @param m_exclusion  all concrete peers referenced by m_exclusion are excluded.  If m_exclusion == nullptr,
          *                     then no concrete peers are excluded.
          *
          */
        virtual void     enumerate_peers  (const_enumerator &m_enum, const peer *m_exclusion = nullptr) const = 0;

        /** @} */
        /** @{ */

        /** Add a peer as a member to this peer group.  This operation will fail if this peer does not
          * represent a peer group or if this peer group already contains m_peer a member.
          *
          * @param m_peer the peer to add as a member to this peer group.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        virtual bool     add_member(peer *m_peer)    = 0;

        /** Remove a member peer from this peer group.  This operation will fail if this peer does not
          * represent a peer group or if this peer group does not contain m_peer as a member.
          *
          * @param m_peer the member peer to remove from this peer group.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        virtual bool     remove_member(peer *m_peer) = 0;

        /** @} */
        /** @{ */

        /** Query this peer as to whether or not it contains another peer as a member.
          *
          * @param m_peer the peer to check for.
          *
          * @retval true    this peer group contains m_peer as a member.
          * @retval false   this peer group does not contain m_peer as a member or this peer represents a concrete peer.
          */
        virtual bool     has_member(peer *m_peer) const = 0;

        /** Query this peer as to whether or not it references a concrete peer.
          *
          * @param m_peer the concrete peer to check for.
          *
          * @retval true    this peer references m_peer.
          * @retval false   this peer does not reference m_peer or m_peer represents a peer group.
          */
        virtual bool     has_peer(peer *m_peer)   const = 0;

        /** @} */
        /** @{ */

        /** Send a message to all concrete peers referenced by this peer.  If this peer represents a
          * concrete peer, then the message is sent only to the host represented by this peer, otherwise
          * the message is sent to every concrete peer referenced by this peer group.
          *
          * @param m_message    the message to send
          * @param m_exclusion  all concrete peers referenced by m_exclusion are excluded.  If m_exclusion == nullptr,
          *                     then no concrete peers are excluded.
          *
          * @retval true    if every concrete peer referenced by this peer excluding the concrete peers referenced by
          *                 m_exclusion successfully sent the message.
          * @retval false   if any of the concrete peers referenced by this peer excluding the concrete peers referenced
          *                 by m_exclusion failed to send the message.
          */
        virtual bool     send_message(const message *m_message, const peer *m_exclusion = nullptr) = 0;

        /** Retrieve the next message waiting in the incoming message queue of this peer.  If this peer
          * represents a peer group, then messages from all concrete peers referenced by this peer group
          * are retrieved, otherwise only messages from the host represented by this concrete peer will
          * be retrieved.  Unless you call message::mark_as_read() on the message retrieved, a clone of this
          * message can still be retrieved from any peer groups that reference the concrete peer that
          * represents the host which sent this message, and the aforementioned concrete peer itself.
          *
          * @param m_in a reference to the incoming_message structure in which to place the incoming
          *             message an the peer_id handle to the concrete peer representing the host that
          *             sent this message.
          *
          * @retval true    the next message waiting in the queue was placed in m_in and popped off of the queue.
          * @retval false   no messages were waiting in the queue, and nothing was placed in m_in.
          */
        virtual bool     receive_message(incoming_message &m_in)                                   = 0;

        /** Discard the next message waiting in the incoming message queue of this peer, if any.  This
          * should never be used in robust code, but is included for completeness.
          */
        virtual void     discard_message()      = 0;

        /** Discard all messages waiting in the incoming message queue of this peer, if any.  Calling
          * this before disconnecting the peer can increase performance if many peers will be disconnected
          * at once, especially if this entire peer group will be disconnected.
          */
        virtual void     discard_all_messages() = 0;

        /** @} */
    };

    /** @} */
}

#endif
