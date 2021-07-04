#include <iostream>
#include <string>
#include "ReedSolomon.hpp"
#include <iomanip>

#define HEX32_STRING(n) std::setfill('0') << std::setw(8) << std::hex << (n) << std::dec

ReedSolomon::ReedSolomon():
    data_length(code_length - fec_length),
    field(field_descriptor,
          schifra::galois::primitive_polynomial_size06,
          schifra::galois::primitive_polynomial06),
    generator_polynomial(field),
    decoder_255_223_32(field, generator_polynomial_index),
    decoder_255_207_48(field, generator_polynomial_index),
    decoder_255_191_64(field, generator_polynomial_index),
    decoder_255_175_80(field, generator_polynomial_index),
    decoder_255_159_96(field, generator_polynomial_index),
    decoder_255_127_128(field, generator_polynomial_index) {

    create_root_generator();

    std::cout << "Dont use this default ReedSolomon() constructor!!\n";
}

ReedSolomon::ReedSolomon(const std::size_t field_descriptor,
                         const std::size_t generator_polynomial_index,
                         const std::size_t generator_polynomial_root_count,
                         const std::size_t code_length,
                         const std::size_t fec_length):
    field_descriptor(field_descriptor),
    generator_polynomial_index(generator_polynomial_index),
    generator_polynomial_root_count(generator_polynomial_root_count),
    code_length(code_length),
    fec_length(fec_length),
    data_length(code_length - fec_length),
    field(field_descriptor,
          schifra::galois::primitive_polynomial_size06,
          schifra::galois::primitive_polynomial06),
    generator_polynomial(field),
    decoder_255_223_32(field, generator_polynomial_index),
    decoder_255_207_48(field, generator_polynomial_index),
    decoder_255_191_64(field, generator_polynomial_index),
    decoder_255_175_80(field, generator_polynomial_index),
    decoder_255_159_96(field, generator_polynomial_index),
    decoder_255_127_128(field, generator_polynomial_index) {

    create_root_generator();

    if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_32)) {
        encoder_255_223_32 = new schifra::reed_solomon::encoder<255, 32, 223>(
                                                field, generator_polynomial);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_48)) {
        encoder_255_207_48 = new schifra::reed_solomon::encoder<255, 48, 207>(
                                                field, generator_polynomial);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_64)) {
        encoder_255_191_64 = new schifra::reed_solomon::encoder<255, 64, 191>(
                                                field, generator_polynomial);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_80)) {
        encoder_255_175_80 = new schifra::reed_solomon::encoder<255, 80, 175>(
                                                field, generator_polynomial);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_96)) {
        encoder_255_159_96 = new schifra::reed_solomon::encoder<255, 96, 159>(
                                                field, generator_polynomial);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_128)) {
        encoder_255_127_128 = new schifra::reed_solomon::encoder<255, 128, 127>(
                                                field, generator_polynomial);
    } else {
        std::cout << "No encoder was initialized. Please select a valid code "
                  << "and FEC length"
                  << std::endl;
    }
}

// this is an easy constructor which just calls the other
// this is c++11 delegating constructor
ReedSolomon::ReedSolomon(const std::size_t code_length,
            const std::size_t fec_length) :
    ReedSolomon(8, 120, fec_length, code_length, fec_length) {
}


ReedSolomon::~ReedSolomon() {

}

// pass proposed settings and this will return true if they are valid
bool ReedSolomon::settings_valid(const std::size_t code_length, const std::size_t fec_length) {
    if (code_length == CODE_LEN_255) {
        if(fec_length == FEC_LEN_32 ||
           fec_length == FEC_LEN_48 ||
           fec_length == FEC_LEN_64 ||
           fec_length == FEC_LEN_80 ||
           fec_length == FEC_LEN_96 ||
           fec_length == FEC_LEN_128) {
            return true;
        }
    }
    return false;
}

