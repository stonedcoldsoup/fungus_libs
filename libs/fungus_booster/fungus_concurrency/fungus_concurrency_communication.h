#ifndef FUNGUSCONCURRENCY_COMMUNICATION_H
#define FUNGUSCONCURRENCY_COMMUNICATION_H

#include "fungus_concurrency_common.h"

namespace fungus_concurrency
{
    using namespace fungus_util;

    class FUNGUSCONCURRENCY_API comm
    {
    public:
        struct FUNGUSCONCURRENCY_API discard_message_callback
        {
            virtual void discard(const any_type &m_message) = 0;
        };
    private:
        FUNGUSUTIL_NO_ASSIGN(comm);

        class impl;
        impl *pimpl_;

        comm(impl *pimpl_);

        friend class process;
    public:
        comm(size_t max_channels, size_t cmd_io_nslots, sec_duration_t timeout_period, discard_message_callback *m_discard_message = nullptr);
        ~comm();

        typedef int channel_id;
        enum {null_channel_id = -1};

        struct event
        {
            enum type
            {
                none,
                channel_open,
                channel_deny,
                channel_lost,
                channel_clos
            };

            type t;
            channel_id id;
            int data;

            event():               t(none), id(null_channel_id), data(0)      {}
            event(const event &e): t(e.t),  id(e.id),            data(e.data) {}

            event(type t, channel_id id, int data): t(t), id(id), data(data)  {}
        };

        channel_id open_channel(comm *mcomm, int data);
        bool       close_channel(channel_id id, int data);
        void       close_all_channels(int data);
        bool       does_channel_exist(channel_id id) const;
        bool       is_channel_open(channel_id id) const;
        bool       channel_send(channel_id id, const any_type &m_message);
        bool       channel_receive(channel_id id, any_type &m_message);
        bool       channel_discard(channel_id id, size_t n = 1);
        bool       channel_discard_all(channel_id id);
        void       all_channels_discard_all();
        void       dispatch();
        bool       peek_event(event &event) const;
        bool       get_event(event &event);
        void       clear_events();
    };
}

#endif
