
####################################################################
#
#  This is how you build programs using the driver above
#
#  Because this is a separate project all settings need to be defined,
#  resulting in some things being written twice

# start a new project
cmake_minimum_required(VERSION 2.6)
project(ModemMain CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("find_zmq.cmake")
FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )



# add some custom CXX flags.  (this is the official way to do a += in cmake)
# FIXME remove -Wno-return-type
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

set(MODEM_SOURCE_FILES

    src/common/utilities.hpp
    src/common/utilities.cpp
    src/common/discovery.cpp
    
)

add_definitions(-DVERILATE_TESTBENCH)

include_directories(
src/
${RISCV_BASEBAND_REPO}/verilator/inc/
)


list(APPEND COMMON_LINK_LIBS
	${SOAPY_SDR_LIBRARY}
	m 
	${ZEROMQ_LIBRARY}
		${Boost_LIBRARIES}
	-lboost_system
	-lboost_random
	-pthread
	)

## Uncomment to list all CMAKE variables, very helpful

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

add_executable(modem_main src/modem_main.cpp ${MODEM_SOURCE_FILES})
target_link_libraries(modem_main ${COMMON_LINK_LIBS} HiggsDriver SoapySDR)

if(BUILD_EXTRA)

#add_executable(modem_floats src/modem_floats.cpp ${MODEM_SOURCE_FILES})
#target_link_libraries(modem_floats ${COMMON_LINK_LIBS})

#add_executable(zmq_test_1 src/test_zmq.cpp ${MODEM_SOURCE_FILES})
#target_link_libraries(zmq_test_1 ${COMMON_LINK_LIBS})

add_executable(discovery_server src/discovery_server.cpp ${MODEM_SOURCE_FILES})
target_link_libraries(discovery_server ${COMMON_LINK_LIBS})

add_executable(discovery_client src/discovery_client.cpp ${MODEM_SOURCE_FILES})
target_link_libraries(discovery_client ${COMMON_LINK_LIBS})

endif()