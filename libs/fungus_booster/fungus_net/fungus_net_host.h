#ifndef FUNGUSNET_HOST_H
#define FUNGUSNET_HOST_H

#include "fungus_net_common.h"
#include "fungus_net_peer.h"
#include "fungus_net_message_factory_manager.h"
#include "fungus_net_authenticator.h"

namespace fungus_net
{
    using namespace fungus_util;

    /** @addtogroup hostpeer_api Host/Peer API
      * @{
      */

    /** \brief The host class represents a networked
      * host that can accept and request connections
      * from other hosts, and contains a map of peers.
      *
      * See documentation for #fungus_net::peer for more information
      * on peers and groups.
      *
      * The host also has a peer group management
      * interface, a callback structure for handling
      * events from the callbacks, and message intercept
      * handling.
      *
      */
    class FUNGUSNET_API host
    {
    private:
        class impl;
        impl *pimpl_;
        mutable mutex m;
    public:
        /// Flags for use with start().
        enum flag: uint32_t
        {
            flag_support_memory_connection    = 0x1,    /**< Support direct connection through memory to another host object. */
            flag_support_networked_connection = 0x2,    /**< Support networked connection to another host object. */

            /// Support both networked and memory connections.
            flag_support_both                 = flag_support_memory_connection |
                                                flag_support_networked_connection
        };

        /** @{ */

        /// Arguments for start() for networked connection support.
        struct FUNGUSNET_API networked_host_args
        {
            ipv4 m_ipv4;            /**< The ipv4 address to bind the host to. */

            uint32_t in_bandwidth;  /**< Maximum incoming bandwidth.  A value of 0 enforces no limit. */
            uint32_t out_bandwidth; /**< Maximum outgoing bandwidth.  A value of 0 enforces no limit. */

            /** Constructor
              *
              * @param m_ipv4           the ipv4 address to bind the host to.
              * @param in_bandwidth     maximum incoming bandwidth.  A value of 0 enforces no limit.
              * @param out_bandwidth    maximum outgoing bandwidth.  A value of 0 enforces no limit.
              */
            inline networked_host_args(const ipv4 &m_ipv4 = ipv4(),
                                       uint32_t in_bandwidth  = 0,
                                       uint32_t out_bandwidth = 0):
                m_ipv4(m_ipv4),
                in_bandwidth(in_bandwidth),
                out_bandwidth(out_bandwidth)
            {}
        };

        /// Arguments for start() that specify the global timeout periods for various protocols.
        struct FUNGUSNET_API timeout_period_args
        {
            sec_duration_t connection_timeout_period; /**< Connection timeout period in seconds. */
            sec_duration_t disconnect_timeout_period; /**< Disconnect timeout period in seconds. */
            sec_duration_t auth_timeout_period;       /**< Authentication timeout period in seconds. */

            /** Constructor.
              *
              * @param connection_timeout_period    Connection timeout period in seconds.
              * @param disconnect_timeout_period    Disconnect timeout period in seconds.
              * @param auth_timeout_period          Authentication timeout period in seconds.
              */
            inline timeout_period_args(sec_duration_t connection_timeout_period = default_connection_timeout_period,
                                       sec_duration_t disconnect_timeout_period = default_disconnect_timeout_period,
                                       sec_duration_t auth_timeout_period       = default_auth_timeout_period):
                connection_timeout_period(connection_timeout_period),
                disconnect_timeout_period(disconnect_timeout_period),
                auth_timeout_period(auth_timeout_period)
            {}
        };

        /** @} */

        /** An event structure represents an event that occured
          * during a call of dispatch() for this host.
          */
        struct FUNGUSNET_API event
        {
            /// Event type
            enum class type: uint32_t
            {
                none = 0,               /**< No event occurred. */
                error,                  /**< An error occurred. */

                /** Peer successfully connected.
                  *
                  * event::m_id contains the peer_id of the peer that has successfully connected.\n
                  * event::content::data contains the uint32 passed to connect().
                  * This can be used to send a small client type code to a server, for example.
                  */
                peer_connected,

                /** Peer has disconnected.
                  *
                  * event::m_id contains the peer_id of the peer that has disconnected.\n
                  * event::m_user_data contains the user data associated with this peer.\n
                  * event::content::data contains the uint32 passed to disconnect().
                  * The value will be 0 if disconnect occurred because of a hard stop()
                  * on the other host, or the connection was reset.
                  */
                peer_disconnected,

