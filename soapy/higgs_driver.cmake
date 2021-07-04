########################################################################
# Project setup -- only needed if device support is a stand-alone build
# We recommend that the support module be built in-tree with the driver.
########################################################################
cmake_minimum_required(VERSION 2.6)
project(SoapySDRHiggs CXX)
enable_testing()

# include sub-files to find and get deps for this project
include("find_zmq.cmake")
include("find_libevent.cmake")

FIND_PACKAGE( Boost 1.40 COMPONENTS program_options system random REQUIRED )


# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

# add -Wshadow
set(CMAKE_CXX_FLAGS
"${CMAKE_CXX_FLAGS} \
-O3 -Werror=return-type -Wall -Wextra -fPIC -Wno-unused-function -Wno-error=attributes \
-std=gnu++11 -Wvla -Wcast-qual -Wdangling-else -Winit-self -Werror=uninitialized \
-Werror=incompatible-pointer-types -Werror=array-bounds -Wshadow \
-Wduplicated-cond -Wnull-dereference -Wdangling-else -Waddress \
-Wint-in-bool-context -Winit-self \
-Wpointer-arith \
-Werror=uninitialized \
-Werror=strict-prototypes \
-Wlogical-op -Werror=logical-op -Werror=null-dereference \
-Werror=sequence-point \
-Werror=missing-braces -Werror=write-strings -Werror=address -Werror=array-bounds \
-Werror=char-subscripts -Werror=enum-compare -Werror=implicit-int \
-Werror=empty-body -Werror=main -Werror=nonnull -Werror=parentheses \
-Werror=pointer-sign -Werror=ignored-qualifiers \
-Werror=missing-parameter-type -Werror=unused-value \
-Wmissing-declarations \
-Wmissing-field-initializers \
-Wcast-align \
-Wduplicated-branches \
-Wformat \
-Wformat-security \
-Wformat-signedness \
-Woverlength-strings \
-Wredundant-decls \
-Wsynth \
-Wtrampolines \
-Wvariadic-macros \
-Wvirtual-inheritance \
"
)
# -Wfloat-equal \
# -Wsign-promo \
# -Wuseless-cast \
# -Wsign-conversion \
# -Wconversion \
# -Wnon-virtual-dtor \
# set(CXX_FLAGS "${CXX_FLAGS} -std=gnu++11")

message(STATUS ${CMAKE_CXX_FLAGS})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")


#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Header and library resources needed to communicate with the device.
# These may be found within the build tree or in an external project.
########################################################################
set(MY_DEVICE_INCLUDE_DIRS 
${RISCV_BASEBAND_REPO}/c/inc
${HIGGS_ROOT}/sim/verilator/inc
${Boost_INCLUDE_DIR}
src/common
${3RD_DIR}/schifra
)

message(STATUS ${Boost_LIBRARIES})

list(APPEND MY_DEVICE_LIBRARIES
	${ZEROMQ_LIBRARY}
	${LIBEVENT_LIBRARIES}
	
	${Boost_LIBRARIES}
	-pthread
	)

########################################################################
# build the module
########################################################################
# find_package(SoapySDR CONFIG)

# message(STATUS "foo")
# message(STATUS SoapySDR_FOUND)

# if (NOT SoapySDR_FOUND)
#     message(WARNING "SoapySDR development files not found - skipping support")
#     return()
# else ()
#     message(STATUS "SoapySDR FOUND here - ${SOAPY_SDR_LIBRARY}")
# endif ()



