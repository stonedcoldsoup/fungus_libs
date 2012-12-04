#ifndef FUNGUSCONCURRENCY_COMM_INTERNAL_H
#define FUNGUSCONCURRENCY_COMM_INTERNAL_H

#include "fungus_concurrency_communication.h"
#include "fungus_concurrency_concurrent_auto_ptr.h"
#include "fungus_concurrency_concurrent_queue.h"
#include <queue>

namespace fungus_concurrency
{
    namespace inlined
    {
        template <typename messageT>
        struct default_discard_functor
        {
            FUNGUSCONCURRENCY_INLINE void operator()(const messageT &m_message) {}
        };

        template <typename messageT>
        struct ptr_discard_functor
        {
            FUNGUSCONCURRENCY_INLINE void operator()(messageT m_message) {delete m_message;}
        };

        template <typename messageT,
                  typename discard_functorT = default_discard_functor<messageT> >
        class comm_tplt
        {
        public:
            typedef messageT             message_type;
            typedef discard_functorT discard_functor_type;

            typedef comm::channel_id channel_id;
            typedef comm::event      event;

            enum {null_channel_id = comm::null_channel_id};
        private:
            struct message_wrap
            {
                enum type {type_close_message, type_user_message};

                virtual ~message_wrap() {}
                virtual type get_type() const = 0;
            };

            typedef typename message_wrap::type message_wrap_type;

            struct user_message_wrap: message_wrap
            {
                messageT m_message;
                discard_functorT discard;
                bool received;

                user_message_wrap(const messageT &m_message, discard_functorT discard):
                    m_message(m_message), discard(discard), received(false) {}

                virtual ~user_message_wrap()
                {
                    if (!received)
                        discard(m_message);
                }

                FUNGUSCONCURRENCY_INLINE void receive(messageT &m_message)
                {
                    m_message = this->m_message;
                    received = true;
                }

                virtual message_wrap_type get_type() const {return message_wrap::type_user_message;}
            };

            struct close_message_wrap: message_wrap
            {
                int data;

                close_message_wrap(int data): data(data) {}

                virtual message_wrap_type get_type() const {return message_wrap::type_close_message;}
            };

            typedef concurrent_queue<message_wrap *>   message_queue;
            typedef concurrent_auto_ptr<message_queue> message_queue_ptr;

            class command
            {
            public:
                struct io
                {
                    struct slot
                    {
                        mutex m;
                        command *cmd;

                        slot(): m(), cmd(nullptr) {}
                        ~slot()
                        {
                            if (cmd)
                                delete cmd;
                        }

                        bool put(command *ncmd)
                        {
                            bool success = false;
                            if (m.try_lock())
                            {
                                success = (cmd == nullptr);
                                if (success)
                                    cmd = ncmd;

                                m.unlock();
                            }

                            return success;
                        }

                        command *get()
                        {
                            command *gcmd = nullptr;
                            if (m.try_lock())
                            {
                                gcmd = cmd;
                                cmd  = nullptr;
                                m.unlock();
                            }

                            return gcmd;
                        }
                    };

                    const size_t cmd_io_nslots;
                    slot *slots;

                    io(const size_t cmd_io_nslots): cmd_io_nslots(cmd_io_nslots)
                    {
                        slots = new slot[cmd_io_nslots];
                    }

                    ~io()
                    {
                        delete[] slots;
                    }

                    void put(command *ncmd)
                    {
                        bool searching = true;
                        for (size_t i = 0; searching; i = (i + 1) % cmd_io_nslots)
                            searching = !slots[i].put(ncmd);

                        //stdlib_bits::locked_cout << stdlib_bits::lock_o << "put command" << stdlib_bits::unlock_o;
                    }

                    bool get(command *&gcmd)
                    {
                        bool success = false;
                        for (size_t i = 0; i < cmd_io_nslots; ++i)
                        {
                            gcmd = slots[i].get();

                            success = (nullptr != gcmd);
                            if (success)
                            {
                                //stdlib_bits::locked_cout << stdlib_bits::lock_o << "got command" << stdlib_bits::unlock_o;
                                break;
                            }
                        }

                        return success;
                    }
                };

                typedef concurrent_auto_ptr<io> io_ptr;

                enum type
                {
                    open_channel,
                    appr_channel,
                    deny_channel
                };

                type          t;
                message_queue_ptr q;
                io_ptr        iop;
                channel_id    id;
                int           data;

