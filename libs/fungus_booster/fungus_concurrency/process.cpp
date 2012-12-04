#include "fungus_concurrency_process.h"
#include "fungus_concurrency_comm_internal.h"

namespace fungus_concurrency
{
    struct __proc_data
    {
        concurrent_auto_ptr<process> mproc;
        any_type                     data;

        __proc_data(concurrent_auto_ptr<process> mproc, const any_type &data):
            mproc(mproc), data(data) {}
    };

    void proc_main(void *data)
    {
        __proc_data *mdata = (__proc_data *)data;

        concurrent_auto_ptr<process> mproc = mdata->mproc;
        any_type                     pdata = mdata->data;

        delete mdata;
        mproc->run(pdata);
        mproc->kill();
    }

    static void __dummy_main(process *m_proc, const any_type &data, comm::channel_id parent_chan) {}

    process::process(main_function main_fn,
                     run_mode_e run_mode,
                     size_t comm_max_channels,
                     size_t comm_cmd_io_nslots,
                     sec_duration_t comm_timeout_period,
                     comm::discard_message_callback *m_discard_message):
        m(), b_is_running(false),
        t_id(this_thread::id()), run_mode(run_mode),
        comm_timeout_period(comm_timeout_period), parent_chan(comm::null_channel_id),
        main_fn(main_fn != nullptr ? main_fn : &__dummy_main)
    {
        comm_impl_p = new comm::impl(comm_max_channels, comm_cmd_io_nslots, comm_timeout_period, m_discard_message);

        comm_ap = new comm(comm_impl_p);
        comm_p  = comm_ap;
    }

    process::process(main_function main_fn, size_t comm_max_channels,
                     size_t comm_cmd_io_nslots,
                     sec_duration_t comm_timeout_period,
                     comm::discard_message_callback *m_discard_message):
        m(), b_is_running(false),
        t_id(this_thread::id()), run_mode(run_mode_user),
        comm_timeout_period(comm_timeout_period), parent_chan(comm::null_channel_id),
        main_fn(main_fn != nullptr ? main_fn : &__dummy_main)
    {
        comm_impl_p = new comm::impl(comm_max_channels, comm_cmd_io_nslots, comm_timeout_period, m_discard_message);

        comm_ap = new comm(comm_impl_p);
        comm_p  = comm_ap;
    }

    process::~process()
    {
        kill();
    }

    void process::kill()
    {
        comm_impl_p->dispatch();
        comm_impl_p->close_all_channels(0);

        for (auto it: peers) it->join();

        cleanup();
    }

          comm *process::get_comm()            {return comm_p;}
    const comm *process::get_comm() const      {return comm_p;}

    thread::id  process::get_thread_id() const {return t_id;}
    bool        process::is_running()    const {return b_is_running;}

    process::spawn_result process::spawn(main_function main_fn,
                                         const any_type &proc_data,
                                         int comm_data, size_t comm_max_channels,
                                         size_t comm_cmd_io_nslots,
                                         sec_duration_t comm_timeout_period,
                                         short flags,
                                         comm::discard_message_callback *m_discard_message)
    {
        bool no_channel   = flags & spawn_flag_no_channel;
        bool non_blocking = flags & spawn_flag_non_blocking;

        concurrent_auto_ptr<process> mproc =
            new process(main_fn, (no_channel || non_blocking) ? run_mode_user : run_mode_spawn,
                        comm_max_channels, comm_cmd_io_nslots, comm_timeout_period, m_discard_message);

        thread *th = nullptr;
        comm::channel_id chan_id = comm::null_channel_id;

        if (no_channel)
        {
            __proc_data *data = new __proc_data(mproc, proc_data);
            th = new thread(&proc_main, data);
        }
        else
        {
            process *mproc_p = mproc;
            chan_id = comm_impl_p->open_channel(mproc_p->comm_impl_p, comm_data);

            __proc_data *data = new __proc_data(mproc, proc_data);
            th = new thread(&proc_main, data);

            if (!non_blocking)
            {
                for (bool mthis_wait = true; mthis_wait;)
                {
                    comm_impl_p->dispatch();

                    if (comm_impl_p->does_channel_exist(chan_id))
                    {
                        if (comm_impl_p->is_channel_open(chan_id))
                            mthis_wait = false;
                    }
                    else
                    {
                        mthis_wait = false;
                        chan_id    = comm::null_channel_id;
                    }
                }
            }
        }

        peers.insert(th);

        spawn_result result;

        result.proc = std::move(mproc);
        result.t_id = th->get_id();
        result.c_id = chan_id;

        return result;
    }

    void process::run(const any_type &data)
    {
        if (run_mode == run_mode_spawn)
        {
            for (bool mthis_wait = true; mthis_wait;)
            {
                comm_impl_p->dispatch();

                comm::event event;
                while (comm_impl_p->get_event(event) && mthis_wait)
                {
                    if (event.t == comm::event::channel_open)
                    {
                        mthis_wait = false;
                        parent_chan = event.id;
                    }
                }
            }
        }

        m.lock();
        b_is_running = true;
        m.unlock();

        main_fn(this, data, parent_chan);

        m.lock();
        b_is_running = false;
        m.unlock();
    }

    void process::cleanup()
    {
        std::queue<thread *> joined_threads;
        for (auto it: peers)
        {
            if (!it->joinable())
                joined_threads.push(it);
        }

        while (!joined_threads.empty())
        {
            thread *th = joined_threads.front();
            peers.erase(th);
            delete th;

            joined_threads.pop();
        }
    }
}
