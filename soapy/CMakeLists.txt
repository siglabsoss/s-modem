cmake_minimum_required(VERSION 2.6)

# Set paths, these are relative to the folder which contains CMakeLists.txt, not build/
set(Q_ENGINE_REPO       "../../q-engine")
set(IP_LIBRARY_REPO     "../../ip-library")
set(RISCV_BASEBAND_REPO "../../riscv-baseband")
set(DATAPATH_REPO       "../../datapath")
set(HIGGS_ROOT          "../../..")
set(TEST_DIR            src/test)
set(3RD_DIR             src/3rd)
set(COMMON_DIR          src/common)
set(DRIVER_DIR          src/driver)

#
# Included scripts do automatic cmake_policy PUSH and POP.
# 
# In CMake 2.6.2 and below, CMake Policy settings in scripts loaded by
# the include() and find_package() commands would affect the includer.
# Explicit invocations of cmake_policy(PUSH) and cmake_policy(POP) were
# required to isolate policy changes and protect the includer.  While
# some scripts intend to affect the policies of their includer, most do
# not.  In CMake 2.6.3 and above, include() and find_package() by
# default PUSH and POP an entry on the policy stack around an included
# script, but provide a NO_POLICY_SCOPE option to disable it.  This
# policy determines whether or not to imply NO_POLICY_SCOPE for
# compatibility.  The OLD behavior for this policy is to imply
# NO_POLICY_SCOPE for include() and find_package() commands.  The NEW
# behavior for this policy is to allow the commands to do their default
# cmake_policy PUSH and POP.
#
# see https://github.com/Kitware/CMake/blob/master/Tests/PolicyScope/CMakeLists.txt 
cmake_policy(SET CMP0011 NEW)



################
#
#  Will build tests and other stuff
option(BUILD_EXTRA "Build non essential binaries" OFF)


################
#
#  Include each of our cmake projects.
#  Each file here declares it's own project() near the head of the file
#
#  The driver .cmake uses some of SOAPY's helpers to build the binary
#  The modem .cmake has 1 or more binaries that interact with the driver
#  Side control is a small json-rpc only binary.
#
#  As we add binaries the idea is to group them in with binaries with similar
#  requirements.  1 project per .cmake

include("higgs_driver.cmake")
include("modem.cmake")
include("unit_test.cmake")
# include("test_libevent.cmake")