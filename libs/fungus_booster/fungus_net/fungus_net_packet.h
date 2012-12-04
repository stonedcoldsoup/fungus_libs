#ifndef FUNGUSNET_PACKET_H
#define FUNGUSNET_PACKET_H

#include "fungus_net_common.h"

#ifdef BUILD_FUNGUSNET
    #ifdef FUNGUSUTIL_WIN32
        #include "enet\enet.h"
    #else
        #include <enet/enet.h>
    #endif
#else
    struct ENetPacket;
    struct ENetPeer;
#endif

#include "fungus_net_message.h"
#include "fungus_net_message_factory_manager.h"

#include <queue>

namespace fungus_net
{
    using namespace fungus_util;

    class packet
    {
    public:
        enum class stream_mode
        {
            invalid,
            sequenced,
            unsequenced
        };

        enum class destination
        {
            incoming,
            outgoing
        };
    private:
        stream_mode smode;
        uint8_t channel;

        destination dest;
        bool initialized;

        deserializer *ds;
        serializer *s;
        serializer_buf buf;

        friend class block_allocator<packet, 1024>;

        packet();
        ~packet();
    public:
        bool initialize_outgoing(const message *m_message, const endian_converter &endian);

        bool initialize_incoming(serializer_buf &buf,
                                 stream_mode smode, uint8_t channel,
                                 const endian_converter &endian);

        bool        switch_destination(const endian_converter &endian);
        destination get_destination() const;
        bool        is_initialized() const;

        stream_mode get_stream_mode() const;
        uint8_t     get_channel() const;

        message    *make_message(message_factory_manager *factory_manager);

        static void set_guard_word(int word);
    private:
        static uint16_t guard_word;

    public:
        typedef std::pair<packet *, ENetPeer *> packet_ref;

        class aggregator;
        class separator;
    };

    template <packet::stream_mode __smode>
    struct __enet_flags;

    template <>
    struct __enet_flags<packet::stream_mode::sequenced>
    {
        static inline constexpr uint32_t get_flags();
    };

    template <>
    struct __enet_flags<packet::stream_mode::unsequenced>
    {
        static inline constexpr uint32_t get_flags();
    };

    class packet::aggregator
    {
    private:
        block_allocator<packet, 1024> m_allocator;
        const endian_converter &endian;
        std::queue<packet_ref> packets;

        class aggregate_serializer_base
        {
        protected:
            bool used;
            serializer s;
        public:
            aggregate_serializer_base(const endian_converter &endian);
			virtual ~aggregate_serializer_base() {}

            virtual serializer &get();
            virtual void send(ENetPeer *peer, uint8_t channel) = 0;
        };

        template <stream_mode smode>
        class aggregate_serializer: public aggregate_serializer_base
        {
        public:
            aggregate_serializer(const endian_converter &endian);

            virtual void send(ENetPeer *peer, uint8_t channel);
        };

        class aggregate
        {
        private:
            const endian_converter   &endian;
            aggregate_serializer_base   *seq;
            aggregate_serializer_base *unseq;

        public:
            ENetPeer   *peer;
            uint8_t     channel;

            aggregate(std::pair<ENetPeer *, uint8_t> &pair, const endian_converter &endian);

            aggregate(const aggregate &agg);
            aggregate(aggregate &&agg);

            ~aggregate();

            aggregate &operator =(const aggregate &agg);
            aggregate &operator =(aggregate &&agg);

            serializer &get_serializer(stream_mode smode);
            void send();
        };

        class aggregate_map
        {
        private:
            const endian_converter &endian;

        public:
            typedef hash_map<default_hash<uint8_t,    aggregate>, UINT8_MAX * 2> map_agg_type;
            typedef hash_map<default_hash<ENetPeer *, map_agg_type>>             map_peer_type;

            map_peer_type aggs;

            aggregate_map(const endian_converter &endian);
            ~aggregate_map();

            aggregate &get_aggregate(std::pair<ENetPeer *, uint8_t> &&pair);

            void send_all();
            void clear();
        };

    public:
        aggregator(const endian_converter &endian);

        packet *create_packet();
        bool destroy_packet(packet *pk);

        void queue_packet(packet *pk, ENetPeer *peer);

        void send_all();
        void clear();
    };

    class packet::separator
    {
    private:
        block_allocator<packet, 1024> m_allocator;
        const endian_converter &endian;

        std::queue<packet *> packets;

        bool create_packet(serializer_buf &buf, stream_mode smode, uint8_t channel);
    public:
        separator(const endian_converter &endian);
        ~separator();

        bool destroy_packet(packet *pk);

        bool separate_packets(ENetPacket *source, uint8_t channel);

        bool packets_waiting() const;
        packet *get_packet();
    };
};

#endif
