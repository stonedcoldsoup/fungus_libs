cmake_minimum_required (VERSION 2.6)
project ( fungus_booster )

set( FUNGUS_BOOSTER_FILES
	fungus_booster.h
	version.cpp
	fungus_util/any.cpp
	fungus_util/bidirectional_container_base.cpp
	fungus_util/endian.cpp
	fungus_util/fungus_util.h
	fungus_util/fungus_util_any.h
	fungus_util/fungus_util_attribute_array_container.h
	fungus_util/fungus_util_auto_ptr.h
	fungus_util/fungus_util_bidirectional_container_base.h
	fungus_util/fungus_util_block_allocator.h
	fungus_util/fungus_util_cfg_file.cpp
	fungus_util/fungus_util_cfg_file.h
	fungus_util/fungus_util_clone_map.h
	fungus_util/fungus_util_common.h
	fungus_util/fungus_util_constexpr.h
	fungus_util/fungus_util_endian.h
	fungus_util/fungus_util_from_string.h
	fungus_util/fungus_util_hash_map.h
	fungus_util/fungus_util_make_string.h
	fungus_util/fungus_util_multi_tree.h
	fungus_util/fungus_util_optional.h
	fungus_util/fungus_util_pow2.h
	fungus_util/fungus_util_predicates.h
	fungus_util/fungus_util_prime.h
	fungus_util/fungus_util_sfinae.h
	fungus_util/fungus_util_singleton.h
	fungus_util/fungus_util_std_ext.h
	fungus_util/fungus_util_string_op.cpp
	fungus_util/fungus_util_string_op.h
	fungus_util/fungus_util_timestamp.h
	fungus_util/fungus_util_type_info_wrap.h
	fungus_util/fungus_util_user.h
	fungus_util/timestamp.cpp
	fungus_util/thread/fungus_util_condition.h
	fungus_util/thread/fungus_util_mutex.h
	fungus_util/thread/fungus_util_thread.h
	fungus_util/thread/fungus_util_thread_common.h
	fungus_util/thread/thread.cpp
	fungus_util/fs/fungus_util_fs.cpp
	fungus_util/fs/fungus_util_fs.h
	)

if (BUILD_FUNGUSCONCURRENCY)
set( FUNGUS_BOOSTER_FILES
	${FUNGUS_BOOSTER_FILES}
	fungus_concurrency/communication.cpp
	fungus_concurrency/process.cpp
	fungus_concurrency/fungus_concurrency.h
	fungus_concurrency/fungus_concurrency_common.h
	fungus_concurrency/fungus_concurrency_communication.h
	fungus_concurrency/fungus_concurrency_comm_internal.h
	fungus_concurrency/fungus_concurrency_concurrent_auto_ptr.h
	fungus_concurrency/fungus_concurrency_concurrent_queue.h
	fungus_concurrency/fungus_concurrency_process.h
	)
endif()

if (BUILD_FUNGUSNET)
set( FUNGUS_BOOSTER_FILES
	${FUNGUS_BOOSTER_FILES}
	fungus_net/authenticator.cpp
	fungus_net/common.cpp
	fungus_net/defs.cpp
	fungus_net/fungus_net.h
	fungus_net/fungus_net_authenticator.h
	fungus_net/fungus_net_authenticator_internal.h
	fungus_net/fungus_net_common.h
	fungus_net/fungus_net_defs.h
	fungus_net/fungus_net_defs_internal.h
	fungus_net/fungus_net_host.h
	fungus_net/fungus_net_host_internal.h
	fungus_net/fungus_net_message.h
	fungus_net/fungus_net_message_factory_manager.h
	fungus_net/fungus_net_message_intercept.h
	fungus_net/fungus_net_packet.h
	fungus_net/fungus_net_peer.h
	fungus_net/fungus_net_peer_internal.h
	fungus_net/fungus_net_unity.h
	fungus_net/fungus_net_unity_base.h
	fungus_net/fungus_net_unity_enet.h
	fungus_net/fungus_net_unity_memory.h
	fungus_net/host.cpp
	fungus_net/host_internal.cpp
	fungus_net/message.cpp
	fungus_net/message_intercept.cpp
	fungus_net/packet.cpp
	fungus_net/peer_base.cpp
	fungus_net/peer_concrete.cpp
	fungus_net/peer_group.cpp
	fungus_net/unity.cpp
	fungus_net/unity_base.cpp
	)
endif()

if(WIN32)
	ADD_DEFINITIONS(-DWIN32)
    set( FUNGUS_BOOSTER_FILES
		${FUNGUS_BOOSTER_FILES}
		fungus_net/enet/callbacks.c
		fungus_net/enet/callbacks.h
		fungus_net/enet/compress.c
		fungus_net/enet/host.c
		fungus_net/enet/list.c
		fungus_net/enet/list.h
		fungus_net/enet/packet.c
		fungus_net/enet/peer.c
		fungus_net/enet/protocol.c
		fungus_net/enet/protocol.h
		fungus_net/enet/time.h
		fungus_net/enet/types.h
		fungus_net/enet/utility.h
		fungus_net/enet/win32.c
		fungus_net/enet/win32.h
		)
endif(WIN32)

if (DLL_FUNGUSUTIL OR SO_FUNGUSUTIL)
	
	add_library( fungus_booster SHARED
		 ${FUNGUS_BOOSTER_FILES}
		)
		
	if(UNIX)
		target_link_libraries( fungus_booster -lenet )
	elseif(WIN32)
		target_link_libraries( fungus_booster -luser32 -lws2_32 -lwinmm )
	endif()

else (DLL_FUNGUSUTIL OR SO_FUNGUSUTIL)
	add_library( fungus_booster STATIC
		 ${FUNGUS_BOOSTER_FILES}
		)
endif()

set_target_properties(fungus_booster PROPERTIES LINK_FLAGS_RELEASE -s )
set_target_properties(fungus_booster PROPERTIES DEBUG_POSTFIX "_d" )

