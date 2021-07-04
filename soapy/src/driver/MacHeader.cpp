#include "MacHeader.hpp"
#include <iostream>
#include <map>

MacHeader::MacHeader() {

}

MacHeader::~MacHeader() {

}

void MacHeader::create_header(uint32_t &upper_header,
                              uint32_t &lower_header,
                              uint32_t length,
                              uint16_t seq_num,
                              uint8_t from,
                              uint8_t flags) {
    uint32_t checksum;
    length = length << 12;
    upper_header = (length & PARAM_LENGTH_BIT_MAP) | (seq_num >> 4);
    lower_header = (seq_num << 28) |
                   (from << 20) |
                   (flags << 12);
    header[0] = upper_header;
    header[1] = lower_header;
    checksum = checksum_32.calc(header);
    lower_header |= (checksum & PARAM_CHECKSUM_BIT_MAP);
}

bool MacHeader::unpack_header(uint32_t upper_header,
                              uint32_t lower_header,
                              uint32_t &length,
                              uint16_t &seq_num,
                              uint8_t &from,
                              uint8_t &flags) {
    uint32_t checksum;

    length = (upper_header & PARAM_LENGTH_BIT_MAP) >> 12;
    seq_num = ((upper_header & PARAM_SEQ_NUM_BIT_MAP) << 4) |
              (lower_header >> 28);
    from = (lower_header & PARAM_FROM_BIT_MAP) >> 20;
    flags = (lower_header & PARAM_FLAGS_BIT_MAP) >> 12;

    return valid_checksum(upper_header, lower_header);
}

std::pair<bool, uint64_t> MacHeader::vote_header(
                          std::vector<uint32_t> &header_array) {
    std::map<uint64_t, uint32_t> header_count;
    std::pair<bool, uint64_t> output(false, 0);
    uint32_t min_vote = header_array.size()/2 - 2;

    for (std::size_t i = 0; i < header_array.size(); i += 2) {
        uint64_t header_64 = ((uint64_t) header_array[i] << 32) |
                             header_array[i + 1];
        if (header_count.count(header_64)) header_count[header_64] += 1;
        else header_count[header_64] = 1;
        if (header_count[header_64] >= min_vote) {
            output.first = true;
            output.second = header_64;
            return output;
        }
    }

    return output;
}

void MacHeader::convert_air_header(
                        uint32_t length,
                        uint8_t seq_num,
                        uint8_t flags,
                        std::tuple<uint32_t, uint8_t, uint8_t> &air_header) {
    std::get<0>(air_header) = length;
    std::get<1>(air_header) = seq_num;
    std::get<2>(air_header) = flags;
}

bool MacHeader::valid_checksum(uint64_t header_64) {
    uint32_t upper_header = header_64 >> 32;
    uint32_t lower_header = header_64;

    return valid_checksum(upper_header, lower_header);
}

bool MacHeader::valid_checksum(uint32_t upper_header, uint32_t lower_header) {
    uint32_t checksum;

    header[0] = upper_header;
    header[1] = lower_header & (~PARAM_CHECKSUM_BIT_MAP);
    checksum = checksum_32.calc(header);

    if ((checksum & PARAM_CHECKSUM_BIT_MAP) !=
        (lower_header & PARAM_CHECKSUM_BIT_MAP)) return false;

    return true;
}