                /** Peer rejected.
                  * Connection to another host represented by this peer was rejected.
                  *
                  * event::m_id contains the peer_id of the peer that was rejected. (this peer has been subsequently destroyed).\n
                  * event::m_user_data contains the user data associated with this peer.\n
                  * event::content::data contains a rejection_reason.
                  */
                peer_rejected,

                /** Received an authentication payload.\n
                  * An authentication payload from another host represented by this peer was received.
                  *
                  * event::m_id contains the peer_id of the peer that received the authentication payload. (this peer has been subsequently destroyed).\n
                  * event::m_user_data contains the user data associated with this peer.\n
                  * event::content::m_payload is a pointer to the auth_payload representing the authentication payload.
                  */
                received_auth_payload,

                /** An authenticating peer timed out.
                  * Authentication for a connection to another host represented by this peer timed out.
                  *
                  * event::m_id contains the peer_id of the peer that timed out.\n
                  * event::m_user_data contains the user data associated with this peer.
                  */
                auth_timeout,

                /** A message was intercepted.\n
                  * A message received from another host was intercepted.
                  *
                  * event::m_id contains the peer_id of the peer or peer group at which the message was intercepted.\n
                  * event::m_sender_id contains the peer_id of the peer representing the host that sent the message.\n
                  * event::m_user_data contains the user data associated with the peer or peer group at which the message was intercepted.\n
                  * event::content::m_message is a pointer to the message that was intercepted.  This message has
                  * been automatically marked as read. (see message::marked_as_read() and message::mark_as_read()).
                  */
                message_intercepted,

                /// The number of event types.
                count
            };

            /** @{ */

            /// Error codes.
            enum error_type: uint32_t
            {
                internal_error            = 0, /**< An internal uncoded error has occurred. */
                unknown_peer_disconnected = 1, /**< An peer_disconnected event was generated for a peer that does not exist. */
                unknown_peer_rejected     = 2  /**< An peer_rejected event was generated for a peer that does not exist. */
            };

            /// Rejection reasons.
            enum rejection_reason: uint32_t
            {
                rejection_timed_out = 10, /**< The peer was implicitly rejected because the attempt to connect to the other host timed out. */
                rejection_denied    = 11  /**< The peer was rejected because the other host denied the connection. */
            };

            /** @} */

            /// Content union.
            union FUNGUSNET_API content
            {
                uint32_t               data;        /**< uint32 data (used for peer_connected, peer_disconnected, and peer_rejected events) */
                auth_payload          *m_payload;   /**< pointer to an authentication payload (used for received_auth_payload events) */
                message               *m_message;   /**< pointer to a message (used for message_intercepted events) */

                /// Constructor.
                content();

            private:
                friend class event;

                content(uint32_t data);
                content(message *m_message);
                content(auth_payload *m_payload);
            };

            type     m_type;      /**< Contains the event type */
            /** @{ */
            peer_id  m_id;        /**< Contains the peer_id of the peer referenced by this event. */
            peer_id  m_sender_id; /**< Contains the peer_id of the peer that sent the intercepted message (used for message_intercepted events) */
            /** @} */
            any_type m_user_data; /**< Contains the user data associated with the peer referenced by this event. */

            content  m_content;   /**< Content union. */

            /// Constructor.
            event();

            /** Move constructor.
              *
              * @param m_event the event to move.
              */
            event(event &&m_event);

            /** Copy constructor.
              *
              * @param m_event the event to copy.
              */
            event(const event &m_event);

            /** Move assignment.
              *
              * @param m_event the event to move.
              * @returns a mutable reference to this event.
              */
            event &operator =(event &&m_event);

            /** Copy assignment.
              *
              * @param m_event the event to copy.
              * @returns a mutable reference to this event.
              */
            event &operator =(const event &m_event);

        private:
            friend class host::impl;

            event(peer_id m_id, const any_type &m_user_data, auth_payload *m_payload);                    // auth payload event constructor
            event(peer_id m_id, const any_type &m_user_data, const peer::incoming_message &m_in_message); // message intercept event constructor
            event(type m_type, peer_id m_id, const any_type &m_user_data, uint32_t data);                 // other event constructor
        };

