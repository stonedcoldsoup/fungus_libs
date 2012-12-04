#ifndef FUNGUSNET_UNITY_BASE_H
#define FUNGUSNET_UNITY_BASE_H

#include "fungus_net_common.h"

#include "fungus_net_message.h"
#include "fungus_net_message_factory_manager.h"

#include "fungus_net_packet.h"
#include "fungus_net_defs_internal.h"

namespace fungus_net
{
    enum class unified_host_type
    {
        networked,
        memory,
        unified
    };

    enum unified_host_flags: uint32_t
    {
        unified_host_flag_networked = 0x1,
        unified_host_flag_memory    = 0x2,
        unified_host_flag_all       = unified_host_flag_networked |
                                      unified_host_flag_memory
    };

    enum reject_reason: uint32_t
    {
        reject_reason_host_timeout = 100,
        reject_reason_host_deny    = 200
    };

    enum disconnect_reason: uint32_t
    {
        disconnect_reason_timeout  = (uint32_t)-10
    };

    class unified_host_base
    {
    public:
        class common_data;

        class policy
        {
        public:
            class factory
            {
            public:
				virtual ~factory() {}
			
                virtual policy *create() const = 0;
            };
			
			virtual ~policy() {}

            virtual bool   grab_peer()           = 0;
            virtual void   drop_peer()           = 0;
            virtual size_t get_max_peers() const = 0;

            inline bool timed_out(timeout_period_type period_type, usec_duration_t duration) const
            {
                return timed_out(period_type, usec_duration_to_sec(duration));
            }

            virtual bool timed_out(timeout_period_type period_type, sec_duration_t duration) const = 0;
        };

        class default_policy: public policy
        {
        protected:
            const size_t max_peers;
            size_t n_peers;

            const sec_duration_t *p_timeout_table;
        public:
            class factory: public policy::factory
            {
            protected:
                const size_t max_peers;
                const sec_duration_t *p_timeout_table;
            public:
                factory(size_t max_peers, const sec_duration_t *p_timeout_table);
                virtual policy *create() const;
            };

            default_policy(size_t max_peers, const sec_duration_t *p_timeout_table);

            virtual bool grab_peer();
            virtual void drop_peer();
            virtual size_t get_max_peers() const;
            virtual bool timed_out(timeout_period_type period_type, sec_duration_t duration) const;
        };

        class common_data
        {
        private:
            policy                 *m_policy;
            size_t                  max_peers;
            endian_converter        endian;

            message_factory_manager m_message_factory_manager;

        public:
            inline common_data(common_data &&m_common_data):
                m_policy(std::move(m_common_data.m_policy)),
                max_peers(m_common_data.max_peers),
                endian(std::move(m_common_data.endian)),
                m_message_factory_manager(std::move(m_common_data.m_message_factory_manager))
            {
                delete m_common_data.m_policy;
                m_common_data.m_policy = nullptr;
            }

            inline common_data(size_t max_peers):
                m_policy(nullptr),
                max_peers(max_peers),
                endian(),
                m_message_factory_manager()
            {
                m_policy = new default_policy(max_peers, timeout_period_map);
            }

            inline common_data(const policy::factory &m_policy_factory):
                m_policy(nullptr),
                endian(),
                m_message_factory_manager()
            {
                set_policy(m_policy_factory);
            }

            inline ~common_data()
            {
                if (m_policy)
                    delete m_policy;
            }

            inline size_t get_max_peers() const
            {
                return max_peers;
            }

            inline void set_policy(const policy::factory &m_policy_factory)
            {
                if (m_policy)
                    delete m_policy;

                m_policy  = m_policy_factory.create();
                max_peers = m_policy->get_max_peers();
            }

            inline       message_factory_manager &get_message_factory_manager()       {return m_message_factory_manager;}
            inline const message_factory_manager &get_message_factory_manager() const {return m_message_factory_manager;}

            inline       endian_converter &get_endian_converter()       {return endian;}
            inline const endian_converter &get_endian_converter() const {return endian;}

            inline       policy &get_policy()       {return *m_policy;}
            inline const policy &get_policy() const {return *m_policy;}
        };

        class peer
        {
        public:
            enum class state: uint8_t
            {
                none = 0,
                connecting,
                disconnecting,
                connected,
                rejected,
                disconnected
            };
        protected:
            unified_host_base *parent;
            state m_state;

            FUNGUSUTIL_ALWAYS_INLINE static inline constexpr bool can_connect(state m_state)
            {
                return m_state == state::none ||
                       m_state == state::rejected ||
                       m_state == state::disconnected;
            }

            FUNGUSUTIL_ALWAYS_INLINE static inline constexpr bool can_disconnect(state m_state)
            {
                return m_state == state::connected ||
                       m_state == state::connecting;
            }

            friend class unified_host_base;
        public:
            peer(unified_host_base *parent);
            virtual ~peer();

            virtual unified_host_type get_host_type() const = 0;

            virtual unified_host_base *get_parent() const;
            virtual state get_state()               const;

            virtual bool send(const message *m_message) = 0;
            virtual message *receive()                  = 0;

            virtual bool connect(const ipv4 &m_ipv4,        uint32_t data);
            virtual bool connect(unified_host_base *m_host, uint32_t data);

            virtual bool disconnect(uint32_t data) = 0;
            virtual bool reset();
        };
    protected:
        common_data &m_common_data;

    public:
        struct event
        {
            enum class type: uint8_t
            {
                none,
                connected,
                disconnected,
                rejected
            };

            type m_type;
            uint32_t data;
            peer *m_peer;

            event(const event &m_event):
                m_type(m_event.m_type), data(m_event.data), m_peer(m_event.m_peer)
            {}

            event(type m_type = type::none, uint32_t data = 0, peer *m_peer = nullptr):
                m_type(m_type), data(data), m_peer(m_peer)
            {}
        };

        unified_host_base(common_data &m_common_data);
        virtual ~unified_host_base();

        virtual       common_data &get_common_data();
        virtual const common_data &get_common_data() const;

        virtual unified_host_type  get_type() const    = 0;

        virtual void reset_all_peers()                 = 0;
        virtual void destroy_all_peers()               = 0;

        virtual size_t count_peers() const             = 0;

        virtual peer *new_peer(unified_host_type type) = 0;
        virtual bool destroy_peer(peer *m_peer)        = 0;

        virtual void dispatch()                        = 0;
        virtual bool next_event(event &m_event)        = 0;
        virtual bool peek_event(event &m_event) const  = 0;
    };

    template <unified_host_type instance_type>
    class unified_host_instance;
}

#endif
