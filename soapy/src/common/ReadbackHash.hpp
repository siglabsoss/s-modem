#pragma once

#include <stdint.h>

namespace siglabs {
namespace hash {

uint32_t hashReadback(const uint32_t ringbus_ttl, const uint32_t word);

}
}