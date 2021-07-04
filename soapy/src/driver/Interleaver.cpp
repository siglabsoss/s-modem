#include <iostream>
#include <cstring>
#include "Interleaver.hpp"



Interleaver::Interleaver(const size_t col_length)
        :  
          required_stream_size(calculate_words_for_code(col_length)) 
          ,code_length(required_stream_size){

    block_interleaver.reserve(M_ROWS);
    std::vector<uint8_t> block_data;
    block_data.resize(code_length);
    for (size_t i = 0; i < M_ROWS; i++) {
        block_interleaver.push_back(block_data);
    }
}

Interleaver::~Interleaver() {

}


bool Interleaver::length_ok(const size_t sz) const {
    return sz == required_stream_size;
}

size_t Interleaver::calculate_words_for_code(const size_t c_len) {
    const size_t stream_size = 
        (c_len % 4) ? ((c_len / 4) + 1) * 4 : c_len;
    return stream_size;
}

size_t Interleaver::calculate_blocks_for_length(const size_t len, const size_t c_len) {
    const size_t req = calculate_words_for_code(c_len);
    const size_t blocks = 
        (len % req) ? ((len / req) + 1) : (len/req);
    return blocks;
}



bool Interleaver::inplace_interleave_block(std::vector<uint32_t> &data_stream) {
    size_t sz = data_stream.size();
    if (sz != required_stream_size) {
        if( print_on_error ) {
            std::cout << "ERROR: Data stream should have " << required_stream_size
                      << " words of data. Data stream currently has "
                      << sz << " words of data\n";
        }
        return false;
    }
    load_block_interleaver(data_stream, sz);
    for (size_t i = 0; i < code_length; i++) {
        uint32_t interleaved_word = block_interleaver[0][i] |
                                    block_interleaver[1][i] << 8 |
                                    block_interleaver[2][i] << 16 |
                                    block_interleaver[3][i] << 24;
        data_stream[i] = interleaved_word;
    }
    return true;
}

bool Interleaver::inplace_deinterleave_block(std::vector<uint32_t> &data_stream) {
    size_t sz = data_stream.size();
    if (sz != required_stream_size) {
        if( print_on_error ) {
            std::cout << "ERROR: Data stream should have " << required_stream_size
                      << " words of data. Data stream currently has "
                      << sz << " words of data\n";
        }
        return false;
    }
    load_block_deinterleaver(data_stream);

    // Copy data from block interleaver back into data stream vector
    for (size_t i = 0; i < M_ROWS; i++) {
        std::memcpy(data_stream.data() + (i * (sz / 4)),
                    block_interleaver[i].data(),
                    code_length);
    }

    // If code length is not a multiple of 4, the upper significant bytes need
    // to be corrected
    if (code_length % 4) {
        uint32_t mask = (0xFFFFFFFF >> ((4 - (code_length % 4)) * 8));
        for (size_t i = 1; i <= M_ROWS; i++) {
            data_stream[(sz / 4) * i - 1] &= mask; 
        }
    }
    return true;
}

bool Interleaver::interleave(
    const std::vector<uint32_t> &input,
    std::vector<uint32_t> &output) {

    output.resize(0);
    output.reserve(input.size() + required_stream_size);

    std::vector<uint32_t> modify;
    modify.reserve(required_stream_size);

    const size_t szin = input.size();
    const size_t blocks_used = calculate_blocks_for_length(szin, required_stream_size);

    // std::cout << "using " << blocks_used << " blocks\n";

    size_t read_start = 0;
    size_t read_end = std::min(szin, required_stream_size); // without this first block can read uninitialized memory

    for(size_t i = 0; i < blocks_used; i++) {

        // on the last loop this read_end may be a bit short
        modify.assign(
            input.begin() + read_start,
            input.begin() + read_end
        );

        // because read_end may be short, make sure we always have the correct input
        // this will either pad zeros or do nothing
        modify.resize(required_stream_size, INTERLEAVE_REMOVE_KEY);

// #define HEX32_STRING(n) setfill('0') << setw(8) << std::hex << (n) << std::dec
//         std::cout << " loading:         ";
//         for(const auto w : modify) {
//             std::cout << w << ",";
//         }
//         std::cout << "\n";


        const bool pass = inplace_interleave_block(modify);
        if(!pass) {
            std::cout << "interleave() should never fail, inplace_interleave_block() had internal error\n";
            return false;
        }

        for(const auto w : modify) {
            output.push_back(w);
        }


        read_start += required_stream_size;
        read_end += required_stream_size;
        read_end = std::min(read_end, szin); // cap, should only run before we enter the last loop
    }

    return true;
}

