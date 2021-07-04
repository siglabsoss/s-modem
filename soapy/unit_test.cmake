# Run tests with:
#  make test CTEST_OUTPUT_ON_FAILURE=TRUE

if(BUILD_EXTRA)

cmake_minimum_required(VERSION 2.6)

# add_subdirectory("src/3rd/schifra")

project(SideControl CXX)

include("find_libevent.cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# add some custom CXX flags.  (this is the official way to do a += in cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=attributes -g -O0 -Werror=return-type")
add_subdirectory("${3RD_DIR}/rapidcheck")

# This allows us to use assert() in the code for good measure
# cmake automatically adds this flag for release builds.  at least for now
# I would like to know when asserts fail
# remove this when running the demo?
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# message(STATUS ${CMAKE_CXX_FLAGS})

set(UNIT_TEST_LINK_LIBRARIES
	-lboost_system
	)

# To add a new test, simply add it to the executable list below. The name of the
# test must correspond to the name of the test file in src/test. For example,
# if you want to build sr/test/test_random.cpp, add test_random to the
# executable list
set(EXECUTABLE_LIST
    test_fec
#     test_csocket
    test_interleaver
    test_circular_buffer_pow2
    test_tx_rx
    # test_packet_hash
    test_convert
    test_convert_2
    test_air_packet_1
    test_air_packet_2
    test_air_packet_3
    # test_air_packet_4
    # test_air_packet_5
    # test_air_packet_6
    test_air_packet_7
    test_air_packet_8
    test_air_packet_9
    test_air_packet_10
    test_hash
    test_schedule_1
    test_schedule_2
    test_eq_filter
    test_evm_1
    test_evm_2
    test_cplx_1
    test_verify_hash
    test_select_sort
    test_client
    test_duplex_1
    test_duplex_2
    test_fb_split_1
    test_reed_solomon_speed
    test_mac_header
    test_duplex_mode
    test_client_duplex
    test_rx_schedule
    test_standalone_fb_parse
    test_emulate_mapmov_1


    # Put slow ones last
    test_reed_solomon
    test_demod_thread
    test_demod_thread_1
)

set(DISABLED_TESTS
    test_tx_rx
    test_eq_filter
    test_demod_thread  # this one segfauls on the buildservers for some reason
    test_evm_1
    test_evm_2
    test_air_packet_3
    test_air_packet_4
    test_air_packet_5
    test_air_packet_6
    test_client
    test_client_duplex
)


# used to build the fast_test target by skipping these
set(SLOW_TESTS
    test_reed_solomon
    test_demod_thread
)

# tests for ben, feel free to copy for your own name
#     test_reed_solomon_speed
set(BENS_TESTS
#     test_fb_split_1
    test_duplex_2
#     test_air_packet_9
#     test_mac_header
#     test_duplex_mode # slow
)




# sources of DUT or "Device Under Test", aka classes we want to test
set(UNIT_TEST_DUT_SOURCES
    # ${DRIVER_DIR}/PredictBuffer.cpp
    # ${DRIVER_DIR}/PredictBuffer.hpp
    # ${COMMON_DIR}/utilities.hpp
    # ${COMMON_DIR}/utilities.cpp
    ${COMMON_DIR}/CounterHelper.hpp
    ${COMMON_DIR}/CounterHelper.cpp
    ${COMMON_DIR}/NullBufferSplit.hpp
    ${COMMON_DIR}/NullBufferSplit.cpp
    ${COMMON_DIR}/CmdRunner.hpp
    ${COMMON_DIR}/CmdRunner.cpp
    ${COMMON_DIR}/convert.cpp
    ${DRIVER_DIR}/StatusCode.hpp
    ${DRIVER_DIR}/StatusCode.cpp
    ${DRIVER_DIR}/AirPacket.hpp
    ${DRIVER_DIR}/AirPacket.cpp
    ${DRIVER_DIR}/ReedSolomon.hpp
    ${DRIVER_DIR}/ReedSolomon.cpp
    ${DRIVER_DIR}/MacHeader.hpp
    ${DRIVER_DIR}/MacHeader.cpp
    ${DRIVER_DIR}/Interleaver.hpp
    ${DRIVER_DIR}/Interleaver.cpp
    ${DRIVER_DIR}/VerifyHash.hpp
    ${DRIVER_DIR}/VerifyHash.cpp
    ${COMMON_DIR}/crc32.hpp
    ${COMMON_DIR}/crc32.cpp
    ${DRIVER_DIR}/RetransmitBuffer.hpp
    ${DRIVER_DIR}/RetransmitBuffer.cpp
    ${DRIVER_DIR}/csocket.hpp
    ${DRIVER_DIR}/csocket.cpp
    ${COMMON_DIR}/FileUtils.hpp
    ${COMMON_DIR}/FileUtils.cpp
    ${COMMON_DIR}/cplx.hpp
    ${COMMON_DIR}/cplx.cpp
    ${3RD_DIR}/turbofec/src/turbo_enc.cpp
    ${3RD_DIR}/json.hpp
    ${DRIVER_DIR}/VerifyHash.cpp
    ${DRIVER_DIR}/VerifyHash.hpp
    ${DRIVER_DIR}/AirMacInterface.hpp
    ${DRIVER_DIR}/AirMacBitrate.cpp
    ${DRIVER_DIR}/AirMacBitrate.hpp
    ${DRIVER_DIR}/StandAloneFbParse.cpp
    ${DRIVER_DIR}/StandAloneFbParse.hpp
    ${DRIVER_DIR}/EmulateMapmov.cpp
    ${DRIVER_DIR}/EmulateMapmov.hpp

# these are HUGE and take a literal minute to compile on a desktop pc
# must be fixed

    # src/data/airpacket0.cpp
    # src/data/airpacket1.cpp
    # src/data/airpacket2.cpp
    # src/data/airpacket3.cpp
    # src/data/airpacket5.cpp
    # src/data/airpacket7.cpp
    # src/data/airpacket18.cpp
    # src/data/demodpacket0.cpp
    # src/data/demodpacket1.cpp
    # src/data/slicedpacket0.cpp

)


