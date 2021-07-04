#include <iostream>
#include <rapidcheck.h>
#include <utility>
#include "driver/MacHeader.hpp"

#define HEADER_SIZE_20         20
#define HEADER_SIZE_40         40
#define MAX_CORRUPT_WORDS      10

/**
 *
 */
bool test_header_packing();

/**
 *
 */
bool test_header_unpacking();

/**
 *
 */
bool test_header_voting();

int main(int argc, char const *argv[])
{
    bool pass = true;
    pass &= test_header_packing();
    pass &= test_header_unpacking();
    pass &= test_header_voting();

    return !pass;
}

bool test_header_packing() {
    MacHeader mac_header;
    siglabs::crc::Crc32 checksum_32;
    std::vector<uint32_t> header(HEADER_WORD_SIZE);
    return rc::check("Test Header Packing",
                     [&mac_header, &checksum_32, &header]() {
        uint32_t upper_header;
        uint32_t lower_header;
        uint32_t checksum;
        uint16_t get_seq_num;
        uint8_t get_from;
        uint8_t get_flags;
        uint32_t get_checksum;
        uint32_t length = *rc::gen::inRange(0u, 0xFFFFFu);
        uint16_t seq_num = *rc::gen::inRange(0u, 0xFFFFu);
        uint8_t from = *rc::gen::inRange(0u, 0xFFu);
        uint8_t flags = *rc::gen::inRange(0u, 0xFFu);

        mac_header.create_header(upper_header,
                                 lower_header,
                                 length,
                                 seq_num,
                                 from,
                                 flags);
        header[0] = upper_header;
        header[1] = lower_header & ~PARAM_CHECKSUM_BIT_MAP;
        checksum = checksum_32.calc(header) & PARAM_CHECKSUM_BIT_MAP;
        get_seq_num = (((upper_header & PARAM_SEQ_NUM_BIT_MAP) << 4) |
                       (lower_header >> 28));
        get_from = ((lower_header & PARAM_FROM_BIT_MAP) >> 20);
        get_flags = (lower_header & PARAM_FLAGS_BIT_MAP) >> 12;
        get_checksum = lower_header & PARAM_CHECKSUM_BIT_MAP;

        RC_ASSERT(!((upper_header ^ (length << 12)) & PARAM_LENGTH_BIT_MAP));
        RC_ASSERT(seq_num == get_seq_num);
        RC_ASSERT(from == get_from);
        RC_ASSERT(flags == get_flags);
        RC_ASSERT(checksum == get_checksum);
    });
}

bool test_header_unpacking() {
    MacHeader mac_header;
    return rc::check("Test Header Unpacking", [&mac_header]() {
        uint32_t upper_header;
        uint32_t lower_header;
        uint32_t length = *rc::gen::inRange(0u, 0xFFFFFu);
        uint16_t seq_num = *rc::gen::inRange(0u, 0xFFFFu);
        uint8_t from = *rc::gen::inRange(0u, 0xFFu);
        uint8_t flags = *rc::gen::inRange(0u, 0xFFu);
        bool correct_checksum = false;

        mac_header.create_header(upper_header,
                                 lower_header,
                                 length,
                                 seq_num,
                                 from,
                                 flags);
        correct_checksum = mac_header.unpack_header(upper_header,
                                                    lower_header,
                                                    length,
                                                    seq_num,
                                                    from,
                                                    flags);
        RC_ASSERT(correct_checksum);
    });
}

bool test_header_voting() {
    MacHeader mac_header;
    siglabs::crc::Crc32 checksum_32;
    std::vector<uint32_t> header(HEADER_WORD_SIZE);
    return rc::check("Test Header Voting",
                     [&mac_header, &checksum_32, &header]() {
        uint32_t checksum;
        uint64_t header_64;
        uint32_t header_size = *rc::gen::element(HEADER_SIZE_20,
                                                 HEADER_SIZE_40);
        uint32_t upper_header = *rc::gen::inRange(0u, 0xFFFFFFFFu);
        uint32_t lower_header = *rc::gen::inRange(0u, 0xFFFFFFFFu);
        uint32_t corrupt = *rc::gen::inRange(0, MAX_CORRUPT_WORDS);
        std::vector<uint32_t> header_array;
        std::pair<bool, uint64_t> output(false, 0);
        header[0] = upper_header;
        header[1] = lower_header & ~PARAM_CHECKSUM_BIT_MAP;
        checksum = checksum_32.calc(header) & PARAM_CHECKSUM_BIT_MAP;
        lower_header = PARAM_CHECKSUM_BIT_MAP & (lower_header | checksum);
        header_64 = (uint64_t) upper_header << 32 | lower_header;
        // Adding corrupt data
        for (std::size_t i = 0; i < corrupt; i++) {
            header_array.push_back(*rc::gen::inRange(0u, 0xFFFFFFFFu));
        }
        if (corrupt % 2) {
            header_array.push_back(lower_header);
            corrupt++;
        }
        // Add correct header
        for (std::size_t j = corrupt; j < header_size; j+=2) {
            header_array.push_back(upper_header);
            header_array.push_back(lower_header);
        }
        output = mac_header.vote_header(header_array);

        if (corrupt > 4) {
            RC_ASSERT(!output.first);
        } else {
            RC_ASSERT(output.first);
            RC_ASSERT(output.second == header_64);
        }
    });
}
