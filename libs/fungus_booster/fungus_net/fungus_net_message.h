#ifndef FUNGUSNET_MSG_H
#define FUNGUSNET_MSG_H

#include "fungus_net_common.h"

namespace fungus_net
{
    using namespace fungus_util;

    /** @addtogroup message_api Message API
      * @{
      */

    /** The message class is an abstract base class
      * for all messages sent and received in fungus_net.
      *
      * message is inherited by user_message and
      * protocol_message.  If you wish to define
      * your own message class, you should inherit
      * from either of those.
      *
      * message has several pure virtual member functions
      * which must be overridden.  See member functions
      * for more details.
      *
      */
    class FUNGUSNET_API message
    {
    public:
        /** The stream_mode represents how messages are sequenced
          * when sent over a network.  stream_mode is not used by
          * memory connections, messages will always arrive in the
          * order sent.
          *
          */
        enum class stream_mode
        {
            /** stream_mode should never be invalid, unless an internal error
              * occured.  A message with a stream_mode of invalid cannot be sent.
              */
            invalid,
            /** Always receive messages in the order sent.
              * This uses a reliability layer to ensure that messages are
              * always received in the correct order, but may be substantially
              * slower than unsequenced streaming under any reasonable load.
              * Reserve the use of this to cases where it is critical that messages
              * arrive in the correct order, such as a protocol_message, where
              * sequenced is implicitly used as the stream_mode.
              */
            sequenced,
            /** Receive messages in any arbitrary order.
              * Because no reliability layer is used other than ensuring that the
              * message arrived, this stream_mode can be faster.
              */
            unsequenced
        };

        /** The base message factory class.
          *
          * For each message_type you create, you must have a corresponding
          * message factory class.  The message factory class is only used
          * by networked connections, memory connections simply pass the message
          * along to the other host.
          *
          * You must inherit your message factory class from this class
          * and then pass an instance to any host that will need to deserialize
          * a message of this type.
          *
          */
        class FUNGUSNET_API factory
        {
        public:
            /** Create a message object.
              *
              * @returns A new message object.
              */
            virtual message     *create()   const = 0;

            /** Move this factory to a dynamically allocated instance.
              * You must allocate the factory using standard C++ new.
              *
              * @returns The new factory.
              */
            virtual factory     *move()     const = 0;

            /** Get the message_type associated with this factory.
              *
              * @returns The message_type associated with this factory.
              */
            virtual message_type get_type() const = 0;
			
			virtual ~factory() {}
        };

        /// Virtual destructor.
        virtual ~message();

        /** @{ */

        /// Get the stream_mode for the message.
        virtual stream_mode  get_stream_mode() const;

        /** Get the channel the message will be sent over.
          * Each channel is sequenced seperately.
          *
          * @returns The channel that this message has been / will be sent over.
          */
        virtual uint8_t      get_channel()     const;

        /** \brief Get the message_type for this message.  Must be overloaded.
          *
          * @returns The message_type for this message.
          */
        virtual message_type get_type() const = 0;

        /** @} */
        /** @{ */

        /** Clone this message object.
          * By calling clone() instead of copy(), you have
          * linked the produced copy of this message to
          * this message, and any message that produced it
          * through a call to clone() recursively.
          * This is important to mark_as_read() and
          * marked_as_read() calls.
          *
          * @returns A clone of this message.
          */
        virtual message      *clone();

        /** \brief Copy the message.  Must be overloaded.
          *
          * @returns A copy of this message.
          */
        virtual message      *copy()    const = 0;

        /** @} */
        /** @{ */

        /** Mark this message as read.  Any message in
          * this message's clone tree will return true
          * on a call to marked_as_read().  mark_as_read()
          * will be called internally when a message is
          * intercepted via a message intercept (see host).
          * If a message has been marked as read, any clones
          * will be skipped and destroyed on any calls to
          * peer::receive_message().  On receipt of a message,
          * all duplicates waiting in any peer groups, including
          * the all_peer_id group will be clones.
          */
        virtual void mark_as_read();

        /** If a call to mark_as_read has been made in
          * any message in this message's clone tree
          * (see clone()), this will return true,
          * otherwise, false.
          *
          * @retval true if message's clone tree has been marked as read
          * @retval false if message's clone tree has not been marked as read
          */
        virtual bool marked_as_read() const;

        /** @} */
    protected:
        /** @{ */
        /** \brief Serialize the data contained in this message using the
          * provided serializer object. Must be overloaded.
          *
          * @param s          the serializer object.
          */
        virtual void serialize_data(serializer &s) const = 0;

        /** \brief Deserialize the incoming data and store it in this
          * message using the provided deserializer object.  Must be
          * overloaded.
          *
          * @param s          the deserializer object.
          */
        virtual void deserialize_data(deserializer &s) = 0;
        /** @} */

        friend class packet;
        friend class protocol_message;
        friend class user_message;
    private:
        message(stream_mode smode, uint8_t channel);

        class impl;
        impl *pimpl_;
    };

    /** Protocol message base class.
      *
      * protocol_messages are always on the reserved channel, and
      * are always sequenced.  Use only when it is critical that
      * the messages of this type be received in order along with
      * other internal protocol messages so that the state of the
      * host assumed by the sender is still accurate upon receipt.
      * You should rarely if ever have to use a protocol_message
      * outside of user management.
      */
    class FUNGUSNET_API protocol_message: public message
    {
    public:
        /// Constructor
        protocol_message();
    };

    /** User message base class.
      *
      * user_messages can be sequenced on any channel except for the
      * reserved protocol channel, and can have any stream mode.
      * All generic event messages should be derived from user_message.
      *
      */
    class FUNGUSNET_API user_message: public message
    {
    public:
        /** Constructor
          *
          * @param smode    the stream_mode for this message.
          * @param channel  the channel that this message should be sequenced on.
          */
        user_message(stream_mode smode, uint8_t channel = 0);
    };

    /** @} */
}

#endif
