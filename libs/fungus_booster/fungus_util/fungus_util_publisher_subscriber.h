#ifndef FUNGUSUTIL_PUBLISHER_SUBSCRIBER_H
#define FUNGUSUTIL_PUBLISHER_SUBSCRIBER_H

#include "fungus_util_common.h"
#include "fungus_util_block_allocator.h"

namespace fungus_util
{
	template <typename objT, typename... argT>
    struct subscriber_function_type
    {
        typedef void (objT::*type)(argT&&...);
    };
    
    namespace detail
    {
        template <typename objT, typename... argT>
        class subscription_base
        {
        private:
            typedef
                typename subscriber_function_type<objT, argT...>::type
                subscriber_function;
        
            uint32_t n_refs;
            
            objT *m_obj;
            subscriber_function m_fn;
        
            template <typename, typename...> friend class subscription;
            template <typename, typename...> friend class publisher;
            template <typename, size_t> friend class block_allocator;
            
            void call(argT&&... argV)
            {
                (m_obj->*m_fn)(std::forward<argT>(argV)...);
            }
            
            subscription_base(objT *m_obj, subscriber_function m_fn):
                n_refs(0),
                m_obj(m_obj),
                m_fn(m_fn)
            {}
        };
    }
    
    template <typename objT, typename... argT>
    class subscription
    {
    private:
        typedef
            detail::subscription_base<objT, argT...>
            subsbase_type;
            
        template <typename, typename...> friend class publisher;
        
        subsbase_type *m_subs;
                
        subscription(subsbase_type *m_subs):
            m_subs(m_subs)
        {
            ++m_subs->n_refs;
        }
    public:
        subscription():
            m_subs(nullptr)
        {}
    
        subscription(const subscription &m_subs):
            m_subs(nullptr)
        {
            operator =(m_subs);
        }
        
        subscription &operator =(const subscription &m_subs)
        {
            if (this->m_subs)
                --this->m_subs->n_refs;
            this->m_subs = m_subs.m_subs;
            if (this->m_subs)
                ++this->m_subs->n_refs;
                
            return *this;
        }
        
        ~subscription()
        {
            if (m_subs)
                --m_subs->n_refs;
        }
    };
    
    template <typename objT, typename... argT>
    class publisher
    {
    private:
        struct _s_nil {};
    
        typedef
            detail::subscription_base<objT, argT...>
            subsbase_type;
    
        typedef
            hash_map
            <
                default_hash
                <
                    subsbase_type *,
                    _s_nil
                >
            >
            subscription_map;
    
        subscription_map m_map;
        block_allocator<subsbase_type, 512> s_blocks;
        
        template <typename> friend class writer;
        
        void publish(argT&&... argV)
        {
            for (auto it = m_map.begin(); it != m_map.end(); ++it)
            {
                subsbase_type *m_subsbase = it->key;
                if (m_subsbase->n_refs > 0)
                    m_subsbase->call(std::forward<argT>(argV)...);
                else
                {
                    s_blocks.destroy(m_subsbase);
                    it = m_map.erase(it);
                    if (it == m_map.end())
                        break;
                }
            }
        }
    public:
        typedef
            subscription<objT, argT...>
            subscription_type;
        
        typedef
            typename subscriber_function_type<objT, argT...>::type
            subscriber_function;
    
        publisher(uint32_t n_probable_writers = 512):
            m_map(),
            s_blocks(std::max(n_probable_writers / uint32_t(512), uint32_t(1)))
        {}
        
        publisher(const publisher &m) = delete;
        publisher(publisher &&m) = delete;
        
        publisher &operator =(const publisher &m) = delete;
        publisher &operator =(publisher &&m) = delete;
        
        subscription_type subscribe(objT *m_obj, subscriber_function m_fn)
        {
            subsbase_type *m_subs = s_blocks.create(m_obj, m_fn);
            m_map.insert(typename subscription_map::entry(m_subs, _s_nil()));
            
            return subscription_type(m_subs);
        }
    };
    
    template <typename publisherT>
    class writer
    {
    private:
        publisherT &m_publisher;
        
    public:
        writer(publisherT &m_publisher):
            m_publisher(m_publisher)
        {}
    
        template <typename... argT>
        void operator()(argT&&... argV)
        {
            m_publisher.publish(std::forward<argT>(argV)...);
        }
    };
    
    template <typename objT, typename... argT>
    struct publisher_subscriber_set
    {
        typedef void (objT::*function_type)(argT&&...);
        
        typedef subscription<objT, argT...> subscription_type;
        typedef publisher<objT, argT...>    publisher_type;
        typedef writer<publisher_type>      writer_type;
    };
}

#endif