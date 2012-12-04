#ifndef FUNGUSNET_AUTHENTICATOR_INTERNAL_H
#define FUNGUSNET_AUTHENTICATOR_INTERNAL_H

#include "fungus_net_authenticator.h"
#include "fungus_net_defs_internal.h"

namespace fungus_net
{
    using namespace fungus_util;

    class payload_factory
    {
    private:
        const endian_converter &endian;
    public:
        payload_factory(const endian_converter &endian);

        auth_payload *create(auth_status m_status)     const;
        void          destroy(auth_payload *m_payload) const;
    };

    class payload_message: public protocol_message
    {
    public:
        class factory: public message::factory
        {
            virtual message *create()        const;
            virtual message::factory *move() const;
            virtual message_type get_type()  const;
        };

        serializer_buf m_buf;
        auth_status    m_status;

        payload_message();
        ~payload_message();

        payload_message(serializer_buf &&m_buf, auth_status m_status);
        payload_message(const serializer_buf &m_buf, auth_status m_status);

        virtual message_type get_type() const;
        virtual message *copy()         const;

        virtual void serialize_data(serializer &s) const;
        virtual void deserialize_data(deserializer &s);
    };

    class payload_to_message_converter
    {
    private:
        auth_payload *m_payload;
    public:
        payload_to_message_converter();
        ~payload_to_message_converter();

        payload_to_message_converter &operator()(auth_payload *m_payload);

        operator message *();
    };

    class message_to_payload_converter
    {
    private:
        const endian_converter &endian;
        payload_message *m_message;
    public:
        message_to_payload_converter(const endian_converter &endian);
        ~message_to_payload_converter();

        message_to_payload_converter &operator()(message *m_message);

        operator auth_payload *();
    };
}

#endif