                command(const command &c):
                    t(c.t), q(c.q), iop(c.iop), id(c.id), data(c.data) {}

                command(type t, message_queue_ptr q, io_ptr iop, channel_id id, int data):
                    t(t), q(q), iop(iop), id(id), data(data)           {}

                ~command() = default;
            };

            typedef typename command::io     cmd_io;
            typedef typename command::io_ptr cmd_io_ptr;

            class channel
            {
            public:
                discard_functorT discard;
                message_queue_ptr    out_m_message, in_m_message;
                std::queue<messageT> wait_out;

                bool      is_stub, closed, allow_timeout;
                int       closed_data;
                timestamp timeout_start;

            private:
                channel(discard_functorT discard): discard(discard),
                    is_stub(true), closed(false), allow_timeout(true),
                    timeout_start(timestamp::current_time)
                {
                    out_m_message = nullptr;
                    in_m_message  = new message_queue();
                }

                channel(message_queue_ptr out_m_message, discard_functorT discard):
                    discard(discard), out_m_message(out_m_message),
                    is_stub(false), closed(false), allow_timeout(false),
                    timeout_start(timestamp::zero_time)
                {
                    in_m_message  = new message_queue();
                }

                ~channel()
                {
                    if (!closed) close(-1);

                    message_wrap *wrap;
                    while (in_m_message->pop(wrap))
                        delete wrap;
                }

                friend class fungus_util::block_allocator<channel, 32>;
            public:
                FUNGUSCONCURRENCY_INLINE bool is_timed_out(const sec_duration_t &timeout_period) const
                {
                    if (allow_timeout)
                    {
                        sec_duration_t dur = usec_duration_to_sec
                            (timestamp(timestamp::current_time) - timeout_start);

                        if (dur >= timeout_period)
                            return true;
                    }

                    return false;
                }

                FUNGUSCONCURRENCY_INLINE void reset_timeout() const
                {
                    timeout_start = timestamp::current_time;
                }

                FUNGUSCONCURRENCY_INLINE void open(message_queue_ptr out_m_message)
                {
                    this->out_m_message = out_m_message;

                    is_stub       = false;
                    allow_timeout = false;
                    closed        = false;
                    timeout_start = timestamp::zero_time;
                }

                FUNGUSCONCURRENCY_INLINE bool receive(messageT &m_message)
                {
                    if (!is_stub)
                    {
                        message_wrap *wrap;
                        bool success = in_m_message->pop_if_open(wrap);

                        if (success)
                        {
                            switch (wrap->get_type())
                            {
                            case message_wrap::type_close_message:
                            {
                                #ifdef FUNGUS_CONCURRENCY_NO_INLINE
                                    close_message_wrap *close_message = dynamic_cast<close_message_wrap *>(wrap);
                                    fungus_util_assert(close_message,  "comm_tplt::channel::receive() got a message of type type_close_message,\nbut message structure was not of type close_message_wrap!");
                                #else
                                    close_message_wrap *close_message = static_cast<close_message_wrap *>(wrap);
                                #endif

                                is_stub     = true;
                                closed      = true;
                                closed_data = close_message->data;

                                success     = false;
                            } break;
                            case message_wrap::type_user_message:
                            {
                                #ifdef FUNGUS_CONCURRENCY_NO_INLINE
                                    user_message_wrap  *user_message  = dynamic_cast<user_message_wrap  *>(wrap);
                                    fungus_util_assert(user_message,   "comm_tplt::channel::receive() got a message of type type_user_message,\nbut message structure was not of type user_message_wrap!");
                                #else
                                    user_message_wrap  *user_message  = static_cast<user_message_wrap  *>(wrap);
                                #endif

                                user_message->receive(m_message);
                            } break;
                            default:
                                fungus_util_assert(false, "comm_tplt::channel::receive() got a message of unknown type!");
                                break;
                            };

                            delete wrap;
                        }

                        return success;
                    }
                    else
                        return false;
                }

                FUNGUSCONCURRENCY_INLINE void send(const messageT &m_message)
                {
                    wait_out.push(m_message);
                }

                FUNGUSCONCURRENCY_INLINE void close(int data)
                {
                    flush();
                    if (!is_stub)
                    {
                        message_wrap *wrap = new close_message_wrap(data);
                        out_m_message->push(wrap);
                    }

                    is_stub     = true;
                    closed      = true;
                    closed_data = data;
                }

