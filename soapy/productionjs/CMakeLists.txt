cmake_minimum_required(VERSION 2.6)
set (CMAKE_CXX_STANDARD 11)
project (sjs)


set(Q_ENGINE_REPO       "../../../q-engine")
set(IP_LIBRARY_REPO     "../../../ip-library")
set(RISCV_BASEBAND_REPO "../../../riscv-baseband")
set(DATAPATH_REPO       "../../../datapath")
set(HIGGS_ROOT          "../../../..")
set(TEST_DIR            ../src/test)
set(3RD_DIR             ../src/3rd)
set(COMMON_DIR          ../src/common)
set(DRIVER_DIR          ../src/driver)
set(BIND_DIR            ../src/bindjs)

message(STATUS ${PROJECT_SOURCE_DIR})


set(CMAKE_CXX_FLAGS
"${CMAKE_CXX_FLAGS} \
-Werror=return-type -Wall -Wextra -std=gnu++11 \
-Wvla -Wcast-qual -Wdangling-else -Winit-self -Werror=uninitialized \
-Werror=incompatible-pointer-types -Werror=array-bounds \
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
-Wmissing-field-initializers \
-Wcast-align \
-Wduplicated-branches \
-Wfloat-equal \
-Wformat \
-Wformat-security \
-Wformat-signedness \
-Woverlength-strings \
-Wredundant-decls \
-Wsign-promo \
-Wsynth \
-Wtrampolines \
-Wvariadic-macros \
-Wvirtual-inheritance \
"
)



FIND_PACKAGE( Boost 1.40 COMPONENTS program_options system random REQUIRED )
include("../find_zmq.cmake")
include("../find_libevent.cmake")
include("../find_higgs_driver.cmake")




set(SOURCE_FILES
    ${BIND_DIR}/BindJs.cpp

    ${BIND_DIR}/BindData.hpp
    ${BIND_DIR}/BindData.cpp

    ${BIND_DIR}/BindSchedule.hpp
    ${BIND_DIR}/BindSchedule.cpp

    ${BIND_DIR}/BindFlushHiggs.hpp
    ${BIND_DIR}/BindFlushHiggs.cpp

    ${BIND_DIR}/BindEventDsp.hpp
    ${BIND_DIR}/BindEventDsp.cpp

    ${BIND_DIR}/BindRadioEstimate.hpp
    ${BIND_DIR}/BindRadioEstimate.cpp

    ${BIND_DIR}/BindSettings.hpp
    ${BIND_DIR}/BindSettings.cpp

    ${BIND_DIR}/BindCustomEvent.hpp
    ${BIND_DIR}/BindCustomEvent.cpp

    ${BIND_DIR}/BindTxSchedule.hpp
    ${BIND_DIR}/BindTxSchedule.cpp

    ${BIND_DIR}/BindHiggsDriver.hpp
    ${BIND_DIR}/BindHiggsDriver.cpp

    ${BIND_DIR}/BindAirPacket.hpp
    ${BIND_DIR}/BindAirPacket.cpp

    ${BIND_DIR}/BindDemodThread.hpp
    ${BIND_DIR}/BindDemodThread.cpp

    ${BIND_DIR}/BindEVMThread.hpp
    ${BIND_DIR}/BindEVMThread.cpp

    ${BIND_DIR}/BindHiggsSNR.hpp
    ${BIND_DIR}/BindHiggsSNR.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

include_directories(
${CMAKE_SOURCE_DIR}/node_modules/node-addon-api
../src/driver
../src/bindjs
../src/common
../src
../../../../libs/riscv-baseband/verilator/inc
../../../../libs/riscv-baseband/c/inc
${RISCV_BASEBAND_REPO}/c/inc
${HIGGS_ROOT}/sim/verilator/inc
${Boost_INCLUDE_DIR}
${CMAKE_JS_INC}
${3RD_DIR}/schifra
)

# message(STATUS "Foo bar")
# message(STATUS ${Boost_LIBRARIES})
# message(STATUS ${PROJECT_NAME})

# link_directories(${CMAKE_SOURCE_DIR}/obj_dir)
# target_include_directories(${PROJECT_NAME}
#   PRIVATE ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api
#   PRIVATE ${VERPATH}/include
#   PRIVATE ${VERPATH}/include/vltstd
#   PRIVATE ${VERPATH}/include/gtkwave
#   PRIVATE ./obj_dir
#   PRIVATE ${CMAKE_JS_INC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME}
${CMAKE_JS_LIB}
${ZEROMQ_LIBRARY}
${LIBEVENT_LIBRARIES}
${Boost_LIBRARIES}
/usr/local/lib/libSoapySDR.so
${HIGGS_DRIVER_LIBRARY}
-pthread
)
#/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/build/libHiggsDriverStatic.a

