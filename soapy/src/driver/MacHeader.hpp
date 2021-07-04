#pragma once

#include <cstdint>
#include <utility>
#include "crc32.hpp"

#define PARAM_LENGTH_BIT_MAP   0xFFFFF000
#define PARAM_SEQ_NUM_BIT_MAP  0x00000FFF
#define PARAM_FROM_BIT_MAP     0x0FF00000
#define PARAM_FLAGS_BIT_MAP    0x000FF000
#define PARAM_CHECKSUM_BIT_MAP 0x00000FFF
#define HEADER_WORD_SIZE       0x2

class MacHeader
{
public:
    siglabs::crc::Crc32 checksum_32;
    std::vector<uint32_t> header = std::vector<uint32_t>(HEADER_WORD_SIZE);
    /**
     *
     */
    MacHeader();

    /**
     *
     */
    ~MacHeader();
    
    /**
     *
     */
    void create_header(uint32_t &upper_header,
                       uint32_t &lower_header,
                       uint32_t length,
                       uint16_t seq_num,
                       uint8_t from,
                       uint8_t flags);

    /**
     *
     */
    bool unpack_header(uint32_t upper_header,
                       uint32_t lower_header,
                       uint32_t &length,
                       uint16_t &seq_num,
                       uint8_t &from,
                       uint8_t &flags);

    /**
     *
     */
    std::pair<bool, uint64_t> vote_header(std::vector<uint32_t> &header_array);

    /**
     *
     */
    void convert_air_header(uint32_t length,
                            uint8_t seq_num,
                            uint8_t flags,
                            std::tuple<uint32_t, uint8_t, uint8_t> &air_header);

    /**
     *
     */
    bool valid_checksum(uint64_t header_64);

    /**
     *
     */
    bool valid_checksum(uint32_t upper_header, uint32_t lower_header);
};
