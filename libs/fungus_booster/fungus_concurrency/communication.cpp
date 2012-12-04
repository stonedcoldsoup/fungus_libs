#include "fungus_concurrency_comm_internal.h"

namespace fungus_concurrency
{
    comm::comm(impl *pimpl_): pimpl_(pimpl_) {}

    comm::comm(size_t max_channels, size_t cmd_io_nslots, sec_duration_t timeout_period, discard_message_callback *discard)
    {
        pimpl_ = new impl(max_channels, cmd_io_nslots, timeout_period, discard);
    }

    comm::~comm()
    {
        delete pimpl_;
    }

    comm::channel_id comm::open_channel(comm *mcomm, int data)          {return pimpl_->open_channel(mcomm->pimpl_, data);}
    bool comm::close_channel(channel_id id, int data)                   {return pimpl_->close_channel(id, data);}
    void comm::close_all_channels(int data)                             {       pimpl_->close_all_channels(data);}
    bool comm::does_channel_exist(channel_id id) const                  {return pimpl_->does_channel_exist(id);}
    bool comm::is_channel_open(channel_id id) const                     {return pimpl_->is_channel_open(id);}
    bool comm::channel_send(channel_id id, const any_type &m_message)   {return pimpl_->channel_send(id, m_message);}
    bool comm::channel_receive(channel_id id, any_type &m_message)      {return pimpl_->channel_receive(id, m_message);}
    bool comm::channel_discard(channel_id id, size_t n)                 {return pimpl_->channel_discard(id, n);}
    bool comm::channel_discard_all(channel_id id)                       {return pimpl_->channel_discard_all(id);}
    void comm::all_channels_discard_all()                               {       pimpl_->all_channels_discard_all();}
    void comm::dispatch()                                               {       pimpl_->dispatch();}
    bool comm::peek_event(event &event) const                           {return pimpl_->peek_event(event);}
    bool comm::get_event(event &event)                                  {return pimpl_->get_event(event);}
    void comm::clear_events()                                           {       pimpl_->clear_events();}
}
