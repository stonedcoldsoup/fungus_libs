#ifndef FUNGUSNET_INCLUDE_H
#define FUNGUSNET_INCLUDE_H

#include "fungus_net_common.h"
#include "fungus_net_defs.h"

#include "fungus_net_message.h"
#include "fungus_net_message_factory_manager.h"

#include "fungus_net_authenticator.h"

#include "fungus_net_host.h"
#include "fungus_net_peer.h"

/**

 \mainpage Manual
 \section intro Introduction

 fungus_net is a powerful C++11 networking library designed specifically
 with games in mind.  The documentation for fungus_net is not 100% complete,
 but will be shortly.  Before diving in to fungus_net, you should understand
 at least basic networking concepts and paradigms.

 This manual is a more academic approach to learning fungus_net.  If you learn
 better by example, then it is recommended that you look through, compile, run,
 and experiment with the example code included in the SDK.

 \section man_contents Contents

 \ref overview    "Overview"\n
 \ref basic_usage "Basic Usage"

 \page overview Overview

 \section hosts_and_peers Hosts and Peers
 The most fundamental concept in fungus_net is the idea of hosts and peers.

 A host can represent a server or a client, but there is no
 distinguishment between which is which.  A host is simply a hub which can
 connect to other hubs, and can behave as either.

 A peer represents a connection to another host, and has a
 state which represents the current status of the connection.
 Each host has a list of peers which represent its connections to other hosts.
 In fungus_net, a peer object may also represent a group of peers or peer groups.
 This distinguishment is purely abstract, and peer groups and concrete peers
 each have the same API.  For example, calling host::disconnect with a peer
 group as the peer to disconnect will actually disconnect every concrete peer
 referenced by that peer group.

 \section group_trees Peer Group Trees
 Each peer will reference a list of concrete peers.  Concrete peers represent
 an actual connection to another host, while all other peers are groups of
 these concrete peers.

 If a peer represents a concrete peer, then it references
 itself.  Peer groups do NOT reference themselves, but instead reference the concrete
 peers that they contain and the concrete peers referenced by groups that they
 contain, recursively.

 Take for example three peer groups and three concrete peers that we will call
 <code>A, B, C,</code> and <code>a, b, c,</code> respectively.  Say we place
 \c a and \c b inside of group \c A and we place \c b and \c c inside of group
 \c B.  The trees would then look like:

 <code>
 A\n
 +-a\n
 +-b

 B\n
 +-b\n
 +-c
 </code>

 Now say that we place \c A and \c B inside of \c C.  The the concrete peers
 referenced by group \c C are now <code>a, b, and c</code>.  Concrete peer
 \c b is not repeated in the list, even though C has two indirect references
 to it.

 It is also possible to place a peer inside of more than one peer group, and
 all of the references are resolved according to the rules laid out above.  The
 only exception to this is that you <i>absolutely must not circularly reference a
 peer group.</i> This means that if \c A contains \c B contains \c C contains \c A,
 the behavior of the library is \b undefined, and so it should never be done and
 may crash or hang.

 \section messages Messages
 Hosts send messages back and forth to communicate.  Because the peer represents
 a connection to another host (or group of connections to other hosts), the API for
 sending and receiving waiting messages is in the peer class.

 The same exact rules apply to sending and receiving messages and peer groups.  Sending to a peer
 group will actually call send on every concrete peer referenced by the group, receiving
 will get an incoming message from any of the concrete peers referenced by the group.
 See the reference documentation for more details on how this is handled, and getting
 duplicate messages can be avoided.

 Each message is represented by a message object.  The message class is a virtual base
 class that is meant to be inherited from.  Each message is sent over a specified channel
 with a specific stream mode.  The stream mode can either be sequenced (slower/reliable order)
 or unsequenced (faster/unreliable order).  Each channel represents a seperate stream, so
 messages sent over seperate channels will never be sequenced together.  Only messages that
 arrive on the same channel and are both sequenced will be sequenced properly.

 The message class has two member functions (serialize_data and deserialize_data) which you
 are responsible for overloading.  Converting your data structure in to a serial buffer to be sent over
 the network and back.  See the reference documentation for details.  Each of these functions
 take a serializer and a deserializer object respectively. \ref fungus_util_placeholder_doc
 "serializer, deserializer, and any_type" will be covered to the extent that you will need to
 use them with fungus_net below until the fungus_util documentation is complete.

 Here is an example of a user defined message class:
 \code
 // here we define our custom message type
 constexpr fungus_net::message_type test_message_type = 700;

 // here is our custom message class
 // we inherit from user_message because we do not want to tie
 // up the protocol channel and we want unsequenced streaming.
 // this could be, for example, a game event.
 class test_message: public fungus_net::user_message
 {
 public:
    const int some_int_attr;      // some factory attribute.
    std::string some_string_data; // the data being sent.  when sending and receiving strings
                                  // with a message, or any time a string is used with a serializer,
                                  // please use std::string.

    // this is the factory class for our message class.  The factory
    // is important because when a message is recieved by a host, the
    // message type (which is always at the beginning of the packet
    // sent over the network) is used as a key to figure out which
    // factory to use when it creates the message object that will
    // deserialize the incoming packet data.
    class factory: public fungus_net::message::factory
    {
    public:
        // this factory will always create incoming message objects
        // with this int attribute.  it is not neccesary (and may even
        // be bad form) to do this.  this is just here to demonstrate
        // one of the tricks you could do with a factory.
        const int some_int_attr;

        // factory constructor.
        factory(int some_int_attr): some_int_attr(some_int_attr) {}

        // this callback will be called whenever a message is recieved over the
        // network, and this factory is mapped to the message_type of that
        // message.  This member function should create a new message object
        // of the appropriate type.  deserialize_data() will be called on the
        // message object immediately afterward with a deserializer to a buffer
        // containing the contents of that message.
        virtual fungus_net::message *create() const
        {
            test_message *m_message = new test_message(some_int_attr, "");
            return m_message;
        }

        // this callback is called once when adding a new factory to a
        // message_factory_manager.  The factory passed is a temporary reference
        // and will be deleted, and the result of this function will be
        // stored in the message_type->factory map.
        virtual fungus_net::message::factory *move() const
        {
            factory *m_factory = new factory(some_int_attr);
            return m_factory;
        }

        // This function should return the message_type to map this factory
        // to.
        virtual fungus_net::message_type get_type() const
        {
            return test_message_type;
        }
    };

    // constructor.
    test_message(int some_int_attr, const std::string &some_string_data):
        user_message(fungus_net::message::stream_mode::unsequenced, 42),
        some_int_attr(some_int_attr), some_string_data(some_string_data)
    {}

    // produce a copy of this message (must be overloaded).
    virtual fungus_net::message *copy() const
    {
        test_message *m_message = new test_message(some_int_attr, some_string_data);
        return m_message;
    }

    // the message_type for this message.  (must be overloaded).
    virtual fungus_net::message_type get_type() const
    {
        return test_message_type;
    }
 protected:
    // the callback called when this message is sent over a networked connection.
    // it should insert all data to be included in the packet in to the serializer
    // object.
    virtual void serialize_data(fungus_util::serializer &s) const
    {
        s << some_string_data;
    }

    // the callback called on a new message object after it is created by a factory
    // when a message of this type is received over the network.  it should extract
    // all data to be loaded back in to the structor from the deserializer object.
    virtual void deserialize_data(fungus_util::deserializer &s)
    {
        s >> some_string_data;
    }
 };

 ...

 // somewhere in the code: add the factory to our host object...
 m_host.get_message_factory_manager().add_factory(test_message::factory());

 // And you're good to go!
 \endcode

 \section fungus_util_placeholder_doc Dependencies on fungus_util
 There are five classes from fungus_util that fungus_net's API depends on:
 \c any_type
 \c serializer
 \c deserializer
 \c serializer_buf
 \c endian_converter

 The only times you will ever need to interact with a serializer or deserializer
 is when one is passed to you in serialize_data or deserialize_data, or when
 get_serializer() or get_deserializer() is called on an authentication payload (see below).

 A serializer can be treated exactly as a std::ostream, except that standard IO manipulators
 are not compatible and would have no purpose, as the serializer is a binary memory stream.

 the code:
 \code
 virtual void serialize_data(fungus_util::serializer &s)
 {
     s << some_int << some_string << some_float;
 }
 \endcode

 Used in a custom message class, the above code would insert those items in to the serializer stream in that order.
 The deserializer is the input binary memory stream, and has the interface you would expect:

 \code
 virtual void deserialize_data(fungus_util::deserializer &s)
 {
     s >> some_int >> some_string >> some_float;
 }
 \endcode

 The serializer has two public member error flags which can be checked directly called \c fail and \c error.
 \c fail means that no operation exists to serialize that data
 \c error means that an internal error occurred.

 These would be accessed as such:\n
 \code
 bool did_it_fail = s.fail;
 bool was_there_an_error = s.error;
 \endcode

 The deserializer has just one error flag (which is a public member as well) called \c eof.  The \c eof flag
 is set if an extraction operation tried to read past the end of the input message buffer.

 This would be accessed as such:\n
 \code
 bool did_it_hit_end_of_buffer = s.eof;
 \endcode

 A serializer_buf is a raw buffer with a size and a block of memory.  The \c buf member is
 a pointer to the buffer, and the \c size member is the number of bytes in the buffer.
 You can insert and extract serializer buffers in to serializers and out of deserializers,
 but the size of the buffer is not inserted or extracted in the stream.  So you must either
 use a fixed length buffer for the data:

 \code
 // serialize message
 fungus_util::serializer_buf m_buf(64);
 ...

 some_serializer << m_buf;

 ...

 // deserialize message
 fungus_util::serializer_buf m_buf(64);
 ...

 some_deserializer >> m_buf;
 \endcode

 Or insert and extract the size explicitly:

 \code
 // serialize message
 fungus_util::serializer_buf m_buf;
 ...

 some_serializer << m_buf.size << m_buf;

 ...

 // deserialize message
 fungus_util::serializer_buf m_buf;

 size_t m_buf_size;
 some_deserializer >> m_buf_size;

 m_buf.set(m_buf_size);
 some_deserializer >> m_buf;
 \endcode

 See the fungus_util documentation for more details.

 The serializer and deserializer classes, as well as all classes that
 will store user data use the any_type class as a storage type.  any_type
 does not need to be explicitly documented for the purposes of fungus_net,
 only know that it behaves as a dynamically typed variable.  It supports
 boolean comparison operators and stream insertion and extraction.
 All types are converted to an any_type before being inserted in to a
 serializer.  However, never extract an any_type from a deserializer,
 instead extract the type that the any_type which was inserted stored.
 Since all use of any_type by the serializers and deserializers is
 implicit, you should just make it a policy to avoid explicitly
 inserting an any_type in to a serializer.

 The user data storage methods of various APIs in fungus_net all take
 an any_type as the type for the user data.  Any type can be implicitly
 casted to an any_type, so calling such a function is a simple as:

 \code
 m_peer->set_user_data(some_string);
 \endcode

 although any type could be passed.  To cast an any_type back to
 the type stored in it you can use \c any_cast.

 \code
 std::string m_str = fungus_util::any_cast<std::string>(m_peer->get_user_data());
 \endcode

 The last class you will need from fungus_net is the endian_converter class.
 All primitive numeric POD types are automatically converted to big endian when
 inserted in to a serializer, and back when extracted from a deserializer.
 However, if you wish to flip the bytes of some other type when sending them
 over a network from a little endian machine (say, a union that contains numeric
 PODs), then you should register that class with the endian_converter on each
 host object in your code:

 \code
 m_host.get_endian_converter().register_type<my_numeric_union_t>();
 \endcode

 This wraps up the use of fungus_util with fungus_net.  fungus_util is not
 yet thoroughly documented, and may never be, as it is more of a personal project.
 It is likely that the members of fungus_util called from fungus_net will either
 be integrated in to fungus_net and documented there or moved to a new library called
 fungus_stream and documented there, given that it is also used minimally in
 fungus_concurrency.  This is the one detail that is likely to change a lot before
 the release of fungus_net, although the behavior and API will very likely
 remain consistent in order to avoid any serious refactoring on the part of users
 and myself.

 \page basic_usage Basic Usage

 \section setup_host Setting Up a Host
 In order to use any of the functionality of fungus_net, you must first set up
 a host object to connect to other host objects.  Before anything else, you should
 check the version_info struct returned by get_version_info() against the FUNGUSBOOSTER_VERSION
 macro to make sure that they match, and initialize the library.

 In your main function you will need some code like:
 \code
 // it is a good policy to make sure that the dll for fungus_net that was
 // linked to during load time is the correct version.
 assert(fnet::get_version_info().m_version.val == FUNGUSBOOSTER_VERSION);

 // initialize fungus_net globally.  must succeed before using fungus_net.
 assert(fungus_net::initialize());

 fungus_net::host m_host;                           // our host object
 fungus_net::host::networked_host_args m_net_args;  // the special arguments taken by hosts that support networked connections.

 // set the address that our host should bind to.
 m_net_args.m_ipv4 = fungus_net::ipv4(fungus_net::ipv4::host("localhost"), // resolve hostname localhost to an IP address.
                                      8999);                               // bind to port 8999.

 // start the host. this must succeed before using the host. starting
 // the host should only fail if fungus_net has not been globally initialized.
 assert(m_host.start(fungus_net::host::flag_support_both, // connection types to support (can be flag_support_memory_connection
                                                          // flag_support_networked_connection, or both).
                     32,                                  // Maximum number of simultaneously connected peers to support.
                     m_net_args));                        // Networked host specific arguments.
 \endcode

 \section connect_host Starting a Connection
 The next thing you will need to do is request a connection with another host.  There are two
 methods for doing this.

 The first is to pass another host object the connect() member function,
 which will open a thread-safe pseudo networked connection through memory.  This can only be successful
 if the host was started with the fungus_net::host::flag_support_memory_connection flag. The other is
 to pass an ipv4 address to the connect() member function, which will open a genuinely
 networked connection over a UDP socket.  This can only be successful if the host was started with
 the fungus_net::host::flag_support_networked_connection flag.

 Starting a networked connection:
 \code

 // create an authentication payload to send to the other host
 // upon successful connection.  In this case, we create a payload
 // with a status of success, and no data.
 fungus_net::auth_payload *m_payload = m_host.create_auth_payload(fungus_net::auth_status::success);

 // create a new peer that will subsequently request a connection from
 // the host at m_some_ipv4_address (which is a fungus_net::ipv4).  The
 // authentication payload (m_payload) will be sent automatically if the
 // initial request for a connection is successful.
 fungus_net::peer         *m_peer    = m_host.connect(m_payload,            // queues the initial payload.
                                                      0,                    // a uint32 to be sent to the other host.
                                                                            // could be used to send a client type code,
                                                                            // for example.
                                                      m_some_ipv4_address); // the address of the host to connect to.

 // if the peer returned is nullptr, then requesting a connection failed either
 // because the number of peers is already at maximum, or because networked
 // connection is not supported by this host.
 assert(m_peer);
 \endcode

 Starting a memory connection:
 \code
 // exactly the same as creating a networked connection, except that the
 // last argument to the connect() member function is a pointer to a host object.
 fungus_net::peer *m_peer = m_host.connect(m_payload, 0, &m_some_other_host);
 \endcode

 Just a note, you should not hold on to the peer object between calls to
 dispatch() on the corresponding host, because it could be destroyed without
 warning, leading to a segmentation fault if any attempt is made to access it.

 Instead, keep the peer_id handle:
 \code
 fungus_net::peer *m_peer = fungus_net::peer *m_peer = m_host.connect(m_payload, 0, &m_some_other_host);

 // do something with the peer object.
 ...

 peer_id m_id = m_peer->get_id();

 ...
 // and when you need to access the peer object again:
 m_peer = m_host.get_peer(m_id);

 // make sure the peer still exists.
 assert(m_peer);
 \endcode

 \section host_auth Authentication
 In the previous section we saw how to set up a connection.  In that section, it was
 neccessary to introduce the auth_payload class.  An auth_payload is a very simple
 object that behaves much like a message, but does not need to be inherited, and has
 special rules about how it is sent and received.

 An authentication payload must be
 created with a call to create_auth_payload() and destroyed with a call to
 destroy_auth_payload().  A payload that is sent or queued to be sent will be
 destroyed for you, and a payload that has been received has already been created
 for you.

 Authentication payloads are sent via the send_auth_payload() member function in host.
 An authentication payload will be queued for sending if the state of the target peer
 is still state::connecting (the connection is pending), or sent immediately if the
 peer's state is state::authenticating or state::connected.  send_auth_payload() will
 fail if the peer is disconnecting or disconnected.

 Authentication shall be complete on the target peer on it's associated host if it has
 received (and ONLY if it has received) an authentication payload with a status of
 auth_status::success or auth_status::failure.  If it has received a payload with a
 status of failure, the peer is destroyed immediately.  If it has received a payload
 with a status of success, the peer transitions from authenticating to connected.

 The following diagrams illustrate some of the permutations of this
 exchange that are possible.

 Host A fails to reach Host B:
 \msc
    a [label="Host A"], b [label="Host B"];

    a-xb [label="Connection Request"];
    ---  [label="Host A has timed out waiting for connection."];
    a>>a [label="host::event\ntype=peer_rejected\nrejection_reason=rejection_timed_out"];
    a=>a [label="Destroy the peer\nrepresenting Host B."];
 \endmsc

 Host A reaches Host B, Host B fails to respond:
 \msc
    a [label="Host A"], b [label="Host B"];

    a->b [label="Connection Request"];
    b-xa [label="Connection Accepted"];
    ---  [label="Host A has timed out waiting for connection."];
    a>>a [label="host::event\ntype=peer_rejected\nrejection_reason=rejection_timed_out"];
    a=>a [label="Destroy the peer\nrepresenting Host B."];
    ---  [label="Host B has timed out waiting for auth_payload."];
    b>>b [label="host::event\ntype=auth_timeout"];
    b=>b [label="Destroy the peer\nrepresenting Host A."];
 \endmsc

 Host A reaches Host B, Host B rejects the connection:
 \msc
    a [label="Host A"], b [label="Host B"];

    a->b [label="Connection Request"];
    b->a [label="Connection Denied"];
    a>>a [label="host::event\ntype=peer_rejected\nrejection_reason=rejection_denied"];
    a=>a [label="Destroy the peer\nrepresenting Host B."];
 \endmsc

 A successful authentication exchange between Host A and Host B:
 \msc
    a [label="Host A"], b [label="Host B"];

    a->b [label="Connection Request data=0"];
    b->a [label="Connection Accepted data=0"];
    a>>a [label="host::event\ntype=peer_connected\n data=0"];
    a=>a [label="Send the queued initial\nauthentication packet."];
    a->b [label="auth_payload status=in_progress"];
    b>>b [label="host::event\ntype=auth_payload_received\nstatus=in_progress"];
    ---  [label="user looks at Host B auth_payload\nand sends a reply."];
    b->a [label="auth_payload status=success"];
    a>>a [label="host::event\ntype=auth_payload_received\nstatus=success"];
    a=>a [label="set state of peer representing Host B to connected."];
    ---  [label="user looks at Host A auth_payload\nand sends confirmation of success."];
    a->b [label="auth_payload status=success"];
    b>>b [label="host::event\ntype=auth_payload_received\nstatus=success"];
    b=>b [label="set state of peer representing Host A to connected."];
 \endmsc

 Upon receiving a successful authentication payload, that peer's state transitions
 from authenticating to connected.

 A failed authentication exchange between Host A and Host B:
 \msc
    a [label="Host A"], b [label="Host B"];

    a->b [label="Connection Request data=0"];
    b->a [label="Connection Accepted data=0"];
    a>>a [label="host::event\ntype=peer_connected\n data=0"];
    a=>a [label="Send the queued initial\nauthentication packet."];
    a->b [label="auth_payload status=in_progress"];
    b>>b [label="host::event\ntype=auth_payload_received\nstatus=in_progress"];
    ---  [label="user looks at Host B auth_payload\nand sends a reply."];
    b->a [label="auth_payload status=failure"];
    a>>a [label="host::event\ntype=auth_payload_received\nstatus=failure"];
    a=>a [label="Destroy the peer\nrepresenting Host B."];
    a->b [label="Disconnect"];
    b>>b [label="host::event\ntype=peer_disconnected\ndata=0"];
    b=>b [label="Destroy the peer\nrepresenting Host A."];
 \endmsc

 If any authentication messages are lost, the authentication attempt will time out,
 resutling in failure.  This should never happen unless the user fails to provide
 a reply to each appropriate authentication packet.

 Each authentication payload has a serializer object and a deserializer object accessed
 through get_serializer() and get_deserializer() respectively.  The serializer is only
 valid on outgoing auth_payloads and the deserializer is only valid on incoming payloads.
 These are accessed in exactly the same way you would in a message, except that you cannot
 use more than one type of authentication payload. You must simply use the neccesary logic
 to distinguish between types of authentication (if any) in your own code.

 For robustness, you should only send an authentication payload in direct response to a
 payload that was just received.  For more information on receiving and sending authentication
 payloads, see the examples under \ref host_events "Event Handling".

 \section host_events Event Handling

 Write me, I am incomplete.

 \section final_assembly Putting It All Together

 Write me, I am incomplete.

 */

#endif