bool ReedSolomon::encode_block(std::vector<uint32_t> &in_message,
                               std::vector<uint32_t> &out_message,
                               std::size_t code_length,
                               std::size_t fec_length,
                               bool puncture) {
    bool encode_successful = false;
    size_t max_word = (code_length - fec_length) / 4;

    if ( !settings_valid(code_length, fec_length) ) {
        std::cout << "Error: Invalid code length " << code_length
                  << " or " << "FEC length " << fec_length << std::endl;
        return false;
    }

    if (in_message.size() > max_word) {
        std::cout << "Error: Message is too long. For a code length of "
                  << code_length << " and FEC length of " << fec_length
                  << " the maximum allowed words to encode is "
                  << max_word << std::endl; 
        return false;
    }

    std::string message = vector_to_str(in_message);
    message.resize(code_length, 0x00);

    if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_32)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_32> block;
        encode_successful = encode(encoder_255_223_32, message, block,
                                   puncture, code_length, out_message);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_48)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_48> block;
        encode_successful = encode(encoder_255_207_48, message, block,
                                   puncture, code_length, out_message);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_64)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_64> block;
        encode_successful = encode(encoder_255_191_64, message, block,
                                   puncture, code_length, out_message);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_80)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_80> block;
        encode_successful = encode(encoder_255_175_80, message, block,
                                   puncture, code_length, out_message);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_96)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_96> block;
        encode_successful = encode(encoder_255_159_96, message, block,
                                   puncture, code_length, out_message);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_128)) {
        schifra::reed_solomon::block<CODE_LEN_255, FEC_LEN_128> block;
        encode_successful = encode(encoder_255_127_128, message, block,
                                   puncture, code_length, out_message);
    } else {
        std::cout << "Didn't match any encode\n";
    }

    return encode_successful;
}

bool ReedSolomon::encode_message(const std::vector<uint32_t> &in_message2,
                                 std::vector<uint32_t> &out_message,
                                 std::size_t code_length,
                                 std::size_t fec_length,
                                 bool puncture) {
    std::vector<uint32_t> in_message = in_message2; // memcopy, can be removed
    std::vector<uint32_t>::const_iterator first;
    std::vector<uint32_t>::const_iterator last;
    std::vector<uint32_t> out_block;
    std::size_t out_size;
    std::size_t word_count = code_length / 4;
    const std::size_t max_word = (code_length - fec_length) / 4;
    std::size_t encode_count = in_message.size() / max_word;
    bool encode_successful = false;

    if (in_message.size() % max_word) encode_count++;
    if (code_length % 4) word_count++;
    out_size = word_count * encode_count;
    resize_in_message(in_message, max_word, encode_count);
    out_message.reserve(out_size);
    out_block.reserve(word_count);

    for (size_t i = 0; i < encode_count; i++) {
        first = in_message.begin() + (max_word * i);
        last = in_message.begin() + ((i + 1) * max_word);
        std::vector<uint32_t> in_block(first, last);
        encode_successful = encode_block(in_block,
                                         out_block,
                                         code_length,
                                         fec_length);
        if (encode_successful) {
            out_message.insert(out_message.end(),
                               out_block.begin(),
                               out_block.end());
        } else {
            std::cout << "Error: Encoding message failed" << std::endl;
            return false;
        }
    }

    return true;
}

