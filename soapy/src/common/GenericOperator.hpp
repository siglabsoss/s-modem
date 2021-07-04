#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>

typedef std::function<void(const uint32_t, const uint32_t)> op_get_cb_t;

namespace siglabs {
namespace rb {

std::vector<uint32_t> op(const std::string s, const uint32_t _sel, const uint32_t _val = 0, const uint32_t _rb_type=0);

class DispatchGeneric {
public:
    op_get_cb_t cb = 0;
    uint32_t mask = 0;
    void gotWord(const uint32_t);
    void setCallback(op_get_cb_t, uint32_t);
private:
    void gotMatchingWord(const uint32_t);
    uint32_t schedule_epoc_storage = 0;
    int32_t schedule_epoc_storage_op = -1;
    constexpr static bool print = false;
};


}
}