        /** A callback class to inherit from.
          *
          * The callbacks class is a base class for the callbacks
          * taken by host.  Each callback returns a callbacks::event_action
          * which specifies what to do with the event after the callback
          * is called and takes the event as it's only argument.
          *
          */
        class FUNGUSNET_API callbacks
        {
        public:
            /** Implementation flags.
              *
              * The implementation flags specify which callbacks are implemented by
              * this callbacks class.  You can use more than one callbacks object,
              * but any attempt to do so will fail if more than one of your callbacks
              * objects implements the same event callback.
              *
              * Inheritance should work as follows:
              * <code>
              * class my_callbacks: public callbacks
              * {
              * public:
              *     my_callbacks(): callbacks(flags) {}
              *     ...
              * };
              * </code>
              *
              */
            enum flags: uint32_t
            {
                implements_on_error                 = 0x01, /**< This callbacks class implements a callback for an event::error event. */
                implements_on_peer_connected        = 0x02, /**< This callbacks class implements a callback for an event::peer_connected event. */
                implements_on_peer_disconnected     = 0x04, /**< This callbacks class implements a callback for an event::peer_disconnected event. */
                implements_on_peer_rejected         = 0x08, /**< This callbacks class implements a callback for an event::peer_rejected event. */
                implements_on_received_auth_payload = 0x10, /**< This callbacks class implements a callback for an event::received_auth_payload event. */
                implements_on_message_intercepted   = 0x20, /**< This callbacks class implements a callback for an event::message_intercepted event. */

                /** This callbacks class implements callbacks for all of the peer low
                  * level connection events (peer_connected, peer_disconnected, peer_rejected)
                  */
                implements_on_peer_x = implements_on_peer_connected    |
                                       implements_on_peer_disconnected |
                                       implements_on_peer_rejected,

                /** This callbacks class implements callbacks for all of the peer management
                  * events (peer_connected, peer_disconnected, peer_rejected, received_auth_payload)
                  */
                implements_peer_management = implements_on_peer_x |
                                             implements_on_received_auth_payload,

                /// This callbacks class implements callbacks for all events except for the error event.
                implements_all_but_error = implements_peer_management |
                                           implements_on_message_intercepted,

                /// This callbacks class implements callbacks for all events.
                implements_all = implements_on_error        |
                                 implements_peer_management |
                                 implements_on_message_intercepted,

                __max_flag = 0x40
            };

            /// What to do after a callback is called.
            enum class event_action: uint32_t
            {
                keep,   /**< Keep the event and push it down the event queue (default). */
                discard /**< Discard the event. */
            };

            const uint32_t flags; /**< Contains the implementation flags. */

            /** Constructor.
              *
              * @param flags the implementation flags indicating which callbacks are implemented by this callbacks class.
              */
            callbacks(uint32_t flags);

            /** Called when an error event is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_error(const event &m_event);

            /** Called when a peer_connected event is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_peer_connected(const event &m_event);

            /** Called when a peer_disconnected event is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_peer_disconnected(const event &m_event);

            /** Called when a peer_rejected is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_peer_rejected(const event &m_event);

            /** Called when a received_auth_payload event is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_received_auth_payload(const event &m_event);

            /** Called when a message_intercepted event is thrown.
              *
              * @param m_event the event.
              * @returns event_action specifying what to do with the event.
              */
            virtual event_action on_message_intercepted(const event &m_event);
        };

        /// Constructor.
        host();

        /** Move constructor.
          *
          * @param m_host host to move.
          */
        host(host &&m_host);

        /** Move assignment.
          *
          * @param m_host host to move.
          * @returns a mutable reference to the host.
          */
        host &operator =(host &&m_host);

        host(const host &m_host)             = delete;
        host &operator =(const host &m_host) = delete;

        /// Destructor.
        ~host();

        /** @{ */

        /** Queries whether or not the host has been moved to a different host object.
          *
          * @retval true    the host has been moved to another host object.
          * @retval false   the host has not been moved to another host object.
          */
        bool is_moved() const;

        /** Queries the host as to whether or not it is running.
          *
          * @retval true    the host is running (between a successful call of start() and a call of stop()).
          * @retval false   the host is not running.
          */
        bool   is_running()    const;

        /** Queries the host about the maximum number of simultaneous connections it supports.
          *
          * @returns The maximum number of simultaneous connections supported.
          */
        size_t get_max_peers() const;

        /** @} */
        /** @{ */

        /** Starts the host.
          *
          * @param flags            Flags specifying what connection types to support (see #flag)
          * @param max_peers        Maximum number of peer connections to support simultaneously.
          * @param m_net_args       A structure containing arguments for a networked host (only applicable if flags includes flag_support_networked_connection)
          * @param m_time_args      A structure containing the timeout periods for all applicable protocols (connecting, disconnecting, authenticating)
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool start(uint32_t flags, size_t max_peers,
                   const networked_host_args &m_net_args  = networked_host_args(),
                   const timeout_period_args &m_time_args = timeout_period_args());

        /** Stops the host.
          *
          * This is a hard stop, so all connections will be reset, and all hosts currently
          * connected to this host will time out and drop their ends of the connections.
          *
          * For robustness, it is reccomended that you wait for all peers to disconnect before
          * stopping the host.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool stop();

        /** @} */
        /** @{ */

