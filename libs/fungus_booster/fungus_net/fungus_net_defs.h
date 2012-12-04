#ifndef FUNGUSNET_DEFS_H
#define FUNGUSNET_DEFS_H

#include "../fungus_util/fungus_util.h"

#ifdef DLL_FUNGUSUTIL
    #ifdef BUILD_FUNGUSNET
        #define FUNGUSNET_API __attribute__ ((dllexport))
    #else
        #define FUNGUSNET_API __attribute__ ((dllimport))
    #endif
#elif SO_FUNGUSUTIL
    #ifdef BUILD_FUNGUSNET
        #define FUNGUSNET_API __attribute__ ((visibility("hidden")))
    #else
        #define FUNGUSNET_API
    #endif
#else
    #define FUNGUSNET_API
#endif

namespace fungus_net
{
    using namespace fungus_util;

    class host;
    class peer;

    /** @defgroup definitions Basic Definitions
      * @{
      */

    /** @{ */

    /** \brief A unique message type.
      *
      * A unique message type for a user defined message
      * and it's corresponding factory class.  This is effectively a
      * handle used by fungus_net internally to determine which factory
      * object should create an incoming message.
      *
      */
    typedef int message_type;

    /** \brief A handle to a peer.
      *
      * A unique (within the scope of a single host object) handle
      * to identify a peer object.  There are two special values for
      * peer_id reserved for a nil peer and the special peer group
      * containing all peers.
      *
      * #fungus_net::null_peer_id \n
      * #fungus_net::all_peer_id
      *
      */
    typedef int peer_id;

    /// auth_status indicates the current status of an authentication.
    enum class auth_status: uint32_t
    {
        in_progress, /**< Authentication is still in progress. */
        success,     /**< Authentication was successful. */
        failure      /**< Authentication failed. */
    };

    /** @} */
    /** @{ */

    constexpr peer_id null_peer_id =  0; /**< null_peer_id is a handle which refers to a nil peer. */
    constexpr peer_id  all_peer_id = -1; /**< all_peer_id is a handle which refers to the special peer group containing all concrete peers for the host. */

    /** @} */
    /** @{ */

    constexpr sec_duration_t default_connection_timeout_period = 5.0; /**< The default timeout period for connection. */
    constexpr sec_duration_t default_disconnect_timeout_period = 5.0; /**< The default timeout period for disconnect. */
    constexpr sec_duration_t default_auth_timeout_period       = 5.0; /**< The default timeout period for authentication. */

    /** @} */
    /** @} */

    /** @defgroup static_queries Static Queries
      * @{
      */

    /** @{ */

    /** Get the maximum number of channels available
      * for a message.  A channel specified for a message
      * should never exceeed get_channel_count() - 1.
      *
      * @returns the number of channels available to each host globally.
      */
    FUNGUSNET_API size_t get_channel_count();

    /** @} */
    /** @} */

    /** @defgroup address_api Address API
      * @{
      */

    /// ipv4 contains an ipv4 address.
    struct FUNGUSNET_API ipv4
    {
        /// Predefined port numbers.
        enum ports: uint16_t
        {
            default_port = 8999 /**< The default port passed as an argument to the ipv4 constructor. */
        };

        /// ipv4::host contains an ipv4 host address.
        union FUNGUSNET_API host
        {
            uint32_t value;   /**< The 32 bit ipv4 host address. */
            uint8_t elems[4]; /**< union access to each byte. */

            /** Construct the address from a hostname by resolving the hostname to a
              * valid ipv4 host address.  If the host does not exist, value is set
              * to 0.
              *
              * @param host_name The hostname to resolve to an ipv4 host address.
              */
            host(const std::string &host_name);

            /** The default constructor.  Constructs the address from a value which defaults to 0.
              *
              * @param value A uint32 that represents the ipv4 host address.
              */
            host(uint32_t value = 0): value(value) {}

            /** Construct the address from an array of 4 bytes.
              *
              * @param elems An array of 4 bytes.
              */
            host(uint8_t *elems):     elems{elems[0],
                                            elems[1],
                                            elems[2],
                                            elems[3]} {}

            /// Resolve the hostname from the ipv4 host address.
            std::string get_host_name() const;
        };

        host     m_host; /**< The ipv4::host address. */
        uint16_t port_i; /**< The port to bind to. */

        /// The default constructor.  port_i and m_host.value default to 0.
        ipv4();

        /// Copy constructor.
        ipv4(const ipv4 &m_ip);

        /// Copy assignment.
        ipv4 &operator =(const ipv4 &m_ip);

        /** Address constructor.
          *
          * @param m_host   the ipv4::host address.
          * @param port_i   the port to bind to.
          */
        ipv4(const host &m_host, uint16_t port_i = default_port);
    };

    /** @} */
}

#endif
