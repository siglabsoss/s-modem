#include <iostream>
#include <rapidcheck.h>
#include <chrono>
#include "driver/ReedSolomon.hpp"

#define BITS_PER_BYTE (8.0)
#define BITS_PER_MEGABIT (1048576.0)
/*
 * This method populates a standard vector with random or counter data
 *
 * Args:
 *     input_data: Vector to populate data
 *     data_length: The amount of data to populate
 *     random: A boolean of whether the data should be random
 */
void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random);

/**
 *
 */
void test_block_decode_speed(std::size_t code_length, std::size_t fec_length);

/**
 *
 */
void test_message_decode_speed(std::size_t code_length,
                               std::size_t fec_length,
                               uint32_t max_length);

/**
 *
 */
void _print_decode_speed(std::size_t code_length,
                         std::size_t fec_length,
                         double block_count,
                         std::vector<double> &decode_time);

int main(int argc, char const *argv[]) {
    std::size_t code_length = 255;
    std::size_t fec_length = 32;
    uint32_t max_length = 0xffffu;

    std::vector<std::size_t> all_fec_length{/*FEC_LEN_32,*/
                                            FEC_LEN_48};
    if (argc == 1) {
        for (std::size_t fec_length : all_fec_length) {
            test_block_decode_speed(code_length, fec_length);
        }
    } else {
        for (int i = 1; i < argc; i++) {
            std::cout << "\nTest Reed Solomon Running Test #" << *argv[i]
                      << "\n";
            std::cout << "FEC Length: " << fec_length
                      << " Code Length: " << code_length << "\n";
            switch (*argv[i]) {
                case '1':
                    test_block_decode_speed(code_length, fec_length);
                    break;
                case '2':
                    test_message_decode_speed(code_length,
                                              fec_length,
                                              max_length);
                    break;
                default:
                    break;
            }
        }
    }

    return 0;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random) {
    uint32_t data;
    for(uint32_t i = 0; i < data_length; i++) {
        data = random ?  *rc::gen::inRange(0u, 0xffffffffu) : i;
        input_data.push_back(data);
    }
}

void test_block_decode_speed(std::size_t code_length, std::size_t fec_length) {
    std::vector<double> decode_time;
    rc::check("Test RS block decoding speed", [code_length,
                                               fec_length,
                                               &decode_time]() {
        bool encode_successful = false;
        bool decode_successful = false;
        std::size_t block_size = (code_length - fec_length) / 4;
        std::vector<uint32_t> in_block;
        std::vector<uint32_t> encoded_block;
        std::vector<uint32_t> decoded_block;
        ReedSolomon reed_solomon(code_length, fec_length);

        generate_input_data(in_block, block_size, true);
        encode_successful = reed_solomon.encode_block(in_block,
                                                      encoded_block,
                                                      code_length,
                                                      fec_length);
        RC_ASSERT(encode_successful);
        auto start = std::chrono::steady_clock::now();
        decode_successful = reed_solomon.decode_block(encoded_block,
                                                      decoded_block,
                                                      code_length,
                                                      fec_length);
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> decode_duration = end - start;
        decode_time.push_back(decode_duration.count());

        RC_ASSERT(decode_successful);
        RC_ASSERT(in_block == decoded_block);
    });
    _print_decode_speed(code_length,
                        fec_length,
                        decode_time.size(),
                        decode_time);
}

void test_message_decode_speed(std::size_t code_length,
                               std::size_t fec_length,
                               uint32_t max_length) {
    const std::size_t word_count = (code_length / 4) + ((code_length % 4)?1:0);
    double block_count = 0;
    std::vector<double> decode_time;
    rc::check("Test RS message decoding", [code_length,
                                           fec_length,
                                           max_length,
                                           &block_count,
                                           &decode_time,
                                           word_count]() {
        bool encode_successful = false;
        bool decode_successful = false;
        std::size_t data_length;
        std::vector<uint32_t> in_message;
        std::vector<uint32_t> temp;
        std::vector<uint32_t> encoded_message;
        std::vector<uint32_t> decoded_message;
        ReedSolomon reed_solomon(code_length, fec_length);

        data_length = *rc::gen::inRange(0x1u, max_length);
        generate_input_data(in_message, data_length, true);
        temp = in_message;
        encode_successful = reed_solomon.encode_message(temp,
                                                        encoded_message,
                                                        code_length,
                                                        fec_length);

        RC_ASSERT(encode_successful);
        const std::size_t decode_count = encoded_message.size() / word_count;
        block_count += decode_count;
        auto start = std::chrono::steady_clock::now();
        decode_successful = reed_solomon.decode_message(encoded_message,
                                                        decoded_message,
                                                        code_length,
                                                        fec_length);
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> decode_duration = end - start;
        decode_time.push_back(decode_duration.count());
        RC_ASSERT(decode_successful);
        RC_ASSERT(in_message == decoded_message);
    });
    _print_decode_speed(code_length,
                        fec_length,
                        block_count,
                        decode_time);
}

void _print_decode_speed(std::size_t code_length,
                         std::size_t fec_length,
                         double block_count,
                         std::vector<double> &decode_time) {
    double total_time = 0;
    double data_length = code_length - fec_length;
    double mbps;

    for (double time : decode_time) total_time += time;
    mbps = (block_count * data_length * BITS_PER_BYTE) /
           (total_time * BITS_PER_MEGABIT);

    std::cout << "Codec: RS("
              << code_length << ", "
              << code_length - fec_length
              << ", "
              << fec_length << ") "
              << "Blocks decoded: " << block_count << " "
              << "Time: " << total_time << " sec "
              << "Rate: " << mbps << " Mbps\n";
}
