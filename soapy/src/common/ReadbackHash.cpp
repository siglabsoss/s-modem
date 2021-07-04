#include "ReadbackHash.hpp"
#include "random.h"
#include "ringbus.h"

namespace siglabs {
namespace hash {


uint32_t hashReadback(const uint32_t us, const uint32_t word) {
    uint32_t hash;
    hash = xorshift32(1, &us, 1);
    hash = xorshift32(hash, &word, 1);

    uint32_t readback = READBACK_HASH_PCCMD | ((us&0xf)<<20) | (hash&0xfffff);

    return readback;
}


}
}