bool ReedSolomon::decode_block(std::vector<uint32_t> &encoded_vector,
                               std::vector<uint32_t> &decoded_vector,
                               const std::size_t code_length,
                               const std::size_t fec_length,
                               const bool puncture) {
    bool decode_successful = false;
    if (!settings_valid(code_length, fec_length)) {
        std::cout << "Error: Invalid code length " << code_length
                  << " or " << "FEC length " << fec_length << std::endl;
        return false;
    }

    if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_32)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_32> received_block;
        decode_successful = decode(decoder_255_223_32, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_48)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_48> received_block;
        decode_successful = decode(decoder_255_207_48, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_64)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_64> received_block;
        decode_successful = decode(decoder_255_191_64, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_80)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_80> received_block;
        decode_successful = decode(decoder_255_175_80, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_96)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_96> received_block;
        decode_successful = decode(decoder_255_159_96, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    } else if ((code_length == CODE_LEN_255) && (fec_length == FEC_LEN_128)) {
        schifra::reed_solomon::block<CODE_LEN_255,FEC_LEN_128> received_block;
        decode_successful = decode(decoder_255_127_128, encoded_vector,
                                   received_block, puncture, code_length,
                                   fec_length, decoded_vector);
    }

    return decode_successful;
}

bool ReedSolomon::decode_message(std::vector<uint32_t> &encoded_vector,
                                 std::vector<uint32_t> &decoded_vector,
                                 const std::size_t code_length,
                                 const std::size_t fec_length,
                                 const bool puncture) {
    std::vector<uint32_t>::const_iterator first;
    std::vector<uint32_t>::const_iterator last;
    std::vector<uint32_t> decoded_block;
    const std::size_t word_count = (code_length / 4) + ((code_length % 4)?1:0);
    bool decode_successful = false;

    // how many words will we be pulling out each time?
    const size_t decode_words = (code_length - fec_length) / 4;
    std::vector<uint32_t> default_block;
    default_block.resize(decode_words);

    decoded_vector.reserve(encoded_vector.size());
    decoded_block.reserve(word_count);

    bool some_fail = false;

    if ((encoded_vector.size() % word_count)) {
        std::cout << "Error: Encoded vector need to have size multiple of "
                  << word_count << ". Current vector size is "
                  << encoded_vector.size() << "\n";

        return false;
    } else {
        const std::size_t decode_count = encoded_vector.size() / word_count;
        // std::cout << "decode_count  " << decode_count << "\n";
        for (std::size_t i = 0; i < decode_count; i++) {
            // std::cout << "i " << i << "\n";
            first = encoded_vector.begin() + (word_count * i);
            last = encoded_vector.begin() + ((i + 1) * word_count);
            // copy into a vector
            std::vector<uint32_t> encoded_block(first, last);
            decode_successful = decode_block(encoded_block,
                                             decoded_block,
                                             code_length,
                                             fec_length);
            if (decode_successful) {
                decoded_vector.insert(decoded_vector.end(),
                                      decoded_block.begin(),
                                      decoded_block.end());

                // std::cout << "sz: " << decoded_block.size() << "\n";
                // std::cout << "word_count: " << word_count << "\n";
                // std::cout << "decode_count: " << decode_count << "\n";
                                      

            } else {
                std::cout << "Error: Decoding message block " << i << " / " << decode_count << " failed inserting zeros at " << decoded_vector.size() << "\n";
                decoded_vector.insert(decoded_vector.end(),
                                      default_block.begin(),
                                      default_block.end());
                some_fail = true;
            }
        }
    }

    if( false && some_fail ) {
        std::cout << "these failed\n";
        for(const auto w : encoded_vector) {
            std::cout << HEX32_STRING(w) << "\n";
        }
        std::cout << "\n\n\n";
    }

    if ((decoded_vector.back() & 0xFFFFFF00) == RS_REMOVE_KEY) {
        if( print_key ) {
            std::cout << "REMOVE KEY\n";
        }
        std::size_t remove_words = decoded_vector.back() & 0xFF;
        decoded_vector.resize(decoded_vector.size() - remove_words);
    }

    return true;
}


