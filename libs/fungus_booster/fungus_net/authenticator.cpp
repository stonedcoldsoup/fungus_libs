#include "fungus_net_authenticator_internal.h"

namespace fungus_net
{
    using namespace fungus_util;

    payload_factory::payload_factory(const endian_converter &endian):
        endian(endian)
    {}

    auth_payload *payload_factory::create(auth_status m_status) const
    {
        auth_payload *m_payload =
            new auth_payload(endian, m_status);

        return m_payload;
    }

    void payload_factory::destroy(auth_payload *m_payload) const
    {
        delete m_payload;
    }

    message *payload_message::factory::create() const
    {
        payload_message *m_message = new payload_message();
        return m_message;
    }

    message::factory *payload_message::factory::move() const
    {
        message::factory *m_factory = new factory();
        return m_factory;
    }

    message_type payload_message::factory::get_type() const
    {
        return internal_defs::payload_message_type;
    }

    payload_message::payload_message():
        m_buf(), m_status(auth_status::in_progress)
    {}

    payload_message::~payload_message() = default;

    payload_message::payload_message(serializer_buf &&m_buf, auth_status m_status):
        m_buf(std::move(m_buf)), m_status(m_status)
    {}

    payload_message::payload_message(const serializer_buf &m_buf,
                                     auth_status m_status):
        m_buf(m_buf), m_status(m_status)
    {}

    message_type payload_message::get_type() const
    {
        return internal_defs::payload_message_type;
    }

    message *payload_message::copy() const
    {
        payload_message *m_message = new payload_message(m_buf, m_status);
        return m_message;
    }

    void payload_message::serialize_data(serializer &s) const
    {
        s << m_status << (size_t)m_buf.size << m_buf;
    }

    void payload_message::deserialize_data(deserializer &s)
    {
        size_t buf_size = 0;
        s >> m_status >> buf_size;

        m_buf.set(buf_size);
        s >> m_buf;
    }

    auth_payload::auth_payload(const endian_converter &endian,
                               const serializer_buf &m_ibuf,
                               auth_status m_status):
        endian(endian), m_ibuf(m_ibuf),
        m_os(endian), m_is(endian, this->m_ibuf.buf, this->m_ibuf.size),
        m_dest(destination::incoming),
        m_status(m_status)
    {}

    auth_payload::auth_payload(const endian_converter &endian,
                               auth_status m_status):
        endian(endian), m_ibuf(),
        m_os(endian), m_is(endian, m_ibuf.buf, m_ibuf.size),
        m_dest(destination::outgoing),
        m_status(m_status)
    {}

    auth_payload::~auth_payload() = default;

    auth_payload::destination auth_payload::get_destination() const
    {
        return m_dest;
    }

    auth_status auth_payload::get_status() const
    {
        return m_status;
    }

    serializer   &auth_payload::get_serializer()   {return m_os;}
    deserializer &auth_payload::get_deserializer() {return m_is;}

    payload_to_message_converter::payload_to_message_converter():
        m_payload(nullptr)
    {}

    payload_to_message_converter::~payload_to_message_converter()
    {
        if (m_payload)
            delete m_payload;
    }

    payload_to_message_converter &payload_to_message_converter::operator()(auth_payload *m_payload)
    {
        if (this->m_payload)
            delete this->m_payload;

        this->m_payload = m_payload;
        return *this;
    }

    payload_to_message_converter::operator message *()
    {
        message *m_message = nullptr;

        if (m_payload)
        {
            serializer_buf m_buf;
            m_payload->m_os.get_buf(m_buf);

            m_message = new payload_message
            (
                std::move(m_buf),
                m_payload->m_status
            );

            delete m_payload;
            m_payload = nullptr;
        }

        return m_message;
    }

    message_to_payload_converter::message_to_payload_converter(const endian_converter &endian):
        endian(endian), m_message(nullptr)
    {}

    message_to_payload_converter::~message_to_payload_converter()
    {
        if (m_message)
            delete m_message;
    }

    message_to_payload_converter &message_to_payload_converter::operator()(message *m_message)
    {
        if (this->m_message)
            delete this->m_message;

        if (m_message && m_message->get_type() == internal_defs::payload_message_type)
            this->m_message = dynamic_cast<payload_message *>(m_message);
        else
            this->m_message = nullptr;

        return *this;
    }

    message_to_payload_converter::operator auth_payload *()
    {
        auth_payload *m_payload = nullptr;

        if (m_message)
        {
            m_payload = new auth_payload(endian, std::move(m_message->m_buf), m_message->m_status);

            delete m_message;
            m_message = nullptr;
        }

        return m_payload;
    }
}
