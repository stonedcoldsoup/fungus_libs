#include "fungus_net_defs_internal.h"

namespace fungus_net
{
    ipv4::host::host(const std::string &host_name)
    {
        ENetAddress m_enet_host;
        enet_address_set_host(&m_enet_host, host_name.c_str());

        value = m_enet_host.host;
    }

    std::string ipv4::host::get_host_name() const
    {
        char m_str[128];
        ENetAddress m_enet_host;
        enet_address_get_host(&m_enet_host, m_str, 128);

        return m_str;
    }

    ipv4::ipv4() = default;
    ipv4::ipv4(const ipv4 &m_ip):
        m_host(m_ip.m_host), port_i(m_ip.port_i)
    {}

    ipv4 &ipv4::operator =(const ipv4 &m_ip)
    {
        m_host = m_ip.m_host;
        port_i = m_ip.port_i;

        return *this;
    }

    ipv4::ipv4(const host &m_host, uint16_t port_i):
        m_host(m_host), port_i(port_i)
    {}

    size_t get_channel_count()
    {
        return internal_defs::max_user_channel_count;
    }
}