bool Interleaver::deinterleave(
    const std::vector<uint32_t> &input,
    std::vector<uint32_t> &output
    ) {

        output.resize(0);
        output.reserve(input.size());

        std::vector<uint32_t> modify;
        modify.reserve(required_stream_size);

        const size_t szin = input.size();

        if( szin % required_stream_size ) {
            if( print_on_error ) {
                std::cout << "ERROR: deinterleave() should have multiple of " << required_stream_size
                          << " words of data. Data stream currently has "
                          << szin << " words of data off by " << (szin % required_stream_size) << "!\n";
            }
            return false;
        }

        const size_t blocks_used = szin / required_stream_size;

        // std::cout << "using " << blocks_used << " blocks\n";

        size_t read_start = 0;
        size_t read_end = required_stream_size;

        for(size_t i = 0; i < blocks_used; i++) {

            // should always be exact multiple
            modify.assign(
                input.begin() + read_start,
                input.begin() + read_end
            );

            // SHOULD ALWAYS DO NOTHING
            // modify.resize(required_stream_size, INTERLEAVE_REMOVE_KEY);

            // for(const auto w : modify) {
            //     std::cout << w << ",";
            // }
            // std::cout << "\n";


            const bool pass = inplace_deinterleave_block(modify);
            if(!pass) {
                std::cout << "interleave() should never fail, inplace_interleave_block() had internal error\n";
                return false;
            }

            for(const auto w : modify) {
                output.push_back(w);
            }

            read_start += required_stream_size;
            read_end += required_stream_size;


            // if( std::min(read_end, szin) != read_end ) {
            //     std::cout << "read_end " << read_end << " szin " << szin << "\n";
            //     std::cout << "deinterleave() must have even blocks\n";
            //     return false;
            // }
            // read_end = std::min(read_end, szin); // cap, should only run before we enter the last loop
        }

        if( auto_remove_tail ) {
            unsigned j = 0;
            for (auto i = output.rbegin(); i != output.rend(); ++i ) { 
                if(*i == INTERLEAVE_REMOVE_KEY) {
                    j++;
                } else {
                    break;
                }
            }

            if( j != 0) {
                output.resize(output.size()-j);
            }
        }

        // if(truncate != 0) {
            // output.resize(truncate);
        // }


    return true;
}

void Interleaver::load_block_interleaver(const std::vector<uint32_t> &data_stream,
                                         const size_t stream_size) {
    for (size_t i = 0; i < M_ROWS; i++) {
        std::memcpy(block_interleaver[i].data(),
                    data_stream.data() + (i * (stream_size / 4)),
                    code_length);
    }
}

void Interleaver::load_block_deinterleaver(const std::vector<uint32_t> &data_stream) {
    for (size_t i = 0; i < code_length; i++) {
        block_interleaver[0][i] = (data_stream[i]      )&0xFF;
        block_interleaver[1][i] = (data_stream[i] >>  8)&0xFF;
        block_interleaver[2][i] = (data_stream[i] >> 16)&0xFF;
        block_interleaver[3][i] = (data_stream[i] >> 24)&0xFF;
    }
}

void Interleaver::print_block_interleaver() const {
    for (size_t i = 0; i < M_ROWS; i++) {
        std::cout << "Block " << i << ":\n[";
        for (size_t j = 0; j < code_length; j++) {
            if (j == code_length - 1) {
                std::cout << (uint32_t) block_interleaver[i][j];
            } else{
                std::cout << (uint32_t) block_interleaver[i][j] << ", ";
            }
        }
        std::cout << "] " << "(len = " << code_length << ")\n";
    }
}