                FUNGUSCONCURRENCY_INLINE void flush()
                {
                    if (is_stub)
                    {
                        while (!wait_out.empty())
                        {
                            discard(wait_out.front());
                            wait_out.pop();
                        }
                    }
                    else
                    {
                        while (!wait_out.empty())
                        {
                            message_wrap *wrap = new user_message_wrap(wait_out.front(), discard);
                            out_m_message->push(wrap);

                            wait_out.pop();
                        }
                    }
                }
            };

            typedef block_allocator_object_hash<channel_id, channel,
                    fungus_util::block_allocator<channel, 32>> channel_hash;

            typedef          fungus_util::hash_map<channel_hash>        channel_map;
            typedef typename fungus_util::hash_map<channel_hash>::entry channel_map_entry;

            fungus_util::block_allocator<channel, 32> m_channel_allocator;

            channel_map            chans;
            std::queue<event>      events;
            std::queue<event>      waiting_events;

            cmd_io_ptr             cmd_io_ap;
            cmd_io                *cmd_io_p;

            std::queue<channel_id> recycled_channel_ids;
            std::queue<channel_id> available_channel_ids;
            channel_id             channel_id_ctr;

            sec_duration_t         timeout_period;
            size_t                 max_channels;

            discard_functorT       discard;

            FUNGUSCONCURRENCY_INLINE void free_channel_id(channel_id id)
            {
                recycled_channel_ids.push(id);
            }

            FUNGUSCONCURRENCY_INLINE void recycle_channel_ids()
            {
                while (!recycled_channel_ids.empty())
                {
                    available_channel_ids.push(recycled_channel_ids.front());
                    recycled_channel_ids.pop();
                }
            }

            FUNGUSCONCURRENCY_INLINE channel_id alloc_channel_id()
            {
                channel_id id;

                if (available_channel_ids.empty())
                    id = ++channel_id_ctr;
                else
                {
                    id = available_channel_ids.front();
                    available_channel_ids.pop();
                }
                return id;
            }

            FUNGUSCONCURRENCY_INLINE void dispatch_handle_open_command(command *cmd)
            {
                int        data               = cmd->data;
                channel_id id                 = cmd->id;
                cmd_io *comm_tplt_cmd_io_p = cmd->iop;

                if (chans.size() < max_channels)
                {
                    channel_id nchan_id = alloc_channel_id();
                    channel *nchan = m_channel_allocator.create(cmd->q, discard);
                    chans.insert(channel_map_entry(nchan_id, nchan));

                    command *ncmd = new command
                    (
                        command::appr_channel,
                        nchan->in_m_message, nullptr,
                        id, data
                    );
                    comm_tplt_cmd_io_p->put(ncmd);

                    events.push(event(event::channel_open, nchan_id, data));
                }
                else
                {
                    command *ncmd = new command
                    (
                        command::deny_channel,
                        nullptr, nullptr,
                        id, 0
                    );
                    comm_tplt_cmd_io_p->put(ncmd);
                }
            }

            FUNGUSCONCURRENCY_INLINE void dispatch_handle_appr_command(command *cmd)
            {
                int        data = cmd->data;
                channel_id id   = cmd->id;

                auto it = chans.find(id);
                if (it != chans.end())
                {
                    channel *chan = it->value;
                    chan->open(cmd->q);
                    events.push(event(event::channel_open, id, data));
                }
            }

            FUNGUSCONCURRENCY_INLINE void dispatch_handle_deny_command(command *cmd)
            {
                int        data = cmd->data;
                channel_id id   = cmd->id;

                auto it = chans.find(id);
                if (it != chans.end())
                {
                    channel *chan = it->value;

                    chan->is_stub = true;
                    chan->closed  = true;

                    events.push(event(event::channel_deny, id, data));
                }
            }

            FUNGUSCONCURRENCY_INLINE void dispatch_handle_command(command *cmd)
            {
                switch (cmd->t)
                {
                case command::open_channel: dispatch_handle_open_command(cmd); break;
                case command::appr_channel: dispatch_handle_appr_command(cmd); break;
                case command::deny_channel: dispatch_handle_deny_command(cmd); break;
                default:
                    fungus_util_assert(false, "comm_tplt::dispatch_handle_command() read unknown command!");
                    break;
                };

                delete cmd;
            }

