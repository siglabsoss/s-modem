include("find_libevent.cmake")

cmake_minimum_required(VERSION 2.6)
project(SideControl CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add some custom CXX flags.  (this is the official way to do a += in cmake)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(TEST_LIBEV_LINK_LIBRARIES
	# -ljsoncpp -lcurl -ljsonrpccpp-common -ljsonrpccpp-client
	m
	)

set(TEST_LIBEV_SOURCE_FILES
    src/test_libevent.cpp
    # src/common/utilities.hpp
    # src/common/utilities.cpp
)

add_definitions(-DVERILATE_TESTBENCH)

include_directories(
src/
)

if(BUILD_EXTRA)

add_executable(test_libevent ${TEST_LIBEV_SOURCE_FILES})
target_link_libraries(test_libevent ${TEST_LIBEV_LINK_LIBRARIES} ${LIBEVENT_LIBRARIES})

endif()