        /** Retrieves a peer by its handle.
          *
          * @param  m_id        the peer_id for the peer to retrieve.
          * @retval == nullptr  no peer exists on this host for this peer_id.
          * @retval > nullptr   a pointer to the peer object.
          */
              peer *get_peer(peer_id m_id);

        /** Retrieves a peer by its handle.
          *
          * @param  m_id        the peer_id for the peer to retrieve.
          * @retval == nullptr  no peer exists on this host for this peer_id.
          * @retval > nullptr   a pointer to the peer object.
          */
        const peer *get_peer(peer_id m_id) const;

        /** @} */
        /** @{ */

        /** Gets the message_factory_manager associated with this host.
          *
          * @returns the message_factory_manager associated with this host.
          */
              message_factory_manager &get_message_factory_manager();

        /** Gets the message_factory_manager associated with this host.
          *
          * @returns the message_factory_manager associated with this host.
          */
        const message_factory_manager &get_message_factory_manager() const;

        /** Gets the endian_converter associated with this host.
          *
          * @returns the endian_converter associated with this host.
          */
              endian_converter &get_endian_converter();

        /** Gets the endian_converter associated with this host.
          *
          * @returns the endian_converter associated with this host.
          */
        const endian_converter &get_endian_converter() const;

        /** @} */
        /** @{ */

        /** Create a new peer group
          *
          * @returns a new peer group.
          */
        peer *create_group();

        /** Destroy a peer group.
          *
          * @param m_peer   the peer group to destroy.  NEVER pass the all_peer group to destroy_group()
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool  destroy_group(peer *m_peer);

        /** Destroy a peer group.
          *
          * @param m_id     the peer_id handle to the peer group to destroy.  NEVER pass all_peer_id group to destroy_group()
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool  destroy_group(peer_id m_id);

        /** @} */
        /** @{ */

        /** Create a new authentication payload.
          *
          * @param m_status the auth_status of the new auth_payload.
          * @returns a pointer to the new authentication payload.
          */
        auth_payload *create_auth_payload(auth_status m_status) const;

        /** Destroy an authentication payload.
          *
          * This is not a safe operation, NEVER pass an invalid pointer to destroy_auth_payload().
          *
          * @param m_payload    the auth_payload to destroy.
          */
        void destroy_auth_payload(auth_payload *m_payload)      const;

        /** @} */
        /** @{ */

        /** Connect to another host object using a networked connection.
          *
          * @param m_payload    The auth_payload to be sent immediately upon successful connection.  Must not be nullptr.
          * @param data         A uint32 to be sent to the other host.  This data will be reported in the
          *                     connection event on both hosts under event::content::data.
          * @param m_ipv4       The address of the other host.
          *
          * @retval == nullptr  Failed to create a new peer for the connection because
          *                     of an internal error or because the maximum allowed peers are already
          *                     connected or connecting.
          * @retval > nullptr   a pointer to the new peer representing the connection.
          */
        peer *connect(auth_payload *m_payload, uint32_t data, const ipv4 &m_ipv4);

        /** Connect to another host object using a memory connection.
          *
          * @param m_payload    The auth_payload to be sent immediately upon successful connection.  Must not be nullptr.
          * @param data         A uint32 to be sent to the other host.  This data will be reported in the
          *                     connection event on both hosts under event::content::data.
          * @param m_host       A pointer to the other host object.
          *
          * @retval == nullptr  Failed to create a new peer for the connection because
          *                     of an internal error or because the maximum allowed peers are already
          *                     connected or connecting.
          * @retval > nullptr   a pointer to the new peer representing the connection.
          */
        peer *connect(auth_payload *m_payload, uint32_t data, host *m_host);

        /** @} */
        /** @{ */

        /** Send an authentication payload to the other host represented by the peer.
          *
          * @param m_peer       The peer representing the other host to send the auth_payload to.
          * @param m_payload    The auth_payload to send.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool send_auth_payload(peer *m_peer, auth_payload *m_payload);

        /** Send an authentication payload to the other host represented by the peer.
          *
          * @param m_id         The peer_id handle to the peer representing the other host to send the auth_payload to.
          * @param m_payload    The auth_payload to send.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool send_auth_payload(peer_id m_id, auth_payload *m_payload);

        /** @} */
        /** @{ */

