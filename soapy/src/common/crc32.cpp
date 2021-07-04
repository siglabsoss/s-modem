#include "crc32.hpp"


namespace siglabs {
namespace crc {

using namespace std;


static void generate_table(uint32_t(&table)[256])
{
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) 
    {
        uint32_t c = i;
        for (size_t j = 0; j < 8; j++) 
        {
            if (c & 1) {
                c = polynomial ^ (c >> 1);
            }
            else {
                c >>= 1;
            }
        }
        table[i] = c;
    }
}

static uint32_t update(const uint32_t (&table)[256], const uint32_t initial, const void* buf, const size_t len)
{
    uint32_t c = initial ^ 0xFFFFFFFF;
    const uint8_t* u = static_cast<const uint8_t*>(buf);
    for (size_t i = 0; i < len; ++i) 
    {
        c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}



Crc32::Crc32() {
    generate_table(table);
}

uint32_t Crc32::calc(const void* buf, const size_t len) const {
    return update(table, initial_value, buf, len);
}

uint32_t Crc32::calc(const std::vector<unsigned char>& v) const {
    return calc(v.data(), v.size());
}

uint32_t Crc32::calc(const std::vector<uint32_t>& v) const {
    return calc(v.data(), v.size()*4);
}


}
}