std::vector<std::pair<uint32_t,uint32_t>> ReedSolomon::decode_message2(std::vector<uint32_t> &encoded_vector,
                                 std::vector<uint32_t> &decoded_vector,
                                 const std::size_t code_length,
                                 const std::size_t fec_length,
                                 const bool puncture) {
    std::vector<uint32_t>::const_iterator first;
    std::vector<uint32_t>::const_iterator last;
    std::vector<uint32_t> decoded_block;
    bool decode_successful = false;
    const std::size_t word_count = (code_length / 4) + ((code_length % 4)?1:0);

    // how many words will we be pulling out each time?
    const size_t decode_words = (code_length - fec_length) / 4;
    std::vector<uint32_t> default_block;
    default_block.resize(decode_words);

    // if (code_length % 4) word_count++;
    decoded_vector.reserve(encoded_vector.size());
    decoded_block.reserve(word_count);

    bool some_fail = false;

    std::vector<std::pair<uint32_t,uint32_t>> failed_blocks;

    if ((encoded_vector.size() % word_count)) {
        std::cout << "Error: Encoded vector need to have size multiple of "
                  << word_count << std::endl;

        return {};
    } else {
        const std::size_t decode_count = encoded_vector.size() / word_count;
        // std::cout << "decode_count  " << decode_count << "\n";
        for (std::size_t i = 0; i < decode_count; i++) {
            // std::cout << "i " << i << "\n";
            first = encoded_vector.begin() + (word_count * i);
            last = encoded_vector.begin() + ((i + 1) * word_count);
            // copy into a vector
            std::vector<uint32_t> encoded_block(first, last);
            decode_successful = decode_block(encoded_block,
                                             decoded_block,
                                             code_length,
                                             fec_length);
            if (decode_successful) {
                decoded_vector.insert(decoded_vector.end(),
                                      decoded_block.begin(),
                                      decoded_block.end());

                // std::cout << "sz: " << decoded_block.size() << "\n";
                // std::cout << "word_count: " << word_count << "\n";
                // std::cout << "decode_count: " << decode_count << "\n";
                                      

            } else {
                auto failed_at = decoded_vector.size();
                std::cout << "Error: Decoding message block " << i << " / " << decode_count << " failed inserting zeros at " << failed_at << "\n";
                decoded_vector.insert(decoded_vector.end(),
                                      default_block.begin(),
                                      default_block.end());
                some_fail = true;
                failed_blocks.emplace_back(failed_at,failed_at+decode_words);
            }
        }
    }

    if( false && some_fail ) {
        std::cout << "these failed\n";
        for(const auto w : encoded_vector) {
            std::cout << HEX32_STRING(w) << "\n";
        }
        std::cout << "\n\n\n";
    }

    if ((decoded_vector.back() & 0xFFFFFF00) == RS_REMOVE_KEY) {
        std::cout << "REMOVE KEY\n";
        std::size_t remove_words = decoded_vector.back() & 0xFF;
        decoded_vector.resize(decoded_vector.size() - remove_words);
    }

    return failed_blocks;
}

void ReedSolomon::set_puncture_mask(std::set<uint32_t> &mask) {
    puncture_mask = mask;
}

std::string ReedSolomon::vector_to_str(std::vector<uint32_t> &vector_message) {
    std::string out_message;
    std::size_t vector_byte_size = vector_message.size()*4;
    out_message.resize(vector_byte_size);
    std::memcpy(&out_message.front(),vector_message.data(),vector_byte_size);

    return out_message;
}

template <typename T>
void ReedSolomon::schifra_block_to_vector(std::vector<uint32_t> &output,
                                          const T &block, size_t code_length) {
    // How many uint32_t do we need to represent the data
    size_t word_count = code_length / 4;
    // Was the division even?
    size_t rem = code_length % 4;
    if( rem != 0 ) {
        // Add 1 uint32_t to hold a non even divison
        word_count++;
    }

    // It seems like block.data[] is an array of uint32_t or similar types,
    // however only 8 bits are used this means if we do a straight memcopy we
    // will be getting zeros that are not part of the mesage


    // Load into a standard vector
    std::vector<unsigned char> aschar;
    aschar.resize(code_length);
    for(size_t i = 0; i < code_length; i++) {
        // Using the [] means we only get the data we want
        aschar[i] = block.data[i];
    }

    // Resize the output to the correct length
    if (output.size() != word_count) {
        output.resize(word_count, 0);
    }
    // Copy the bytes from the char vector
    std::memcpy(output.data(), aschar.data(), code_length);
}

