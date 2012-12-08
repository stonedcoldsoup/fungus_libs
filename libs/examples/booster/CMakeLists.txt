#################################
# fungus_booster examples CMake #
#################################

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

PROJECT( fungus_booster_examples )

set(CMAKE_CXX_FLAGS_RELEASE "-Wall -std=c++0x -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x -g")

set(CMAKE_C_FLAGS_RELEASE "-Wall -ansi -O3")
set(CMAKE_C_FLAGS_DEBUG "-Wall -ansi -g ")

# hax lol
if(DLL_FUNGUSBOOSTER)
	ADD_DEFINITIONS( -DDLL_FUNGUSUTIL=1 )
endif()

if(NOT CMAKE_COMPILER_IS_GNUCXX)
	message (FATAL_ERROR "fungus_booster is only compatible with GNU C++ 4.x or later.\n")
endif()

######################
# Check Dependencies #
######################

MESSAGE ( "Searching for dependencies..." )

FIND_PACKAGE( Threads REQUIRED )
SET( LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

set( SEARCH_PATHS
	${FUNGUSBOOSTER_PATH}
	${FUNGUSBOOSTER_PATH}/lib
	)

FIND_PATH( FUNGUSBOOSTER_INCLUDE_DIR fungus_booster/fungus_booster.h PATHS ${SEARCH_PATHS} )

if (DLL_FUNGUSUTIL)
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster_d.dll PATHS ${SEARCH_PATHS} )
	else(CMAKE_BUILD_TYPE MATCHES Debug)
		FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster.dll PATHS ${SEARCH_PATHS} )
	endif(CMAKE_BUILD_TYPE MATCHES Debug)
	if (FUNGUSBOOSTER_LIBRARY)
		message("Copying fungus_booster runtime...")
		file(COPY ${FUNGUSBOOSTER_PATH}/bin DESTINATION ${CMAKE_BINARY_DIR})
	else (FUNGUSBOOSTER_LIBRARY)
		message( FATAL_ERROR "not found!\n"
							 "Try building without -DDLL_FUNGUSUTIL.\n" )
	endif()
else (DLL_FUNGUSUTIL)
	if (CMAKE_BUILD_TYPE MATCHES Debug)
		FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster_d PATHS ${SEARCH_PATHS} )
	else(CMAKE_BUILD_TYPE MATCHES Debug)
		FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster PATHS ${SEARCH_PATHS} )
	endif(CMAKE_BUILD_TYPE MATCHES Debug)
	if (NOT FUNGUSBOOSTER_LIBRARY)
		message( FATAL_ERROR "Could not find fungus_booster library.\n"
							 "Try building with -DDLL_FUNGUSUTIL.\n" )
	endif()
endif(DLL_FUNGUSUTIL)

SET( LIBRARIES ${LIBRARIES} ${FUNGUSBOOSTER_LIBRARY} )
SET( INCLUDES ${INCLUDES} ${FUNGUSBOOSTER_INCLUDE_DIR} )

#################################
# Change the output directories #
#################################
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${fungus_booster_examples_SOURCE_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${fungus_booster_examples_SOURCE_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${fungus_booster_examples_SOURCE_DIR}/bin)
SET(CMAKE_OUTPUT_OUTPUT_DIRECTORY ${fungus_booster_examples_SOURCE_DIR}/bin)

#######################
# Include Directories #
#######################

include_directories( ${INCLUDES})

add_executable( test0 test0.cpp )
target_link_libraries( test0 ${LIBRARIES} )

add_executable( test1 test1.cpp )
target_link_libraries( test1 ${LIBRARIES} )

add_executable( test2 test2.cpp )
target_link_libraries( test2 ${LIBRARIES} )

add_executable( test3 test3.cpp )
target_link_libraries( test3 ${LIBRARIES} )

set_target_properties(test0 test1 test2 test3 PROPERTIES LINK_FLAGS_RELEASE -s )
set_target_properties(test0 test1 test2 test3 PROPERTIES DEBUG_POSTFIX "_d" )
