#include <iostream>
#include <rapidcheck.h>
#include "driver/ReedSolomon.hpp"
#define GENERATOR_POLYNOMIAL_INDEX 120
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

/*
 * This method test the ReedSolomon block encoding method  
 *
 * Args:
 *     code_length: Code length. Current accepted value is 255
 *     fec_length: FEC length. Current accepted values are 32 and 48
 */
bool test_block_encoding(std::size_t code_length, std::size_t fec_length);

/*
 * This method test the ReedSolomon block decoding method
 *
 * Args:
 *     code_length: Code length. Current accepted value is 255
 *     fec_length: FEC length. Current accepted values are 32 and 48
 */
bool test_block_decoding(std::size_t code_length, std::size_t fec_length);

/*
 * This method test the ReedSolomon message encoding method. Message encoding
 * differ from block encoding in that the message length can be variable
 * whereas block encoding must have lengths (code_length - fec_length) bytes.
 *
 * Args:
 *     code_length: Code length. Current accepted value is 255
 *     fec_length: FEC length. Current accepted values are 32 and 48
 *     max_length: Maximum message length
 */
bool test_message_encoding(std::size_t code_length,
                           std::size_t fec_length,
                           uint32_t max_length);

/*
 * This method test the ReedSolomon message decoding method. Message decoding
 * differ from block decoding in that the message length can be variable
 * whereas block decoding must have lengths (code_length - fec_length) bytes.
 *
 * Args:
 *     code_length: Code length. Current accepted value is 255
 *     fec_length: FEC length. Current accepted values are 32 and 48
 *     max_length: Maximum message length
 */
bool test_message_decoding(std::size_t code_length,
                           std::size_t fec_length,
                           uint32_t max_length);

/*
 * This method test the ReedSolomon block puncturing method. An arbitrary mask
 * is used to determine the specific bytes to puncture.
 *
 * Args:
 *      code_length: Code length. Current accepted value is 255
 *      fec_length: FEC length. Current accepted values are 32 and 48
 */
bool test_block_puncturing(std::size_t code_length,
                           std::size_t fec_length,
                           std::size_t puncture_size);


// rc::check returns `true` on success and `false` on failure.

/*
 * Execute various Reed Solomon unit tests
 */
