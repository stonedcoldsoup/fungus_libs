########################
# fungus_booster CMake #
########################

# This file was created by modifying the
# CMakeLists.txt file from PhoenixCore, as
# it has all of the dependencies of PhoenixCore.

CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( fungus_booster )

if(CMAKE_COMPILER_IS_GNUCXX)
	if (NDEBUG)
		set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -O3")
		set(CMAKE_EXE_LINKER_FLAGS "-s")
	else (NDEBUG)
		set(CMAKE_CXX_FLAGS "-g -std=c++0x -DFUNGUS_CONCURRENCY_NO_INLINE")
	endif()
endif()

ADD_DEFINITIONS( -DBUILD_FUNGUSUTIL )
ADD_DEFINITIONS( -DBUILD_FUNGUSCONCURRENCY )
ADD_DEFINITIONS( -DBUILD_FUNGUSNET )

#if (BUILD_FUNGUSNET)
#	if (NOT BUILD_FUNGUSCONCURRENCY)
#		ADD_DEFINITIONS( -DBUILD_FUNGUSCONCURRENCY )
#	endif()
#endif()

#SET(CMAKE_BUILD_TYPE  Debug)

#############################
# Disallow in-source builds #
#############################

IF( NOT "${fungus_booster_SOURCE_DIR}" STREQUAL "${fungus_booster_BINARY_DIR}" )
	MESSAGE( FATAL_ERROR "\fungus_booster must be built from it's root directory.\n"
			"Run cmake in the root directory of the project.\n" )
ENDIF ( NOT "${fungus_booster_SOURCE_DIR}" STREQUAL "${fungus_booster_BINARY_DIR}" )

######################
# Check Dependencies #
######################

MESSAGE ( "Searching for dependencies..." )


FIND_PACKAGE( Threads REQUIRED )
SET( LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

#################################
# Change the output directories #
#################################
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_OUTPUT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#######################
# Include Directories #
#######################

include_directories( ${fungus_booster_SOURCE_DIR}/fungus_booster
					 ${fungus_booster_SOURCE_DIR}/fungus_booster/fungus_util
					 ${fungus_booster_SOURCE_DIR}/fungus_booster/fungus_concurrency
					 ${fungus_booster_SOURCE_DIR}/fungus_booster/fungus_net
					 ${INCLUDES})

###################
# Include subdirs #
###################

subdirs( fungus_booster )
