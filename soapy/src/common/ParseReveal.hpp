#pragma once

#include <string>
#include <vector>

namespace siglabs {
namespace reveal {


std::vector<std::vector<uint32_t>> parse_reveal_dump(std::string filename, const unsigned header_lines = 4, const size_t signals = 8, const bool print = false);

}
}