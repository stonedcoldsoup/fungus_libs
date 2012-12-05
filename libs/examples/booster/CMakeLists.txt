#################################
# fungus_booster examples CMake #
#################################

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

PROJECT( fungus_booster_examples )

if(CMAKE_COMPILER_IS_GNUCXX)
	if (NDEBUG)
		set(CMAKE_CXX_FLAGS "-Wall -std=c++0x -O3")
		set(CMAKE_EXE_LINKER_FLAGS "-s")
	else (NDEBUG)
		set(CMAKE_CXX_FLAGS "-g -std=c++0x -DFUNGUS_CONCURRENCY_NO_INLINE")
	endif()
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
	FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster.dll PATHS ${SEARCH_PATHS} )
	if (FUNGUSBOOSTER_LIBRARY)
		message("Copying fungus_booster.dll...")
		file(COPY ${FUNGUSBOOSTER_PATH}/bin DESTINATION ${CMAKE_BINARY_DIR})
	else (FUNGUSBOOSTER_LIBRARY)
		message( FATAL_ERROR "Could not find libfungus_booster.dll.a.\n"
							 "Try building without -DDLL_FUNGUSUTIL.\n" )
	endif()
else (DLL_FUNGUSUTIL)
	FIND_LIBRARY( FUNGUSBOOSTER_LIBRARY fungus_booster PATHS ${SEARCH_PATHS} )
	if (NOT FUNGUSBOOSTER_LIBRARY)
		message( FATAL_ERROR "Could not find libfungus_booster.a.\n"
							 "Try building with -DDLL_FUNGUSUTIL.\n" )
	endif()
endif(DLL_FUNGUSUTIL)

SET( LIBRARIES ${LIBRARIES} ${FUNGUSBOOSTER_LIBRARY} )
SET( INCLUDES ${INCLUDES} ${FUNGUSBOOSTER_INCLUDE_DIR} )

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

include_directories( ${INCLUDES})

add_executable( test0 test0.cpp )
target_link_libraries( test0 ${LIBRARIES} )

add_executable( test1 test1.cpp )
target_link_libraries( test1 ${LIBRARIES} )

add_executable( test2 test2.cpp )
target_link_libraries( test2 ${LIBRARIES} )

add_executable( test3 test3.cpp )
target_link_libraries( test3 ${LIBRARIES} )
