#include "fungus_net_message.h"
#include "fungus_net_message_factory_manager.h"
#include "fungus_net_defs_internal.h"

namespace fungus_net
{
    class message_factory_manager::impl
    {
    private:
        class factory_map_hash:
            public default_hash_no_replace<message_type, message::factory *>
        {};

        typedef hash_map<factory_map_hash> map_type;

        mutable  mutex m;
        map_type factories;
    public:
        inline impl(): m(), factories() {}

        inline ~impl()
        {
            factories.clear();
        }

        inline bool add_factory(message::factory &&factory)
        {
            lock guard(m);

            return factories.insert(map_type::entry(factory.get_type(), factory.move())) != factories.end();
        }

        inline bool remove_factory(message_type type)
        {
            lock guard(m);

            return factories.erase(type);
        }

        inline void clear_factories()
        {
            factories.clear();
        }

        inline const message::factory *get_factory(message_type type) const
        {
            lock guard(m);

            auto it = factories.find(type);
            return (it == factories.end()) ? nullptr : it->value;
        }
    };

    message_factory_manager::message_factory_manager():
        pimpl_(nullptr)
    {
        pimpl_ = new impl;
    }

    message_factory_manager::message_factory_manager(message_factory_manager &&m_mgr):
        pimpl_(m_mgr.pimpl_)
    {
        m_mgr.pimpl_ = nullptr;
    }

    message_factory_manager::~message_factory_manager()
    {
        if (pimpl_) delete pimpl_;
    }

    bool message_factory_manager::add_factory(message::factory &&factory)
    {
        return pimpl_ ? pimpl_->add_factory(std::move(factory)) : false;
    }

    bool message_factory_manager::remove_factory(message_type type)
    {
        return pimpl_ ? pimpl_->remove_factory(type) : false;
    }

    void message_factory_manager::clear_factories()
    {
        if (pimpl_) pimpl_->clear_factories();
    }

    const message::factory *message_factory_manager::get_factory(message_type type) const
    {
        return pimpl_ ? pimpl_->get_factory(type) : nullptr;
    }

    class message::impl
    {
    private:
        message *m_message;

        friend class message;

        struct _cm_copy
        {
            FUNGUSUTIL_ALWAYS_INLINE
            inline message *operator()(const message *m_message)
            {
                message *nm_message = m_message->copy();
                fungus_util_assert(nm_message, "message copy failed!");

                return nm_message;
            }
        };

        struct _cm_get
        {
            typedef clonable<MAP_GLOBAL, message, bool, _cm_copy, _cm_get> __clonable;

            FUNGUSUTIL_ALWAYS_INLINE
            inline __clonable *operator()(message *m_message)
            {
                return &(m_message->pimpl_->clonable);
            }
        };

        _cm_get::__clonable clonable;

        message::stream_mode smode;
        uint8_t channel;
    public:
        FUNGUSUTIL_ALWAYS_INLINE
        inline impl(message *m_message, message::stream_mode smode, uint8_t channel):
            m_message(m_message), clonable(m_message, false, _cm_copy(), _cm_get()), smode(smode), channel(channel)
            {m_message->pimpl_ = this;}

        FUNGUSUTIL_ALWAYS_INLINE
        inline ~impl() {}

        FUNGUSUTIL_ALWAYS_INLINE
        inline message *clone()
        {
            message *nm_message = clonable.clone();
            fungus_util_assert(nm_message, "message copy failed!");

            return nm_message;
        }

        FUNGUSUTIL_ALWAYS_INLINE
        inline void mark_as_read()
        {
            clonable.global_data(true);
        }

        FUNGUSUTIL_ALWAYS_INLINE
        inline bool marked_as_read() const
        {
            return clonable.global_data();
        }
    };

    message::message(stream_mode smode, uint8_t channel)
    {
        pimpl_ = new impl(this, smode, channel);
    }

    message::~message()
    {
        delete pimpl_;
    }

    message::stream_mode message::get_stream_mode() const {return pimpl_->smode;}
    uint8_t message::get_channel() const                  {return pimpl_->channel;}

    message *message::clone()                             {return pimpl_->clone();}

    void message::mark_as_read()                          {       pimpl_->mark_as_read();}
    bool message::marked_as_read() const                  {return pimpl_->marked_as_read();}

    protocol_message::protocol_message():
        message(message::stream_mode::sequenced, internal_defs::i_protocol_channel)
    {}

    user_message::user_message(stream_mode smode, uint8_t channel):
        message(smode, internal_defs::i_user_channel_base + channel)
    {
        fungus_util_assert(channel <= internal_defs::max_user_channel_count, "Channel selected is out of range!");
    }
}
