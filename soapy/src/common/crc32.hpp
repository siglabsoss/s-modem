#pragma once

#include <stdint.h>
#include <cstddef>
#include <vector>

namespace siglabs {
namespace crc {

class Crc32
{
public:
    Crc32();
    uint32_t table[256];
    constexpr static uint32_t initial_value = 0xffffffff;

    uint32_t calc(const void* buf, const size_t len) const;
    uint32_t calc(const std::vector<unsigned char>& v) const;
    uint32_t calc(const std::vector<uint32_t>& v) const;
};

}
}
