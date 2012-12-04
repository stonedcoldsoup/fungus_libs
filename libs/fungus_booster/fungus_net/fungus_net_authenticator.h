#ifndef FUNGUSNET_AUTHENTICATOR_H
#define FUNGUSNET_AUTHENTICATOR_H

#include "fungus_net_common.h"
#include "fungus_net_defs.h"
#include "fungus_net_message.h"

namespace fungus_net
{
    using namespace fungus_util;

    /** @defgroup auth_api Authentication API
      * @{
      */

    /** \brief An auth_payload is a block of memory containing
      * authentication information and authentication status.
      *
      * An auth_payload has serializer and deserializer objects,
      * which is how the data is accessed.
      *
      * An auth_payload also contains an auth_state, which
      * tells the recipient if the authentication is still in
      * progress, successful, or failed.  If the auth_payload
      * is failed on receipt, then the peer associated with it
      * has already been destroyed.  If the authentication was
      * successful, then the state of the associated peer will
      * change from authenticating to connected.
      */
    class FUNGUSNET_API auth_payload
    {
    public:
        /// The destination of the auth_payload
        enum class destination
        {
            outgoing, /**< Indicates that the auth_payload has not yet been sent. */
            incoming  /**< Indicates that the auth_payload was just received. */
        };
    private:
        const endian_converter &endian;

        serializer_buf m_ibuf;

        serializer     m_os;
        deserializer   m_is;

        destination    m_dest;
        auth_status    m_status;

        auth_payload(const endian_converter &endian,
                     const serializer_buf &m_ibuf,
                     auth_status m_status);

        auth_payload(const endian_converter &endian,
                     auth_status m_status);

        ~auth_payload();

        friend class payload_factory;
        friend class payload_message;
        friend class payload_to_message_converter;
        friend class message_to_payload_converter;
    public:
        auth_payload(auth_payload &&m_payload)                  = delete;
        auth_payload(const auth_payload &m_payload)             = delete;

        auth_payload &operator =(auth_payload &&m_payload)      = delete;
        auth_payload &operator =(const auth_payload &m_payload) = delete;

        /** Get the destination of the auth_payload.
          *
          * @returns the destination of the auth_payload.
          */
        destination get_destination() const;

        /** Get the status of the auth_payload.
          *
          * @returns the status of the auth_payload.
          */
        auth_status get_status()      const;

        /** \brief Get the serializer for the auth_payload.
          *
          * The serializer will only be valid if the auth_payload
          * is outgoing.
          *
          * @returns the serializer for the auth_payload.
          */
        serializer   &get_serializer();

        /** \brief Get the deserializer for the auth_payload.
          *
          * The deserializer will only be valid if the auth_payload
          * is incoming.
          *
          * @returns the deserializer for the auth_payload.
          */
        deserializer &get_deserializer();
    };

    /** @} */
}

#endif