        /** Disconnect from another host(s) represented by the peer or peer group.  Gently disconnects
          * from the other host(s) using a protocol command, ensuring that the resulting
          * behavior will be more robust.
          *
          * @param m_peer       The peer or peer group representing the other host(s) to disconnect from.
          * @param data         A uint32 to be sent to the other host(s).  This data will be reported in the
          *                     disconnect event on this host and the other host(s) under event::content::data.
          * @param m_exclusion  A peer or peer group representing the other host(s) to exclude.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool disconnect(peer *m_peer, uint32_t data, const peer *m_exclusion);

        /** Disconnect from another host(s) represented by the peer or peer group.  Gently disconnects
          * from the other host(s) using a protocol command, ensuring that the resulting
          * behavior will be more robust.
          *
          * @param m_id             The peer_id handle to the peer or peer group representing the other host(s) to disconnect from.
          * @param data             A uint32 to be sent to the other host(s).  This data will be reported in the
          *                         disconnect event on this host and the other host(s) under event::content::data.
          * @param m_exclusion_id   peer_id handle to a peer or peer group representing the other host(s) to exclude.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool disconnect(peer_id m_id, uint32_t data, peer_id m_exclusion_id);

        /** @} */
        /** @{ */

        /** Dispatch all.
          *
          * This function must be called frequently in order to acheive good networking performance
          * and reliability (ideally once in your main loop).  This function will actually send
          * all messages in outgoing queues, throw events, recycle all unused peer_ids, and handle
          * state changes for each peer.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool dispatch();

        /** Get the next event in the event queue and pop it off of the queue.
          *
          * @param m_event  a mutable reference to an event structure to be filled with data concerning the event.
          *
          * @retval true    an event was waiting in the event queue and has been placed in m_event.
          * @retval false   no event was waiting and m_event is unchanged.
          */
        bool next_event(event &m_event);

        /** Get the next event in the event queue and leave it in the queue.
          *
          * @param m_event  a mutable reference to an event structure to be filled with data concerning the event.
          *
          * @retval true    an event was waiting in the event queue and has been placed in m_event.
          * @retval false   no event was waiting and m_event is unchanged.
          */
        bool peek_event(event &m_event) const;

        /** @} */
        /** @{ */

        /** Set a message intercept.
          *
          * Maps a message intercept to the peer or peer group and message_type.  Any message
          * of type m_type that can be received through the peer at the peer_id handle m_id
          * will be intercepted until unset_message_intercept() is called with the same arguments.
          *
          * @param m_id     The peer_id handle to the peer or peer group at which a message of this message_type should be intercepted.
          * @param m_type   The message_type of the message to intercept at this peer.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool    set_message_intercept(peer_id m_id, message_type m_type);

        /** Disable a message intercept.
          *
          * @param m_id     The peer_id handle to the peer or peer group at which a message of this message_type should no longer be intercepted.
          * @param m_type   The message_type of the message to no longer intercept at this peer.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool  unset_message_intercept(peer_id m_id, message_type m_type);

        /** Query the host as to whether or not a message intercept is set.
          *
          * @param m_id     The peer_id handle to the peer or peer group at which to check if a message of this message_type will be intercepted.
          * @param m_type   The message_type to check against.
          *
          * @retval true    if the message intercept is set.
          * @retval false   if the message intercept is disabled or nonexistent.
          */
        bool is_message_intercept_set(peer_id m_id, message_type m_type) const;

        /** @} */
        /** @{ */

        /** Add a callbacks object to the host.  If a callbacks object
          * has already been added to the host and if the aforementioned callbacks
          * object implements any of the same event callbacks (as indicated by the
          * implementation flags) then it will fail.
          *
          * @param m_callbacks  a pointer to the callbacks object to add.
          *
          * @retval true    on success.
          * @retval false   on failure.
          */
        bool set_callbacks(callbacks *m_callbacks);

        /** Remove a callbacks object to the host.  If the callbacks object
          * is not in use by this host, remove_callbacks() will fail silently.
          *
          * @param m_callbacks  a pointer to the callbacks object to remove.
          */
        void remove_callbacks(callbacks *m_callbacks);

        /// Clear all callbacks object from the host.
        void clear_callbacks();

        /** @} */
    };

    /** @} */
}

#endif