set(DRIVER_SOURCE_FILES
    ${DRIVER_DIR}/HiggsDriverSharedInclude.hpp
    ${DRIVER_DIR}/HiggsSoapyDriver.cpp
    ${DRIVER_DIR}/HiggsSoapyDriver.hpp
    ${DRIVER_DIR}/HiggsSoapySettings.cpp
    ${DRIVER_DIR}/HiggsSoapyStreaming.cpp
    ${DRIVER_DIR}/HiggsSoapyFeedbackBus.cpp
    ${DRIVER_DIR}/csocket.hpp
    ${DRIVER_DIR}/csocket.cpp
    ${DRIVER_DIR}/HiggsRing.cpp
    ${DRIVER_DIR}/HiggsRing.hpp
    ${DRIVER_DIR}/UnusedStubs.cpp
    ${DRIVER_DIR}/UnusedStubs.hpp
    ${DRIVER_DIR}/HiggsZmq.cpp
    ${DRIVER_DIR}/HiggsZmqDiscovery.cpp
    ${DRIVER_DIR}/SubcarrierEstimate.cpp
    ${DRIVER_DIR}/SubcarrierEstimate.hpp
    ${DRIVER_DIR}/HiggsTDMA.cpp
    ${DRIVER_DIR}/HiggsTDMA.hpp
    ${DRIVER_DIR}/HiggsUdpOOO.hpp
    ${DRIVER_DIR}/HiggsSettings.hpp
    ${DRIVER_DIR}/HiggsSettings.cpp
    ${DRIVER_DIR}/HiggsPartner.cpp
    ${DRIVER_DIR}/RadioEstimate.hpp
    ${DRIVER_DIR}/RadioEstimate.cpp
    ${DRIVER_DIR}/AttachedEstimate.hpp
    ${DRIVER_DIR}/AttachedEstimate.cpp
    ${DRIVER_DIR}/HiggsTunThread.cpp
    ${DRIVER_DIR}/HiggsEvent.cpp
    ${DRIVER_DIR}/HiggsEvent.hpp
    ${DRIVER_DIR}/EventDsp.hpp
    ${DRIVER_DIR}/EventDsp.cpp
    ${DRIVER_DIR}/EventDspFetch.cpp
    ${DRIVER_DIR}/EventDspArbiter.cpp
    ${DRIVER_DIR}/FlushHiggs.hpp
    ${DRIVER_DIR}/FlushHiggs.cpp
    ${DRIVER_DIR}/TxSchedule.hpp
    ${DRIVER_DIR}/TxSchedule.cpp
    ${DRIVER_DIR}/DashTie.hpp
    ${DRIVER_DIR}/DashTie.cpp
    ${DRIVER_DIR}/EventDspFsm.cpp
    ${DRIVER_DIR}/EventDspFsmTx.cpp
    ${DRIVER_DIR}/HiggsStructTypes.hpp
    ${DRIVER_DIR}/FsmMacros.hpp
    ${DRIVER_DIR}/BoostIOThread.cpp
    ${DRIVER_DIR}/CustomEventTypes.hpp
    ${DRIVER_DIR}/BoostTcpClient.hpp
    ${DRIVER_DIR}/BoostTcpClient.cpp
    ${DRIVER_DIR}/EventDspDash.cpp
    ${COMMON_DIR}/NullBufferSplit.hpp
    ${COMMON_DIR}/NullBufferSplit.cpp
    ${COMMON_DIR}/FileUtils.hpp
    ${COMMON_DIR}/FileUtils.cpp
    ${DRIVER_DIR}/DispatchMockRpc.hpp
    ${DRIVER_DIR}/DispatchMockRpc.cpp

    ${DRIVER_DIR}/ReedSolomon.cpp
    ${DRIVER_DIR}/ReedSolomon.hpp
    ${DRIVER_DIR}/MacHeader.cpp
    ${DRIVER_DIR}/MacHeader.hpp

    ${DRIVER_DIR}/AirMacInterface.hpp
    ${DRIVER_DIR}/AirMacBitrate.hpp
    ${DRIVER_DIR}/AirMacBitrate.cpp

    ${DRIVER_DIR}/AirPacket.hpp
    ${DRIVER_DIR}/AirPacket.cpp

    ${COMMON_DIR}/CounterHelper.hpp
    ${COMMON_DIR}/CounterHelper.cpp

    ${COMMON_DIR}/CmdRunner.hpp
    ${COMMON_DIR}/CmdRunner.cpp

    ${DRIVER_DIR}/StatusCode.cpp
    ${DRIVER_DIR}/StatusCode.hpp
    
    ${DRIVER_DIR}/HiggsFineSync.cpp
    ${DRIVER_DIR}/HiggsFineSync.hpp

    ${DRIVER_DIR}/RetransmitBuffer.cpp
    ${DRIVER_DIR}/RetransmitBuffer.hpp

    ${DRIVER_DIR}/ScheduleData.cpp
    ${DRIVER_DIR}/ScheduleData.hpp

    ${DRIVER_DIR}/VerifyHash.cpp
    ${DRIVER_DIR}/VerifyHash.hpp

    ${DRIVER_DIR}/DemodThread.cpp
    ${DRIVER_DIR}/DemodThread.hpp

    ${DRIVER_DIR}/EVMThread.cpp
    ${DRIVER_DIR}/EVMThread.hpp

    ${DRIVER_DIR}/RadioDemodTDMA.cpp
    ${DRIVER_DIR}/RadioDemodTDMA.hpp

    ${DRIVER_DIR}/Interleaver.hpp
    ${DRIVER_DIR}/Interleaver.cpp

    ${DRIVER_DIR}/HiggsSNR.hpp
    ${DRIVER_DIR}/HiggsSNR.cpp

    ${DRIVER_DIR}/StandAloneFbParse.cpp
    ${DRIVER_DIR}/StandAloneFbParse.hpp

    ${DRIVER_DIR}/EmulateMapmov.cpp
    ${DRIVER_DIR}/EmulateMapmov.hpp

    ${COMMON_DIR}/utilities.hpp
    ${COMMON_DIR}/utilities.cpp
    ${COMMON_DIR}/discovery.cpp
    ${COMMON_DIR}/constants.hpp
    ${COMMON_DIR}/convert.hpp
    ${COMMON_DIR}/convert.cpp
    ${COMMON_DIR}/crc32.hpp
    ${COMMON_DIR}/crc32.cpp
    ${COMMON_DIR}/cplx.cpp
    ${COMMON_DIR}/ReadbackHash.hpp
    ${COMMON_DIR}/ReadbackHash.cpp
    ${COMMON_DIR}/DataVectors.hpp
    ${COMMON_DIR}/DataVectors.cpp
    ${COMMON_DIR}/GenericOperator.hpp
    ${COMMON_DIR}/GenericOperator.cpp

    ${3RD_DIR}/json.hpp

    ${RISCV_BASEBAND_REPO}/c/inc/schedule.c
    ${RISCV_BASEBAND_REPO}/c/inc/feedback_bus.c
    ${RISCV_BASEBAND_REPO}/c/inc/random.c
    ${RISCV_BASEBAND_REPO}/c/inc/fixed_iir.c
    ${RISCV_BASEBAND_REPO}/c/inc/duplex_schedule.c

)