            FUNGUSCONCURRENCY_INLINE void cleanup()
            {
                // queue to delete dead channels
                std::queue<std::pair<channel_id, channel *>> dead_chans;

                // go through all channels
                for (auto it: chans)
                {
                    // get channel/id
                    channel_id id = it.key;
                    channel *chan = it.value;

                    // if channel is closed, throw a closed event and kill
                    // if channel is timed out, throw a lost event and kill
                    if (chan->closed)
                    {
                        events.push(event(event::channel_clos, id, chan->closed_data));
                        dead_chans.push(std::pair<channel_id, channel *>(id, chan));
                    }
                    else if (chan->is_timed_out(timeout_period))
                    {
                        events.push(event(event::channel_lost, id, 0));
                        dead_chans.push(std::pair<channel_id, channel *>(id, chan));
                    }
                }

                // clean up dead channels
                while (!dead_chans.empty())
                {
                    auto pair = dead_chans.front();

                    chans.erase(pair.first);
                    dead_chans.pop();
                }
            }

            FUNGUSCONCURRENCY_INLINE void flush_all()
            {
                // go through all channels and flush them
                for (auto it: chans)
                    it.value->flush();
            }
        public:
            comm_tplt(size_t max_channels, size_t cmd_io_nslots, sec_duration_t timeout_period, discard_functorT discard = discard_functorT()):
                m_channel_allocator(max_channels / 32 + 1),
                chans(max_channels * 2, channel_hash(m_channel_allocator)),
                events(), waiting_events(),
                recycled_channel_ids(),
                available_channel_ids(),
                channel_id_ctr(0),
                timeout_period(timeout_period),
                max_channels(max_channels),
                discard(discard)
            {
                cmd_io_ap = new cmd_io(cmd_io_nslots);
                cmd_io_p  = cmd_io_ap;
            }

            virtual ~comm_tplt()
            {
                chans.clear();

                while (!waiting_events.empty())        waiting_events.pop();
                while (!events.empty())                events.pop();
                while (!recycled_channel_ids.empty())  recycled_channel_ids.pop();
                while (!available_channel_ids.empty()) available_channel_ids.pop();
            }

            FUNGUSCONCURRENCY_INLINE channel_id open_channel(comm_tplt *mcomm_tplt, int data)
            {
                if (chans.size() >= max_channels)
                    return null_channel_id;

                cmd_io_ptr comm_tplt_cmd_io_ap = mcomm_tplt->cmd_io_ap;
                cmd_io    *comm_tplt_cmd_io_p  = comm_tplt_cmd_io_ap;

                channel_id nchan_id = alloc_channel_id();
                channel *nchan = m_channel_allocator.create(discard);


                chans.insert(channel_map_entry(nchan_id, nchan));

                command *cmd = new command
                (
                    command::open_channel,
                    nchan->in_m_message, cmd_io_ap,
                    nchan_id, data
                );
                comm_tplt_cmd_io_p->put(cmd);

                return nchan_id;
            }

            FUNGUSCONCURRENCY_INLINE bool close_channel(channel_id id, int data)
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                channel *chan = it->value;

                if (!chan)        return false;
                if (chan->closed) return false;

                chan->close(data);
                return true;
            }

            FUNGUSCONCURRENCY_INLINE void close_all_channels(int data)
            {
                for (auto it: chans)
                {
                    channel *chan = it.value;

                    if (!chan->closed)
                        chan->close(data);
                }
            }

            FUNGUSCONCURRENCY_INLINE bool does_channel_exist(channel_id id) const
            {
                return chans.find(id) != chans.end();
            }

            FUNGUSCONCURRENCY_INLINE bool is_channel_open(channel_id id) const
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                const channel *chan = it->value;
                return chan && !chan->is_stub;
            }

            FUNGUSCONCURRENCY_INLINE bool channel_send(channel_id id, const messageT &m_message)
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                channel *chan = it->value;

                if (!chan)        return false;
                if (chan->closed) return false;

