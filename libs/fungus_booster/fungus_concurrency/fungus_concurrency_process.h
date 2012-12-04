#ifndef FUNGUSCONCURRENCY_PROCESS_H
#define FUNGUSCONCURRENCY_PROCESS_H

#include "fungus_concurrency_common.h"
#include "fungus_concurrency_communication.h"
#include "fungus_concurrency_concurrent_auto_ptr.h"
#include <set>

namespace fungus_concurrency
{
    using namespace fungus_util;

    class FUNGUSCONCURRENCY_API process
    {
    public:
        typedef void (*main_function)(process *mproc, const any_type &data, comm::channel_id parent_chan);

        struct spawn_result
        {
            concurrent_auto_ptr<process> proc;
            thread::id                   t_id;
            comm::channel_id             c_id;

            spawn_result():
                proc(), t_id(), c_id(comm::null_channel_id)
            {}

            spawn_result(spawn_result &&m_spawn_result):
                proc(std::move(m_spawn_result.proc)),
                t_id(m_spawn_result.t_id),
                c_id(m_spawn_result.c_id)
            {}

            spawn_result(const spawn_result &m_spawn_result):
                proc(m_spawn_result.proc),
                t_id(m_spawn_result.t_id),
                c_id(m_spawn_result.c_id)
            {}

            spawn_result &operator =(spawn_result &&m_spawn_result)
            {
                proc = std::move(m_spawn_result.proc);
                t_id = m_spawn_result.t_id;
                c_id = m_spawn_result.c_id;

                return *this;
            }

            spawn_result &operator =(const spawn_result &m_spawn_result)
            {
                proc = m_spawn_result.proc;
                t_id = m_spawn_result.t_id;
                c_id = m_spawn_result.c_id;

                return *this;
            }
        };

        enum
        {
            spawn_no_flags          = 0x0,
            spawn_flag_non_blocking = 0x1,  // using this flag will open the channel but not wait for the connection to
                                            // complete.  it will be your responsibility to make sure the channel is open
                                            // before sending any data.

            spawn_flag_no_channel   = 0x2   // do not create a channel to the new process.
        };
    private:
        enum run_mode_e
        {
            run_mode_spawn,
            run_mode_user
        };

        typedef concurrent_auto_ptr<comm> comm_ptr;

        mutable mutex m;
        bool b_is_running;

        const thread::id t_id;
        const run_mode_e run_mode;

        const sec_duration_t comm_timeout_period;

        comm::impl *comm_impl_p;
        comm::channel_id parent_chan;

        std::set<thread *> peers;

        comm_ptr comm_ap;
        comm    *comm_p;

        main_function main_fn;

        process(main_function main_fn,
                run_mode_e run_mode,
                size_t comm_max_channels,
                size_t comm_cmd_io_nslots,
                sec_duration_t comm_timeout_period,
                comm::discard_message_callback *m_discard_message);
    public:
        process(main_function main_fn, size_t comm_max_channels,
                size_t comm_cmd_io_nslots,
                sec_duration_t comm_timeout_period,
                comm::discard_message_callback *m_discard_message = nullptr);

        ~process();

        void kill();

              comm *get_comm();
        const comm *get_comm() const;

        thread::id get_thread_id() const;
        bool       is_running()    const;

        spawn_result spawn(main_function main_fn,
                           const any_type &proc_data, int comm_data,
                           size_t comm_max_channels,
                           size_t comm_cmd_io_nslots,
                           sec_duration_t comm_timeout_period,
                           short flags = spawn_no_flags,
                           comm::discard_message_callback *m_discard_message = nullptr);

        void run(const any_type &proc_data);

        void cleanup();
    };
}

#endif
