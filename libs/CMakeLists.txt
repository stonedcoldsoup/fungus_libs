########################
# fungus_booster CMake #
########################

# This file was created by modifying the
# CMakeLists.txt file from PhoenixCore, as
# it has all of the dependencies of PhoenixCore.

CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

PROJECT( fungus_booster )

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

ADD_DEFINITIONS( -DBUILD_FUNGUSUTIL )
ADD_DEFINITIONS( -DBUILD_FUNGUSCONCURRENCY )
ADD_DEFINITIONS( -DBUILD_FUNGUSNET )

#if (BUILD_FUNGUSNET)
#	if (NOT BUILD_FUNGUSCONCURRENCY)
#		ADD_DEFINITIONS( -DBUILD_FUNGUSCONCURRENCY )
#	endif()
#endif()


#############################
# Disallow in-source builds #
#############################

IF( "${fungus_booster_SOURCE_DIR}" STREQUAL "${fungus_booster_BINARY_DIR}" )
	MESSAGE( FATAL_ERROR "\fungus_booster must NOT be built from it's root directory.\n"
			"Make two directories called debug and release, run cmake from each.\n" )
ENDIF()

######################
# Check Dependencies #
######################

MESSAGE ( "Searching for dependencies..." )


FIND_PACKAGE( Threads REQUIRED )
SET( LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

#################################
# Change the output directories #
#################################
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${fungus_booster_SOURCE_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${fungus_booster_SOURCE_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${fungus_booster_SOURCE_DIR}/bin)
SET(CMAKE_OUTPUT_OUTPUT_DIRECTORY ${fungus_booster_SOURCE_DIR}/bin)

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
