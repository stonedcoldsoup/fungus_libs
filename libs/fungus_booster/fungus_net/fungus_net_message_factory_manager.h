#ifndef FUNGUSNET_MESSAGE_FACTORY_MANAGER_H
#define FUNGUSNET_MESSAGE_FACTORY_MANAGER_H

#include "fungus_net_message.h"

namespace fungus_net
{
    using namespace fungus_util;

    /** @addtogroup message_api Message API
      * @{
      */

    /** The message_factory_manager contained in each host
      * object.  It is used to map each message_type to
      * a message::factory class that will create the correct
      * message for that message type.  A message::factory
      * can be queried for it's associated message type,
      * we need only pass a message factory to add_factory.
      *
      */
    class FUNGUSNET_API message_factory_manager
    {
    private:
        class impl;
        impl *pimpl_;
    public:
        /** @{ */

        /// Default constructor.
        message_factory_manager();

        /** Move constructor.
          *
          * @param m_mgr    message_factory_manager to move.
          */
        message_factory_manager(message_factory_manager &&m_mgr);

        /// Destructor.
        ~message_factory_manager();

        /** @} */
        /** @{ */

        /** Add a factory class to the message_type to factory map.
          * The following convention should be used for all calls
          * to add_factory():
          *
          * <code>
          *    m_message_factory_mgr.add_factory(my_factory_class());
          * </code>
          *
          * @param factory  rvalue reference to the factory to be inserted
          *                 in to the map.
          *
          * @retval true    on success
          * @retval false   on failure
          */
        bool add_factory(message::factory &&factory);

        /** Remove a factory class from the message_type to factory map.
          *
          * @param type     the message_type of the factory to be removed.
          *
          * @retval true    on success
          * @retval false   on failure
          */
        bool remove_factory(message_type type);

        /// Clear all factories in the map.
        void clear_factories();

        /** @} */

        /** Get the factory associated with a message_type.
          *
          * @param type                the message_type of the factory to be retreived.
          *
          * @retval == <b>nullptr</b>  no factory is associated with this message_type
          * @retval > <b>nullptr</b>   a pointer to the message::factory object.
          *
          */
        const message::factory *get_factory(message_type type) const;
    };

    /** @} */
}

#endif