SET_SOURCE_FILES_PROPERTIES( 
	${RISCV_BASEBAND_REPO}/c/inc/schedule.c
    ${RISCV_BASEBAND_REPO}/c/inc/feedback_bus.c
    ${RISCV_BASEBAND_REPO}/c/inc/random.c
    ${RISCV_BASEBAND_REPO}/c/inc/fixed_iir.c
    ${RISCV_BASEBAND_REPO}/c/inc/duplex_schedule.c
	PROPERTIES LANGUAGE CXX )

set_property(
   SOURCE ${RISCV_BASEBAND_REPO}/c/inc/schedule.c
   PROPERTY COMPILE_DEFINITIONS
     VERILATE_TESTBENCH=1
 )

add_definitions(-DVERILATE_TESTBENCH)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ljsoncpp -lmicrohttpd -ljsonrpccpp-common -ljsonrpccpp-server")
# add_compile_options(-std=c++1z)
# add_compile_options(-std=gnu++17)

include_directories(${MY_DEVICE_INCLUDE_DIRS})


add_library(HiggsDriver SHARED ${DRIVER_SOURCE_FILES})
target_link_libraries(HiggsDriver ${MY_DEVICE_LIBRARIES})

# will build static library. was needed for node-gyp but not for cmake-js
#add_library(HiggsDriverStatic ${DRIVER_SOURCE_FILES})
#target_link_libraries(HiggsDriverStatic ${MY_DEVICE_LIBRARIES})



# SOAPY_SDR_MODULE_UTIL(
#     TARGET HiggsDriver
#     SOURCES ${DRIVER_SOURCE_FILES}
#     LIBRARIES ${MY_DEVICE_LIBRARIES}
# )

# only works on ubuntu 18.04
# set_target_properties(HiggsDriver PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS ON
# )

# if(CMAKE_COMPILER_IS_GNUCXX)
#     add_definitions(--std=c++1z)
# endif()

add_custom_target(dump_info COMMAND echo ${MY_DEVICE_LIBRARIES} | sed 's/ /\\n/g' )
#add_custom_target(dump_info COMMAND echo ${ZEROMQ_LIBRARY} && echo ${Boost_LIBRARIES} | sed 's/ /\\n/g' )

add_custom_target(setup_link_dir COMMAND sudo mkdir -p /usr/local/lib/SoapySDR/modules0.7/ && sudo chown `whoami`. /usr/local/lib/SoapySDR/modules0.7/)
add_custom_target(install_link_user COMMAND rm -f /usr/local/lib/SoapySDR/modules0.7/libHiggsDriver.so && ln -s `realpath libHiggsDriver.so` /usr/local/lib/SoapySDR/modules0.7/libHiggsDriver.so)

add_custom_target(install_link COMMAND sudo mkdir -p /usr/local/lib/SoapySDR/modules0.7/ && sudo ln -s `realpath libHiggsDriver.so` /usr/local/lib/SoapySDR/modules0.7/libHiggsDriver.so)
add_custom_target(uninstall_link COMMAND sudo rm -f /usr/local/lib/SoapySDR/modules0.7/libHiggsDriver.so)


# add_custom_target(catch_bad_link ALL echo "Strings should match, if not, run:      make install_link" && echo "" && realpath libHiggsDriver.so && readlink -f /usr/local/lib/SoapySDR/modules0.7/libHiggsDriver.so && echo "" && echo "" && echo ""
#                   WORKING_DIRECTORY .
#                   COMMENT comment "Checking link"
#                   )