                chan->send(m_message);
                return true;
            }

            FUNGUSCONCURRENCY_INLINE bool channel_receive(channel_id id, messageT &m_message)
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                channel *chan = it->value;

                if (chan)
                    return chan->receive(m_message);
                else
                    return false;
            }

            FUNGUSCONCURRENCY_INLINE bool channel_discard(channel_id id, size_t n = 1)
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                channel *chan = it->value;
                bool success = false;

                if (chan)
                {
                    messageT m_message;
                    success = true;

                    for (size_t i = 0; i < n; ++i)
                    {
                        if (chan->receive(m_message))
                            discard(m_message);
                        else
                        {
                            success = false;
                            break;
                        }
                    }
                }

                return success;
            }

            FUNGUSCONCURRENCY_INLINE bool channel_discard_all(channel_id id)
            {
                auto it = chans.find(id);
                if (it == chans.end()) return false;

                channel *chan = it->value;
                if (!chan) return false;

                messageT m_message;
                while (chan->receive(m_message)) discard(m_message);

                return true;
            }

            FUNGUSCONCURRENCY_INLINE void all_channels_discard_all()
            {
                for (auto it: chans)
                {
                    channel *chan = it.value;

                    messageT m_message;
                    while (chan->receive(m_message)) discard(m_message);
                }
            }

            FUNGUSCONCURRENCY_INLINE void dispatch()
            {
                while (!waiting_events.empty())
                {
                    events.push(waiting_events.front());
                    waiting_events.pop();
                }

                command *cmd;
                while (cmd_io_p->get(cmd))
                    dispatch_handle_command(cmd);

                cleanup();
                flush_all();

                this_thread::yield();
            }

            FUNGUSCONCURRENCY_INLINE bool peek_event(event &event) const
            {
                if (!events.empty())
                {
                    event = events.front();
                    return true;
                }
                else
                    return false;
            }

            FUNGUSCONCURRENCY_INLINE bool get_event(event &event)
            {
                bool success = peek_event(event);
                if (success)
                    events.pop();

                return success;
            }

            FUNGUSCONCURRENCY_INLINE void clear_events()
            {
                event e;
                while (get_event(e));
            }
        };
    }

    class comm::impl
    {
    private:
        struct __discard_functor
        {
            comm::discard_message_callback *cb;

            FUNGUSCONCURRENCY_INLINE __discard_functor(): cb(nullptr)                           {}
            FUNGUSCONCURRENCY_INLINE __discard_functor(const __discard_functor &f): cb(f.cb) {}
            FUNGUSCONCURRENCY_INLINE __discard_functor(comm::discard_message_callback *cb): cb(cb)   {}

            FUNGUSCONCURRENCY_INLINE void operator()(const any_type &m_message) {if (cb) cb->discard(m_message);}
        };

        inlined::comm_tplt<any_type, __discard_functor> impl_;
    public:
        FUNGUSCONCURRENCY_INLINE impl(size_t max_channels, size_t cmd_io_nslots, sec_duration_t timeout_period, comm::discard_message_callback *discard):
            impl_(max_channels, cmd_io_nslots, timeout_period, __discard_functor(discard)) {}

        FUNGUSCONCURRENCY_INLINE channel_id open_channel(impl *pimpl_, int data)              {return impl_.open_channel(&(pimpl_->impl_), data);}
        FUNGUSCONCURRENCY_INLINE bool       close_channel(channel_id id, int data)            {return impl_.close_channel(id, data);}
        FUNGUSCONCURRENCY_INLINE void       close_all_channels(int data)                      {       impl_.close_all_channels(data);}
        FUNGUSCONCURRENCY_INLINE bool       does_channel_exist(channel_id id) const           {return impl_.does_channel_exist(id);}
        FUNGUSCONCURRENCY_INLINE bool       is_channel_open(channel_id id) const              {return impl_.is_channel_open(id);}
        FUNGUSCONCURRENCY_INLINE bool       channel_send(channel_id id, const any_type &m_message)  {return impl_.channel_send(id, m_message);}
        FUNGUSCONCURRENCY_INLINE bool       channel_receive(channel_id id, any_type &m_message)     {return impl_.channel_receive(id, m_message);}
        FUNGUSCONCURRENCY_INLINE bool       channel_discard(channel_id id, size_t n = 1)      {return impl_.channel_discard(id, n);}
        FUNGUSCONCURRENCY_INLINE bool       channel_discard_all(channel_id id)                {return impl_.channel_discard_all(id);}
        FUNGUSCONCURRENCY_INLINE void       all_channels_discard_all()                        {return impl_.all_channels_discard_all();}
        FUNGUSCONCURRENCY_INLINE void       dispatch()                                        {       impl_.dispatch();}
        FUNGUSCONCURRENCY_INLINE bool       peek_event(event &event) const                    {return impl_.peek_event(event);}
        FUNGUSCONCURRENCY_INLINE bool       get_event(event &event)                           {return impl_.get_event(event);}
        FUNGUSCONCURRENCY_INLINE void       clear_events()                                    {       impl_.clear_events();}
    };
}

#endif
