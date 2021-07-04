#pragma once

// this file is meant to be included by all files in the driver
// but not by users of the driver

#include "ringbus.h"
#include "ringbus2_pre.h"
#define OUR_RING_ENUM RING_ENUM_PC
#include "ringbus2_post.h"


#include "deprecate_all_ringbus.h"

#include "sig_branch_prediction.h"
