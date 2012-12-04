#include "fungus_booster/fungus_booster.h"

#include <set>
#include <iostream>
#include <sstream>
#include <cstdio>

using namespace fungus_util;
using namespace fungus_concurrency;

static constexpr size_t NBYTES = 1000000;
static constexpr size_t NROWS  = 1000;

static size_t n_slaves         = 1;
static size_t n_rows_per_slave = (size_t)NROWS;

static const int quit_message = 0;

void report_mproc_main(process *mproc, const any_type &data, comm::channel_id parent_chan)
{
    std::set<comm::channel_id> channels;
    channels.insert(parent_chan);

    comm *mcomm = mproc->get_comm();

    bool quit = false;
    for (;;)
    {
        for (auto it: channels)
        {
            any_type m_message;
            while (mcomm->channel_receive(it, m_message))
            {
                if (m_message == quit_message)
                {
                    quit = true;
                    break;
                }
                else
                    std::cout << it << ": " << m_message;
            }

            if (quit) break;
        }

        if (quit) break;

        comm::event event;
        while (mcomm->get_event(event))
        {
            switch (event.t)
            {
            case comm::event::channel_open:
                channels.insert(event.id);
                break;
            case comm::event::channel_clos:
            case comm::event::channel_lost:
                channels.erase(event.id);
                break;
            default:
                break;
            };
        };

        mcomm->dispatch();
        this_thread::yield();
    }
}

void slave_mproc_main(process *mproc, const any_type &data, comm::channel_id parent_chan)
{
    concurrent_auto_ptr<process> report_proc = any_cast<concurrent_auto_ptr<process> >(data);

    comm *mreportcomm = report_proc->get_comm();
    comm *mcomm = mproc->get_comm();

    comm::channel_id report_chan = mcomm->open_channel(mreportcomm, 1);

    bool wait_report_chan = true;
    while (parent_chan == comm::null_channel_id || wait_report_chan)
    {
        comm::event event;

        while (mcomm->get_event(event))
        {
            switch (event.t)
            {
            case comm::event::channel_open:
                switch (event.data)
                {
                case 0:
                    parent_chan = event.id;
                    break;
                case 1:
                    wait_report_chan = false;
                    break;
                };
            default:
                break;
            };
        }

        mcomm->dispatch();
    }

    std::string *s_p = nullptr;

    for (;;)
    {
        any_type m_message;
        if (mcomm->channel_receive(parent_chan, m_message))
        {
            if (m_message == quit_message)
                break;

            s_p = any_cast<std::string *>(m_message);
            std::string &s = *s_p;

            for (size_t i = 0; i < s.size(); ++i)
                s[i] = s[i] + (char)1;
        }
        this_thread::yield();
    }

    std::stringstream ss;
    ss << "Slave done.\n";
    mcomm->channel_send(report_chan, ss.str());
}

