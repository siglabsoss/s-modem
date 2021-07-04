#include <iostream>
#include <cstring>
#include <rapidcheck.h>
#include "driver/Interleaver.hpp"
#include "cpp_utils.hpp"
#include "FileUtils.hpp"
using namespace siglabs::file;

using namespace std;

/**
 * This method test the Interleaver class
 *
 * @param[in] code_length: Code length
 */
bool test_interleaver(const std::size_t code_length);

/**
 * This method generates random input data where the vector size in bytes is
 * four times the code length
 *
 * @param[in,out] input_data: Input vector to store randomly generated value
 * @param[in] code_length: Code length
 * @param[in] random: Set false to use counter data
 */
void generate_input_data_interleave(std::vector<uint32_t> &input_data,
                         std::size_t code_length,
                         std::size_t row_count,
                         bool random);

std::vector<uint32_t> generate_input_data_lin(const size_t sz, const bool random);



void generate_input_data(
    std::vector<uint32_t> &input_data,
    const uint32_t data_length,
    const uint32_t low = 0u,
    const uint32_t high = 0xffffffffu) {
    for(uint32_t i = 0; i < data_length; i++) {
        const uint32_t a_word = *rc::gen::inRange(low, high);
        input_data.push_back(a_word);
    }
}


bool test_more(void);
bool test_stream(void);
bool test_trouble(void);

int main(int argc, char const *argv[])
{
    bool pass = true;

    std::size_t code_length = 255;
    (void)code_length;

    pass &= test_interleaver(code_length);
    pass &= test_more();
    pass &= test_stream();
    pass &= test_trouble();

    return !pass; // important, main returns !pass
}

bool test_interleaver(const std::size_t code_length) {
    bool pass = true;

    pass &= rc::check("Test Interleaver", [code_length]() {
        std::vector<uint32_t> data_stream_copy;
        std::vector<uint32_t> data_stream;
        bool random = true;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        Interleaver interleaver(code_length);
        std::size_t word_size = Interleaver::calculate_words_for_code(code_length);
        data_stream.resize(word_size);

        generate_input_data_interleave(data_stream, code_length, 4, random);
        data_stream_copy = data_stream;
        interleave_successful = interleaver.inplace_interleave_block(data_stream);
        RC_ASSERT(interleave_successful);
        deinterleave_successful = interleaver.inplace_deinterleave_block(data_stream);
        RC_ASSERT(deinterleave_successful);
        RC_ASSERT(data_stream == data_stream_copy);
    });

    pass &= rc::check("Test Length Check", [code_length]() {
        std::vector<uint32_t> data_stream_copy;
        std::vector<uint32_t> data_stream;
        bool random = true;
        bool interleave_successful = false;
        Interleaver interleaver(code_length);
        interleaver.print_on_error = false; // omit printing for this test as we want it to fail
        std::size_t word_size = Interleaver::calculate_words_for_code(code_length);


        constexpr size_t test_sub = 16;

        RC_PRE(code_length > test_sub);

        for(size_t i = 1; i < 16; i++) {
            data_stream.resize(word_size-i);
            generate_input_data_interleave(data_stream, code_length, 4, random);
            interleave_successful = interleaver.inplace_interleave_block(data_stream);
            RC_ASSERT(!interleave_successful);
        }


        for(size_t i = 1; i < 16; i++) {
            data_stream.resize(word_size+i);
            generate_input_data_interleave(data_stream, code_length, 4, random);
            interleave_successful = interleaver.inplace_interleave_block(data_stream);
            RC_ASSERT(!interleave_successful);
        }


    });


    pass &= rc::check("calculate_words_for_code is consistent", [code_length](const unsigned a) {
        // check that passing the output of calculate_words_for_code() to itself gives
        // the same result
        unsigned b = Interleaver::calculate_words_for_code(a);
        unsigned c = Interleaver::calculate_words_for_code(b);

        RC_ASSERT(b==c);

    });

    /// a1 is code un-fixed
    /// a11 is fixed code
    /// a0 is number of words to to "stream" interleave
    pass &= rc::check("calculate_blocks_for_length is consistent", [code_length](const unsigned a0, const unsigned a1) {
        // check that passing the output of calculate_blocks_for_length() to itself gives
        // the same result

        // test inputs are not valud if a1 (code length) is zero
        RC_PRE(a1>0u);

        RC_PRE(a0 < 1000000000u);
        RC_PRE(a1 < 1000000000u);

        // note that if a0 is zero, that means we are trying to interleave 0 words
        // which is ok

        // we need to pass argument 2 of function below with a value that is already
        // been corrected.
        unsigned a11 = Interleaver::calculate_words_for_code(a1);

        // b is divided, so we need to multiply before we pass in to get c
        unsigned b = Interleaver::calculate_blocks_for_length(a0, a11);
        unsigned c = Interleaver::calculate_blocks_for_length(b*a11, a11);

        // cout << "a0: " << a0 << " a1 " << a11 << " b " << b << " c " << c << "\n";

        RC_ASSERT(b==c);
    });

    return pass; // important, return pass
}


