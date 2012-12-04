#ifndef FUNGUSNET_UNITY_H
#define FUNGUSNET_UNITY_H

#include "fungus_net_unity_enet.h"
#include "fungus_net_unity_memory.h"

namespace fungus_net
{
    using namespace fungus_util;

    class unified_host: public unified_host_base
    {
    protected:
        class host_storage
        {
        public:
            class enumerator
            {
            public:
                virtual void operator()(unified_host_base *m_host) = 0;
            };

        private:
            common_data &m_common_data;

            optional<unified_host_instance<unified_host_type::networked>> m_networked_host;
            optional<unified_host_instance<unified_host_type::memory>>    m_memory_host;

            friend unified_host_instance<unified_host_type::memory> *__unified_memory_host(unified_host_base *m_base);
        public:
            host_storage(common_data &m_common_data,
                         uint32_t flags, const ipv4 &m_ipv4,
                         uint32_t in_bandwidth, uint32_t out_bandwidth);

            FUNGUSUTIL_ALWAYS_INLINE
            inline unified_host_base *get_host(unified_host_type type)
            {
                switch (type)
                {
                case unified_host_type::networked: return m_networked_host; break;
                case unified_host_type::memory:    return m_memory_host;    break;
                default:
                    break;
                };

                return nullptr;
            }

            FUNGUSUTIL_ALWAYS_INLINE
            inline void enumerate(enumerator &m_enumerator, uint32_t flags)
            {
                if (flags & unified_host_flag_networked)
                    m_enumerator(m_networked_host);

                if (flags & unified_host_flag_memory)
                    m_enumerator(m_memory_host);
            }
        };

        class call_reset_all_peers: public host_storage::enumerator
        {
        public:
            virtual void operator()(unified_host_base *m_host) {m_host->reset_all_peers();}
        };

        class call_destroy_all_peers: public host_storage::enumerator
        {
        public:
            virtual void operator()(unified_host_base *m_host) {m_host->destroy_all_peers();}
        };

        class call_count_peers: public host_storage::enumerator
        {
        public:
            size_t result;

            call_count_peers(): result(0) {}

            virtual void operator()(unified_host_base *m_host)
            {
                result += m_host->count_peers();
            }
        };

        class call_dispatch: public host_storage::enumerator
        {
        public:
            virtual void operator()(unified_host_base *m_host) {m_host->dispatch();}
        };

        class call_next_event: public host_storage::enumerator
        {
        public:
            bool b_got_event;
            event &m_event;

            call_next_event(event &m_event): b_got_event(false), m_event(m_event) {}

            virtual void operator()(unified_host_base *m_host)
            {
                if (!b_got_event)
                    b_got_event = m_host->next_event(m_event);
            }
        };

        class call_peek_event: public host_storage::enumerator
        {
        public:
            bool b_got_event;
            event &m_event;

            call_peek_event(event &m_event): b_got_event(false), m_event(m_event) {}

            virtual void operator()(unified_host_base *m_host)
            {
                if (!b_got_event)
                    b_got_event = m_host->peek_event(m_event);
            }
        };

        friend unified_host_instance<unified_host_type::memory> *__unified_memory_host(unified_host_base *m_base);

                uint32_t     flags;
        mutable host_storage m_host_storage;
    public:
        unified_host(common_data &m_common_data,
                     uint32_t flags, const ipv4 &m_ipv4,
                     uint32_t in_bandwidth = 0, uint32_t out_bandwidth = 0);

        unified_host(common_data &m_common_data);

        virtual ~unified_host();

        virtual unified_host_type get_type() const
        {
            return unified_host_type::unified;
        }

        virtual void reset_all_peers();
        virtual void destroy_all_peers();

        virtual size_t count_peers() const;

        virtual peer *new_peer(unified_host_type type);
        virtual bool destroy_peer(peer *m_peer);

        virtual void dispatch();
        virtual bool next_event(event &m_event);
        virtual bool peek_event(event &m_event) const;
    };
}

#endif