template <typename T>
void ReedSolomon::schifra_vector_to_block(const std::vector<uint32_t> &in_vector,
                                          T &block,
                                          const std::size_t code_length) {
    for (std::size_t i = 0; i < code_length; i++) {
        block.data[i] = ((in_vector[i / 4])>>((i % 4) * 8))&(0xFF);
    }
}

void ReedSolomon::create_root_generator(bool print) {
    bool gen_seq_root = schifra::make_sequential_root_generator_polynomial(
                                                field,
                                                generator_polynomial_index,
                                                generator_polynomial_root_count,
                                                generator_polynomial);

    if (!gen_seq_root) {
        std::cout << "Error: Failed to create sequential root generator\n";
    } else {
        if(print) {
            std::cout << "Successfully create sequential root generator\n";
        }
    }
}

void ReedSolomon::str_to_vector(const std::string str_message,
                                std::vector<uint32_t> &out_vector) {
    out_vector.resize(str_message.size()/4);
    std::memcpy(out_vector.data(), str_message.data(), str_message.size());
}

template <typename T>
void ReedSolomon::print_block(const T &block, std::size_t code_length) {
    std::cout << "[";
    for(size_t i = 0; i < code_length; i++) {
        if (i == code_length - 1) {
            std::cout << block.data[i];
        } else {
            std::cout << block.data[i] << ", ";
        }
    }
    std::cout << "] " << "(len = " << code_length << ")" << std::endl;
}

void ReedSolomon::resize_in_message(std::vector<uint32_t> &in_message,
                                    std::size_t max_word,
                                    std::size_t encode_count) {
    if (in_message.size() % max_word) {
        std::size_t remove_words = max_word - (in_message.size() % max_word);
        std::size_t new_size = max_word * encode_count;
        in_message.resize(new_size, 0x00);
        uint32_t remove_signature = RS_REMOVE_KEY|remove_words;
        in_message[new_size - 1] = remove_signature;
    }
}

template <typename T>
void ReedSolomon::puncture_block(T &block, size_t code_length) {
    for (uint32_t puncture_index : puncture_mask) {
        if (puncture_index < (code_length - 1)) {
            block.data[puncture_index] = 0;
        }
    }
}

template <typename T, typename U>
bool ReedSolomon::encode(T *encoder,
                         std::string message,
                         U &block,
                         bool puncture,
                         std::size_t code_length,
                         std::vector<uint32_t> &out_message) {
    if (!encoder->encode(message, block)) {
        std::cout << "Error: Critical encoding failure! "
                  << "Message: " << block.error_as_string() << "\n";
        return false;
    }
    if (puncture) puncture_block(block, code_length);
    schifra_block_to_vector(out_message, block, code_length);

    return true;
}

template <typename T, typename U>
bool ReedSolomon::decode(T &decoder,
                         const std::vector<uint32_t> &encoded_vector,
                         U &received_block,
                         const bool puncture,
                         const std::size_t code_length,
                         const std::size_t fec_length,
                         std::vector<uint32_t> &decoded_vector) {
    std::string decoded_string;
    decoded_string.resize(code_length - fec_length);
    schifra_vector_to_block(encoded_vector, received_block, code_length);
    if (!decoder.decode(received_block)) {
        std::cout << "Error: Critical decoding failure! "
                  << "Message: " << received_block.error_as_string()
                  << "\n";
        return false;
    }

    if (!received_block.data_to_string(decoded_string)) {
        std::cout << "Error: Unable to convert block data to string\n";
        return false;
    }
    str_to_vector(decoded_string, decoded_vector);

    return true;
}