int main(int argc, char const *argv[])
{
    std::size_t code_length = 255;
    std::size_t fec_length = 96;
    std::size_t puncture_size = 16;
    uint32_t max_length = 0xffffu;
    std::vector<std::size_t> all_fec_length{FEC_LEN_32,
                                            FEC_LEN_48,
                                            FEC_LEN_64,
                                            FEC_LEN_80,
                                            FEC_LEN_96,
                                            FEC_LEN_128};
    bool pass = true;
    if (argc == 1) {
        for (std::size_t fec_val : all_fec_length) {
            fec_length = fec_val;
            std::cout << "\nTest Reed Solomon Running all Tests\n";
            std::cout << "FEC Length: " << fec_length
                      << " Code Length: " << code_length << "\n";
            pass &= test_block_encoding(code_length, fec_length);
            pass &= test_block_decoding(code_length, fec_length);
            pass &= test_message_encoding(code_length, fec_length, max_length);
            pass &= test_message_decoding(code_length, fec_length, max_length);
            pass &= test_block_puncturing(code_length,
                                          fec_length,
                                          puncture_size);
        }
    } else {
        for (int i = 1; i < argc; i++) {
            std::cout << "\nTest Reed Solomon Running Test #" << *argv[i]
                      << "\n";
            std::cout << "FEC Length: " << fec_length
                      << " Code Length: " << code_length << "\n";
            switch (*argv[i]) {
                case '1':
                    pass &= test_block_encoding(code_length, fec_length);
                    break;
                case '2':
                    pass &= test_block_decoding(code_length, fec_length);
                    break;
                case '3':
                    pass &= test_message_encoding(code_length,
                                                  fec_length,
                                                  max_length);
                    break;
                case '4':
                    pass &= test_message_decoding(code_length,
                                                  fec_length,
                                                  max_length);
                    break;
                case '5':
                    pass &= test_block_puncturing(code_length,
                                                  fec_length,
                                                  puncture_size);
                    break;
                default:
                    break;
            }

        }
    }

    return !pass;
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

bool test_block_encoding(std::size_t code_length, std::size_t fec_length) {
    return rc::check("Test RS block encoding", [code_length, fec_length]() {
        bool encode_successful = false;
        std::size_t block_size =  (code_length - fec_length) / 4;
        std::vector<uint32_t> in_block;
        std::vector<uint32_t> encoded_block;
        std::size_t generator_polynomial_root_count = fec_length;
        ReedSolomon reed_solomon(FIELD_DESCRIPTOR,
                                 GENERATOR_POLYNOMIAL_INDEX,
                                 generator_polynomial_root_count,
                                 code_length,
                                 fec_length);

        generate_input_data(in_block, block_size, true);
        encode_successful = reed_solomon.encode_block(in_block,
                                                      encoded_block,
                                                      code_length,
                                                      fec_length);

        RC_ASSERT(encode_successful);

    });
}

bool test_block_decoding(std::size_t code_length, std::size_t fec_length) {
    return rc::check("Test RS block decoding", [code_length, fec_length]() {
        bool encode_successful = false;
        bool decode_successful = false;
        std::size_t block_size = (code_length - fec_length) / 4;
        std::vector<uint32_t> in_block;
        std::vector<uint32_t> encoded_block;
        std::vector<uint32_t> decoded_block;
        std::size_t generator_polynomial_root_count = fec_length;
        ReedSolomon reed_solomon(FIELD_DESCRIPTOR,
                                 GENERATOR_POLYNOMIAL_INDEX,
                                 generator_polynomial_root_count,
                                 code_length,
                                 fec_length);

        generate_input_data(in_block, block_size, true);
        encode_successful = reed_solomon.encode_block(in_block,
                                                      encoded_block,
                                                      code_length,
                                                      fec_length);
        RC_ASSERT(encode_successful);
        decode_successful = reed_solomon.decode_block(encoded_block,
                                                      decoded_block,
                                                      code_length,
                                                      fec_length);
        RC_ASSERT(decode_successful);
        RC_ASSERT(in_block == decoded_block);
    });
}

bool test_message_encoding(std::size_t code_length,
                           std::size_t fec_length,
                           uint32_t max_length) {
    return rc::check("Test RS message encoding", [code_length,
                                           fec_length,
                                           max_length]() {
        bool encode_successful = false;
        std::size_t data_length;
        std::vector<uint32_t> in_message;
        std::vector<uint32_t> encoded_message;
        std::size_t generator_polynomial_root_count = fec_length;
        ReedSolomon reed_solomon(FIELD_DESCRIPTOR,
                                 GENERATOR_POLYNOMIAL_INDEX,
                                 generator_polynomial_root_count,
                                 code_length,
                                 fec_length);

        // data_length = *rc::gen::inRange(0u, max_length);
        // generate_input_data(in_message, data_length, true);
        // std::vector<uint32_t> in_message;
        for(int i = 0; i < 454; i++) {
            in_message.push_back(i*5+i);
        }
        encode_successful = reed_solomon.encode_message(in_message,
                                                        encoded_message,
                                                        code_length,
                                                        fec_length);

        RC_ASSERT(encode_successful);
    });
}

bool test_message_decoding(std::size_t code_length,
                           std::size_t fec_length,
                           uint32_t max_length) {
    return rc::check("Test RS message decoding", [code_length,
                                           fec_length,
                                           max_length]() {
        bool encode_successful = false;
        bool decode_successful = false;
        std::size_t data_length;
        std::vector<uint32_t> in_message;
        std::vector<uint32_t> temp;
        std::vector<uint32_t> encoded_message;
        std::vector<uint32_t> decoded_message;
        std::size_t generator_polynomial_root_count = fec_length;
        ReedSolomon reed_solomon(FIELD_DESCRIPTOR,
                                 GENERATOR_POLYNOMIAL_INDEX,
                                 generator_polynomial_root_count,
                                 code_length,
                                 fec_length);

        data_length = *rc::gen::inRange(0x1u, max_length);
        generate_input_data(in_message, data_length, true);
        temp = in_message;
        encode_successful = reed_solomon.encode_message(temp,
                                                        encoded_message,
                                                        code_length,
                                                        fec_length);

        RC_ASSERT(encode_successful);
        decode_successful = reed_solomon.decode_message(encoded_message,
                                                        decoded_message,
                                                        code_length,
                                                        fec_length);
        RC_ASSERT(decode_successful);
        RC_ASSERT(in_message == decoded_message);
    });
}

bool test_block_puncturing(std::size_t code_length,
                           std::size_t fec_length,
                           std::size_t puncture_size) {
    return rc::check("Test RS block puncturing", [code_length,
                                           fec_length,
                                           puncture_size]() {
        std::set<uint32_t> mask = *rc::gen::container<std::set<uint32_t>>(
                puncture_size, rc::gen::inRange(0u, (uint32_t) code_length));
        std::vector<uint32_t> in_block;
        std::vector<uint32_t> encoded_block;
        std::vector<uint32_t> decoded_block;
        bool encode_successful = false;
        bool decode_successful = false;
        std::size_t block_size = (code_length - fec_length) / 4;
        std::size_t generator_polynomial_root_count = fec_length;
        ReedSolomon reed_solomon(FIELD_DESCRIPTOR,
                                 GENERATOR_POLYNOMIAL_INDEX,
                                 generator_polynomial_root_count,
                                 code_length,
                                 fec_length);

        generate_input_data(in_block, block_size, true);
        reed_solomon.set_puncture_mask(mask);
        encode_successful = reed_solomon.encode_block(in_block,
                                                      encoded_block,
                                                      code_length,
                                                      fec_length,
                                                      true);
        RC_ASSERT(encode_successful);
        decode_successful = reed_solomon.decode_block(encoded_block,
                                                      decoded_block,
                                                      code_length,
                                                      fec_length,
                                                      true);
        RC_LOG() << "Broke when puncture size is: " << mask.size() << "\n";
        RC_ASSERT(decode_successful);
        RC_ASSERT(in_block == decoded_block);
    });
}