bool test_more(void) {
    bool pass = true;

    pass &= rc::check("Test linear", []() {
        bool print = false;
        std::vector<uint32_t> input;
        std::vector<uint32_t> output;
        bool random = false;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        size_t code_length = 8;
        Interleaver interleaver(code_length);

        input = generate_input_data_lin(8, random);
        const std::vector<uint32_t> ideal = input;

        if( print ) {
            for(const auto w : input) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        interleave_successful = interleaver.inplace_interleave_block(input);

        if( print ) {
            for(const auto w : input) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        output = input;

        RC_ASSERT(interleave_successful);
        deinterleave_successful = interleaver.inplace_deinterleave_block(output);
        RC_ASSERT(deinterleave_successful);
        RC_ASSERT(output == ideal);

        if( print ) {
            for(const auto w : output) {
                cout << HEX32_STRING(w) << "\n";
            }
        }


        // exit(0);
    });

    pass &= rc::check("Test Interleaver 2", []() {

        // const uint8_t = *rc::gen::inRange(0u, 0xffu);


        std::vector<uint32_t> data_stream_ideal;
        std::vector<uint32_t> data_stream;
        bool random = false;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        size_t code_length = 20;
        Interleaver interleaver(code_length);
        std::size_t word_size = Interleaver::calculate_words_for_code(code_length);
        data_stream.resize(word_size);

        // cout << "word_size: " << word_size << " " << interleaver.length_ok(word_size) << "\n";

        generate_input_data_interleave(data_stream, code_length, 4, random);
        // for(const auto w : data_stream) {
        //     cout << HEX32_STRING(w) << "\n";
        // }
        // cout << "\n\n";
        data_stream_ideal = data_stream;
        interleave_successful = interleaver.inplace_interleave_block(data_stream);

        // for(const auto w : data_stream) {
        //     cout << HEX32_STRING(w) << "\n";
        // }


        RC_ASSERT(interleave_successful);
        deinterleave_successful = interleaver.inplace_deinterleave_block(data_stream);
        RC_ASSERT(deinterleave_successful);
        RC_ASSERT(data_stream == data_stream_ideal);

        // exit(0);
    });

    pass &= rc::check("Test Interleaver Random length", []() {

        std::vector<uint32_t> data_stream_ideal;
        std::vector<uint32_t> data_stream;
        bool random = false;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        size_t code_length = *rc::gen::inRange(1, 2048);
        Interleaver interleaver(code_length);
        std::size_t word_size = Interleaver::calculate_words_for_code(code_length);
        data_stream.resize(word_size);

        // cout << "word_size: " << word_size << " " << interleaver.length_ok(word_size) << "\n";

        generate_input_data_interleave(data_stream, code_length, 4, random);
        // for(const auto w : data_stream) {
        //     cout << HEX32_STRING(w) << "\n";
        // }
        // cout << "\n\n";
        data_stream_ideal = data_stream;
        interleave_successful = interleaver.inplace_interleave_block(data_stream);

        // for(const auto w : data_stream) {
        //     cout << HEX32_STRING(w) << "\n";
        // }


        RC_ASSERT(interleave_successful);
        deinterleave_successful = interleaver.inplace_deinterleave_block(data_stream);
        RC_ASSERT(deinterleave_successful);
        RC_ASSERT(data_stream == data_stream_ideal);

        // exit(0);
    });


    return pass; // important, return pass
}



bool test_stream(void) {
    bool pass = true;

    // cout << "hi\n";
    // auto len = Interleaver::calculate_blocks_for_length(24, 21);
    // cout << "len: " << len << "\n";
    // exit(0);

    pass &= rc::check("Test Stream", []() {
        bool print = false;
        std::vector<uint32_t> input;
        std::vector<uint32_t> intermediate_output;
        std::vector<uint32_t> output;
        const bool random = false;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        const size_t code_length = Interleaver::calculate_words_for_code(*rc::gen::inRange(2, 1024));
        Interleaver interleaver(code_length);

        const size_t total_input_length = *rc::gen::inRange(2, 1024*10);

        // generate_input_data(input, total_input_length, 0xaabbcc04);
        // generate_input_data(input, total_input_length, 04);

        input = generate_input_data_lin(total_input_length, false);
        // cout << "code_length: " << code_length << " total input: " << total_input_length << "\n";



        // input = generate_input_data_lin(8, random);
        const std::vector<uint32_t> ideal = input;

        if( print ) {
            for(const auto w : input) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        // cout << "before int\n";
        interleave_successful = interleaver.interleave(input, intermediate_output);
        // cout << "after int\n";


        RC_ASSERT(interleave_successful);

        if( print ) {
            for(const auto w : intermediate_output) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        //// should fail
        {
            std::vector<uint32_t> output2;
            std::vector<uint32_t> intermediate_output_2 = intermediate_output;
            intermediate_output_2.push_back(4);
            RC_PRE(code_length > 1u);
            interleaver.print_on_error = false;
            bool should_fail = interleaver.deinterleave(intermediate_output_2, output2);
            interleaver.print_on_error = true;
            RC_ASSERT(!should_fail);
        }
        ////



        // output = input;


        // right now interleaver doesn't have a way to tell if you passed in a short final block
        // so this is left up to the user.  if you happen to know the final lenght, pass it here
        // if not no joy and output will always be padded with zeros when input is non multiple
        deinterleave_successful = interleaver.deinterleave(intermediate_output, output);
        RC_ASSERT(deinterleave_successful);

        if( print ) {
            for(const auto w : output) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        // exit(0);


        RC_ASSERT(output == ideal);

        // if( print ) {
        //     for(const auto w : output) {
        //         cout << HEX32_STRING(w) << "\n";
        //     }
        // }


        // exit(0);
    });

    return pass; // important, return pass
}




bool test_trouble(void) {
    bool pass = true;

    // cout << "hi\n";
    // auto len = Interleaver::calculate_blocks_for_length(24, 21);
    // cout << "len: " << len << "\n";
    // exit(0);

    const auto orig = file_read_hex("../src/data/interleaver_trouble_1.hex");

    
    pass &= rc::check("Test Trouble with 117", [&]() {
        
        // cout << HEX32_STRING(orig[0]) << "\n";

        bool print = false;

        std::vector<uint32_t> input = orig;
        std::vector<uint32_t> intermediate_output;
        std::vector<uint32_t> output;
        bool interleave_successful = false;
        bool deinterleave_successful = false;
        Interleaver interleaver(117);

        // const size_t total_input_length = *rc::gen::inRange(2, 1024*10);

        // generate_input_data(input, total_input_length, 0xaabbcc04);
        // generate_input_data(input, total_input_length, 04);

        // input = generate_input_data_lin(total_input_length, false);
        // cout << "code_length: " << code_length << " total input: " << total_input_length << "\n";



        // input = generate_input_data_lin(8, random);
        const std::vector<uint32_t> ideal = input;

        if( print ) {
            for(const auto w : input) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }

        // cout << "before int\n";
        interleave_successful = interleaver.interleave(input, intermediate_output);

        deinterleave_successful = interleaver.deinterleave(intermediate_output, output);
        RC_ASSERT(deinterleave_successful);


        if( print ) {
            for(const auto w : output) {
                cout << HEX32_STRING(w) << "\n";
            }
            cout << "\n\n";
        }



        RC_ASSERT(output == ideal);


        // exit(0);



        
    });

    return pass; // important, return pass
}

void generate_input_data_interleave(std::vector<uint32_t> &input_data,
                         std::size_t code_length,
                         std::size_t row_count,
                         bool random) {
    uint8_t char_data;
    std::vector<uint8_t> data;
    // std::size_t row_count = 4;
    std::size_t word_size = Interleaver::calculate_words_for_code(code_length);
    data.resize(code_length);

    for (std::size_t i = 0; i < code_length; i++) {
        char_data = random ? *rc::gen::inRange(0u, 0xffu) : i;
        data[i] = char_data;
    }

    for (std::size_t i = 0; i < row_count; i++) {
        std::memcpy(input_data.data() + (i * (word_size / 4)),
                    data.data(),
                    code_length);
    }
}

std::vector<uint32_t> generate_input_data_lin(const size_t sz, const bool random) {
    std::vector<uint8_t> chars;
    // std::size_t row_count = 4;
    // std::size_t word_size = Interleaver::calculate_words_for_code(code_length);
    // size_t sz = words.size();
    auto charlen = sz*4;
    chars.resize(charlen);


    for (std::size_t i = 0; i < charlen; i++) {
        const uint8_t c = random ? *rc::gen::inRange(0u, 0xffu) : i;
        chars[i] = c;
    }

    std::vector<uint32_t> out;
    out.resize(sz);

    std::memcpy(out.data(), chars.data(), charlen);
    return out;

    // for (std::size_t i = 0; i < row_count; i++) {
    //     std::memcpy(input_data.data() + (i * (word_size / 4)),
    //                 data.data(),
    //                 code_length);
    // }
}

