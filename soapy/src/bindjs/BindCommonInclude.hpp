#pragma once

#include "NapiHelper.hpp"

#include <iostream>
#include <string.h>
#include <iostream>
#include <unistd.h> //usleep
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <set>
#include <thread>

// callback stuff
#include "CToJavascript.hpp"
#include <functional>

// copied from modem_main
// #include <boost/program_options.hpp>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <complex.h>
#undef I
#include <iostream>
#include <stdint.h>

#define SIG_UTILS_DO_NOT_DEFINE_NULL
#include "HiggsSoapyDriver.hpp"
#include "common/utilities.hpp"