# Source files that come from the RISCV folder
# Some of these files are dual C/C++ files with a macro, but they have the .c
# extension
set(RISCV_SOURCES
${RISCV_BASEBAND_REPO}/c/inc/schedule.c
${RISCV_BASEBAND_REPO}/c/inc/feedback_bus.c
${RISCV_BASEBAND_REPO}/c/inc/random.c
${RISCV_BASEBAND_REPO}/c/inc/circular_buffer_pow2.c
${RISCV_BASEBAND_REPO}/c/inc/duplex_schedule.c
)

# Extra work we have to do for dual role files
SET_SOURCE_FILES_PROPERTIES( 
${RISCV_SOURCES}
PROPERTIES LANGUAGE CXX )


set_property(
   SOURCE ${RISCV_SOURCES}
   PROPERTY COMPILE_DEFINITIONS
     VERILATE_TESTBENCH=1
 )

set_property(
   SOURCE ${RISCV_SOURCES}
   PROPERTY COMPILE_DEFINITIONS
     OUR_RING_ENUM RING_ENUM_PC
 )

set(UNIT_TEST_INCLUDE_DIRS 
${RISCV_BASEBAND_REPO}/c/inc
${HIGGS_ROOT}/sim/verilator/inc
${Boost_INCLUDE_DIR}
${3RD_DIR}/schifra
)


add_definitions(-DVERILATE_TESTBENCH)

include_directories(
${3RD_DIR}/turbofec/
${3RD_DIR}/turbofec/include/
${3RD_DIR}/turbofec/src/
src/
src/driver/
)



include_directories(${UNIT_TEST_INCLUDE_DIRS})



######################################
# Previous setup compiled each file N times (where N is the number of unit test)
# With this UnitTestDeps, we compile all of the cpp files (except for main) into a .so file
# this means a change to one file only results in one compile
add_library(UnitTestDeps SHARED ${UNIT_TEST_DUT_SOURCES} ${RISCV_SOURCES})




foreach(executable ${EXECUTABLE_LIST})

    message(STATUS "Building " ${executable} "")
    message(STATUS "")

    add_executable(${executable}
                   ${TEST_DIR}/${executable}.cpp)
    target_link_libraries(${executable}
                          ${SOAPY_SDR_LIBRARY}
                          m
                          pthread
                          UnitTestDeps
                          ${UNIT_TEST_LINK_LIBRARIES})
    target_link_libraries(${executable} rapidcheck)
endforeach(executable)




######################################
# Test specific files
#


# The PRIVATE and PUBLIC keywords specify where those corresponding sources should be used.
# PRIVATE simply means those sources should only be added to myLib,
# whereas PUBLIC means those sources should be added to myLib and to any target that
# links to myLib
# see https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/
target_sources(test_demod_thread PRIVATE
${DRIVER_DIR}/DemodThread.hpp
${DRIVER_DIR}/DemodThread.cpp
${DRIVER_DIR}/HiggsSettings.hpp
${DRIVER_DIR}/HiggsSettings.cpp
${DRIVER_DIR}/HiggsEvent.hpp
${DRIVER_DIR}/HiggsEvent.cpp
)

target_link_libraries(test_demod_thread ${LIBEVENT_LIBRARIES} -pthread)





###################################
# for running tests:
#

####
# Copy EXECUTABLE_LIST to RUNABLE_TEST_LIST
set(RUNABLE_TEST_LIST
${EXECUTABLE_LIST}
)

# subtract un-runable tests
list(REMOVE_ITEM RUNABLE_TEST_LIST ${DISABLED_TESTS})
####


####
# Copy RUNABLE_TEST_LIST to FAST_RUNABLE_TEST_LIST
set(FAST_RUNABLE_TEST_LIST
${RUNABLE_TEST_LIST}
)

# subtract
list(REMOVE_ITEM FAST_RUNABLE_TEST_LIST ${SLOW_TESTS})
####



# add_custom_target(foo_info COMMAND echo ${EXECUTABLE_LIST})
# add_custom_target(foo_info2 COMMAND echo ${RUNABLE_TEST_LIST})

enable_testing() # Enables testing for this directory and below.

# Loop for main tests
foreach(runable ${RUNABLE_TEST_LIST})
add_test(NAME run_${runable} COMMAND ${runable})
endforeach(runable)
add_custom_target(test_all COMMAND make -j12 all test CTEST_OUTPUT_ON_FAILURE=TRUE)



# Loop for fast tests





# Loop for bens tests
foreach(runable ${BENS_TESTS})
set(BEN_FOO
${BEN_FOO} make ${runable} && ./${runable};
)
endforeach(runable)
add_custom_target(ben_test COMMAND ${BEN_FOO} )



# Run tests with:
#  make test CTEST_OUTPUT_ON_FAILURE=TRUE





endif()