void master_mproc_main(process *mproc, const any_type &data, comm::channel_id parent_chan)
{
    // spawn a process to report messages.  this is faster than interlocked usage of cout.
    process::spawn_result report_spawn_result = mproc->spawn(report_mproc_main, 0, 0, n_slaves + 1, n_slaves * 3 + 2, 2.0);
    comm::channel_id report_chan = report_spawn_result.c_id;

    // get our comm object.
    comm *mcomm = mproc->get_comm();
    mcomm->clear_events();

    // set up our table
    size_t n_finished = 0, n_sent = 0, next_row = 0;
    std::string rows[NROWS];

    std::string start_row, finish_row;
    start_row.assign(NBYTES, 'c');
    finish_row.assign(NBYTES, 'd');

    for (size_t i = 0; i < NROWS; ++i)
        rows[i] = start_row;

    std::stringstream s0;
    s0 << "Master process starting, table is:\n  c...x" << (size_t)NBYTES << " x" << (size_t)NROWS << "\n";
    mcomm->channel_send(report_chan, s0.str());

    // record the starting time for this trial
    timestamp start = timestamp::current_time;

    // spawn our slave processes.
    for (size_t i = 0; i < n_slaves; ++i)
    {
        process::spawn_result result = mproc->spawn(slave_mproc_main, report_spawn_result.proc,
                                                    0, 2, n_slaves * 3 + 2, 2.0, process::spawn_flag_non_blocking);
    }

    mcomm->channel_send(report_chan, fungus_util::make_string("Started all ", n_slaves, " slave processes.\n"));

    // dispatch loop
    while (n_finished < n_slaves)
    {
        comm::event event;

        // discard all messages incoming.
        // you must either read or discard all
        // messages in order to receive a waiting
        // close event.  this is because
        // the channel will not mark as closed
        // until all waiting messages are read.
        //
        // this behavior is subject to change,
        // because it is somewhat cumbersome,
        // and could also result in messages
        // being sent to a process that has
        // long since died.
        //
        // the correct way to use this API for
        // the moment is to check or ignore
        // ALL incoming messages before doing
        // anything else.
        //
        // we won't receive any messages in this
        // simplistic example, only events, so
        // we can call this to skip the checking
        // messages stage.
        mcomm->all_channels_discard_all();
        while (mcomm->get_event(event))
        {
            switch (event.t)
            {
            case comm::event::none:
                fungus_util_assert(false, "A none event was received!");
                break;
            case comm::event::channel_open:
                // if one of our channels is now open, send
                // the rows for the slave process to work on.
                for (size_t i = 0; i < n_rows_per_slave; ++i)
                    mcomm->channel_send(event.id, rows + next_row++);

                // send it a quit message last, to tell it when
                // the work is done and it should stoo
                mcomm->channel_send(event.id, quit_message);

                if (++n_sent == n_slaves)
                    mcomm->channel_send(report_chan, fungus_util::make_string("All ", n_slaves, " slave processes supplied.\n"));
                break;
            case comm::event::channel_lost:
                fungus_util_assert(false, fungus_util::make_string("A channel to a new process timed out (channel_id=", event.id, ")!"));
                break;
            case comm::event::channel_clos:
                // increment n_finished.
                if (++n_finished == n_slaves)
                    mcomm->channel_send(report_chan, fungus_util::make_string("All ", n_slaves, " slave processes finished.\n"));
                break;
            case comm::event::channel_deny:
                // increment n_finished, one of our slaves denied openning a channel.
                mcomm->channel_send(report_chan, fungus_util::make_string("Slave process denied channel (channel_id=", event.id, ")!\n"));
                if (++n_finished == n_slaves)
                    mcomm->channel_send(report_chan, fungus_util::make_string("All ", n_slaves, " slave processes finished.\n"));
                break;
            default:
                fungus_util_assert(false, "An event that was unexpected was received!");
                break;
            };
        }

        // dispatch.
        mcomm->dispatch();
        this_thread::yield();
    }

    // record the time.
    timestamp finish = timestamp::current_time;

    // get the pointer to the trial duration from
    // user data.
    sec_duration_t *dur_p = any_cast<sec_duration_t *>(data);
    *dur_p = usec_duration_to_sec(finish - start);

    // n_bad_rows should always be 0.
    size_t n_bad_rows = 0;

    for (size_t i = 0; i < NROWS; ++i)
    {
        if (rows[i] != finish_row) ++n_bad_rows;
    }

    // report the ending condition.
    std::stringstream s1;
    s1 << "Master process complete, table is:\n"
       << "  d...x" << (size_t)NBYTES << " x" << (NROWS - n_bad_rows) << '\n'
       << "  with " << n_bad_rows << " rows incorrect.\n";

    // send our report channel a final string
    // and our quit message.
    mcomm->channel_send(report_chan, s1.str());
    mcomm->channel_send(report_chan, quit_message);

    // wait for the report channel to close.
    while (mcomm->is_channel_open(report_chan))
    {
        mcomm->all_channels_discard_all();
        mcomm->dispatch();
        this_thread::yield();
    }
}

sec_duration_t average  = 0.0;
static size_t  n_trials = 3;

int run_test_args()
{
    n_rows_per_slave = NROWS / n_slaves;
    fungus_util_assert((size_t)NROWS % n_rows_per_slave == 0, "Number of slaves does not go evenly in to number of rows!\n");

    std::cout << "Number of trials:          " << n_trials         << '\n'
              << "Number of slave processes: " << n_slaves         << '\n'
              << "Number of bytes per row:   " << (size_t)NBYTES   << '\n'
              << "Number of rows:            " << (size_t)NROWS    << '\n'
              << "Number of rows per slave:  " << n_rows_per_slave << "\n\n";

    sec_duration_t duration;
    process master(master_mproc_main, n_slaves + 1, n_slaves * 3 + 2, 2.0);

    for (size_t i = 0; i < n_trials; ++i)
    {
        master.run(&duration);

        std::cout << "\nTrial " << i << " complete, time was "
                  << duration << " seconds.\n\n";

        average += duration;
        master.kill();
    }

    average /= (sec_duration_t)n_trials;

    std::cout << "\n" << n_trials
              << " trials complete, average time was "
              << average << " seconds.\n";

    return 0;
}

static constexpr size_t NTESTS = 4;

int run_test()
{
    sec_duration_t averages [NTESTS];
    size_t         slave_cts[NTESTS];
    size_t         row_cts  [NTESTS];

    int result = 0;
    for (size_t i = 0; i < NTESTS; ++i)
    {
        result += run_test_args();

        averages[i]  = average;
        slave_cts[i] = n_slaves;
        row_cts[i]   = n_rows_per_slave;

        n_slaves         *= 2;
        n_rows_per_slave /= 2;

        average = 0.0;
    }

    std::cout << NTESTS << " tests performed:\n";
    for (size_t i = 0; i < NTESTS; ++i)
    {
        std::cout << "Test " << i << ": Average Time was " << averages[i] << " seconds,\n"
                  << "        with " << slave_cts[i] << " slave processes and " << row_cts[i] << " rows per slave.\n";
    }

    return result;
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
        return run_test();
    else
    {
        fungus_util_assert(argc > 1 && sscanf(argv[1], "%u", &n_slaves) == 1, "Could not read first argument: n_slaves!");
        std::cout << "n_slaves=" << n_slaves << ", ";

        fungus_util_assert(argc > 2 && sscanf(argv[2], "%u", &n_trials) == 1, "Could not read first argument: n_trials!");
        std::cout << "n_trials=" << n_trials << "\n\n";

        return run_test_args();
    }
}
