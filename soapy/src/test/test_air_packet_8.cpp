#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "driver/AirPacket.hpp"
#include "cpp_utils.hpp"
#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_error_processes.hpp"
#include "schifra_reed_solomon_bitio.hpp"
#include "driver/ReedSolomon.hpp"

using namespace std;

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random = true) {
    uint32_t data;
    for(uint32_t i = 0; i < data_length; i++) {
        data = random ?  *rc::gen::inRange(0u, 0xffffffffu) : i;
        input_data.push_back(data);
    }
}

void print_data(std::vector<uint32_t> &input_data) {
    std::cout << "[";
    for (uint32_t i = 0; i < input_data.size(); i++) {
        if (i == input_data.size() - 1) {
            std::cout << input_data[i];
        } else {
            std::cout << input_data[i] << ",";
        }
    }
    std::cout << "] " << "(len = " << input_data.size() << ")" << std::endl;
}


bool test_normal() {
    bool pass = true;

    pass &= rc::check("Testing Reed Solomon within AirPacket framework 1", [&]() {

        const bool print2 = false;

        // uint32_t code_length = 255;
        // uint32_t fec_length = 48; // 32

        // int code_bad0, code_bad1;
        
        

        AirPacketOutbound tx;
        tx.print_settings_did_change = false;
        tx.print5 = false;
        tx.setTestDefaults();
        // code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);

        AirPacketInbound airRx;
        airRx.print_settings_did_change = false;
        airRx.setTestDefaults();
        // airRx.reed_solomon->print_key = false;
        // code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        
        
        // RC_ASSERT(!code_bad0);
        // RC_ASSERT(!code_bad1);

        uint32_t val;
        std::vector<uint32_t> input_data;

        const unsigned input_length = *rc::gen::inRange(1u, 3000u);
        generate_input_data(input_data, input_length);

        // cout << "input length: " << input_data.size() << "\n";


        uint8_t seq;
        std::vector<uint32_t> d2 = tx.transform(input_data, seq, 0);
	
	RC_ASSERT(d2.size() == tx.getLengthForInput(input_length));
	//std::cout << "transform size: " << d2.size() << " getLengthForInput: " << tx.getLengthForInput(input_length) << " input_size: " << input_length << std::endl;
        // cout << "transform length: " << d2.size() << "\n";
        tx.padData(d2);
        // cout << "final pad length: " << d2.size() << "\n\n";

        auto sliced_words = tx.emulateHiggsToHiggs(d2);

        uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

        for(int i = 0; i < 128; i++) {
            final_counter++;
            sliced_words[0].push_back(final_counter);
            sliced_words[1].push_back(0);
        }


        // #define DATA_TONE_NUM 32
        // WARNING this class was hand designed around 32 full iq subcarriers
        // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
        // airRx.data_tone_num = DATA_TONE_NUM;


        std::vector<uint32_t> output_data;
        bool found_something = false;
        bool found_something_again = false;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t found_length;
        air_header_t found_header;

        airRx.demodulateSlicedSimple(
            sliced_words,
            output_data,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        RC_ASSERT(found_something);

        if( print2 ) {
            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
            }
        }

        RC_ASSERT( input_data == output_data );


        // if( runs > 0) {
        //     exit(0);
        // }
        // runs++;

    });

    return pass;
}

// Templates like this is useful but unused in other AirPacket tests
template <typename T>
void simpleSettings(T& x) {
    x.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    x.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    // x.set_subcarrier_allocation(MAPMOV_SUBCARRIER_128); // test defaults, not duplex compat
}

std::vector<uint32_t> _get_counter(const int start, const int stop) {
  std::vector<uint32_t> out;
  for(int i = start; i < stop; i++) {
    out.push_back(i);
  }

  if(0) {
    cout << "get_counter( " << start << ", " << stop << ")" << endl;
    for(auto it = out.begin(); it < out.end(); it++) {
      cout << *it << endl;
    }
  }
  return out;
}


// used to help make test_feedback_bus_25
bool test_simple_320() {
    bool pass = true;

    pass &= rc::check("Testing Simple AirPacket for before/after duplex testing", [&]() {

        const bool print2 = false;

        // uint32_t code_length = 255;
        // uint32_t fec_length = 48; // 32

        // int code_bad0, code_bad1;
        
        

        AirPacketOutbound tx;
        tx.print_settings_did_change = false;
        tx.print5 = false;
        simpleSettings(tx);
        // tx.setTestDefaults();
        // code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);

        AirPacketInbound airRx;
        airRx.print_settings_did_change = false;
        simpleSettings(airRx);
        // airRx.setTestDefaults();
        // airRx.reed_solomon->print_key = false;
        // code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        
        
        // RC_ASSERT(!code_bad0);
        // RC_ASSERT(!code_bad1);

        uint32_t val;
        std::vector<uint32_t> input_data;

        const unsigned input_length = 20;//*rc::gen::inRange(1u, 3000u);
        input_data = _get_counter(0xff000000, 0xff000000+input_length);
        // generate_input_data(input_data, input_length);

        cout << "input length: " << input_data.size() << "\n";


        uint8_t seq;
        std::vector<uint32_t> d2 = tx.transform(input_data, seq, 0);

        cout << "transform length: " << d2.size() << "\n";
        tx.padData(d2);
        cout << "final pad length: " << d2.size() << "\n\n";

        auto sliced_words = tx.emulateHiggsToHiggs(d2);

        for(auto w : sliced_words[1]) {
            cout << HEX32_STRING(w) << "\n";
        }

        uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

        for(int i = 0; i < 128; i++) {
            final_counter++;
            sliced_words[0].push_back(final_counter);
            sliced_words[1].push_back(0);
        }


        // #define DATA_TONE_NUM 32
        // WARNING this class was hand designed around 32 full iq subcarriers
        // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
        // airRx.data_tone_num = DATA_TONE_NUM;


        std::vector<uint32_t> output_data;
        bool found_something = false;
        bool found_something_again = false;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t found_length;
        air_header_t found_header;

        airRx.demodulateSlicedSimple(
            sliced_words,
            output_data,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        RC_ASSERT(found_something);

        if( print2 ) {
            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
            }
        }

        RC_ASSERT( input_data == output_data );


        // if( runs > 0) {
            exit(0);
        // }
        // runs++;

    });

    return pass;
}




void change_code(AirPacketOutbound& tx, AirPacketInbound& airRx, unsigned choice) {
    int code_bad0, code_bad1;
    uint32_t code_length = 255;
    uint32_t fec_length;

    if( choice == 32) {
        fec_length = 32;
        code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        airRx.reed_solomon->print_key = false;
        RC_ASSERT(!code_bad0);
        RC_ASSERT(!code_bad1);
        return;
    }

    if( choice == 48) {
        fec_length = 48;
        code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        airRx.reed_solomon->print_key = false;
        RC_ASSERT(!code_bad0);
        RC_ASSERT(!code_bad1);
        return;
    }

    if( choice == 0) {
        code_bad0 = tx.set_code_type(AIRPACKET_CODE_UNCODED, 0, 0);
        code_bad1 = airRx.set_code_type(AIRPACKET_CODE_UNCODED, 0, 0);
        RC_ASSERT(!code_bad0);
        RC_ASSERT(!code_bad1);
        return;
    }
}


int main() {
    bool pass = true;

    int runs = 0;

    pass &= test_normal();
    // pass &= test_simple_320();

    pass &= rc::check("Testing All Combinations of rs/interleave", [&]() {
        // return;
        const bool print2 = false;
        const bool print3 = false;
        const bool print_settings = false;

        uint32_t code_length = 255;
        // uint32_t fec_length = 48; // 32
        uint32_t fec_length = 32; // 32

        int code_bad0, code_bad1;

        const uint32_t interleave = *rc::gen::inRange(0u, 6000u);

        if( print_settings) {
            cout << "Initial interlave: " << interleave << "\n";
        }
        

        AirPacketOutbound tx;
        tx.print_settings_did_change = false;
        tx.print5 = false;
        tx.setTestDefaults();
        code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        tx.set_interleave(interleave);

        AirPacketInbound airRx;
        airRx.print_settings_did_change = false;
        airRx.setTestDefaults();
        code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        airRx.reed_solomon->print_key = false;
        airRx.set_interleave(interleave);
        
        
        RC_ASSERT(!code_bad0);
        RC_ASSERT(!code_bad1);

        uint32_t val;
        std::vector<uint32_t> input_data;

        for(int i = 0; i < 7; i++) {

            // cout << (unsigned[3]){1,2,3}[*rc::gen::inRange(0u, 3u)] << "\n";

            const unsigned input_length = *rc::gen::inRange(1u, 3000u);
            input_data.resize(0);
            generate_input_data(input_data, input_length);
            if( print_settings) {
                cout << "input length: " << input_data.size() << "\n";
            }

            if( print3 ) {
                for(auto w: input_data) {
                    cout <<"  o   " << w <<"\n";
                }
            }

            if( i == 1 || i == 3 || i == 5) {
                unsigned choice = (unsigned[3]){0,32,48}[*rc::gen::inRange(0u, 3u)];
                cout << "changing to code: " << choice << "\n";
                change_code(tx, airRx, choice);

                if( *rc::gen::inRange(0u, 2u) > 0 ) {
                    uint32_t interleave2 = *rc::gen::inRange(0u, 6000u);
                    cout << "changing Interleave " << interleave2 << "\n";
                    tx.set_interleave(interleave2);
                    airRx.set_interleave(interleave2);
                }

            }


            uint8_t seq;
            std::vector<uint32_t> d2 = tx.transform(input_data, seq, 0);
	    //std::cout << ":::::::::::::::::::::::::::::::::::: " << d2.size() << std::endl;
	    RC_ASSERT(d2.size() == tx.getLengthForInput(input_length));
	    

            if( print_settings) {
                cout << "transform length: " << d2.size() << "\n";
            }
            tx.padData(d2);
            if( print_settings) {
                cout << "final pad length: " << d2.size() << "\n\n";
            }

            auto sliced_words = tx.emulateHiggsToHiggs(d2);

            uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

            for(int i = 0; i < 128; i++) {
                final_counter++;
                sliced_words[0].push_back(final_counter);
                sliced_words[1].push_back(0);
            }


            // #define DATA_TONE_NUM 32
            // WARNING this class was hand designed around 32 full iq subcarriers
            // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
            // airRx.data_tone_num = DATA_TONE_NUM;


            std::vector<uint32_t> output_data;
            bool found_something = false;
            bool found_something_again = false;
            unsigned do_erase;
            uint32_t found_at_frame;
            unsigned found_at_index; // found_i
            uint32_t found_length;
            air_header_t found_header;

            airRx.demodulateSlicedSimple(
                sliced_words,
                output_data,
                found_something,
                found_length,
                found_at_frame,
                found_at_index,
                found_header,
                do_erase
                );

            RC_ASSERT(found_something);

            if( print2 ) {
                std::cout << std::endl << "output_data:" << std::endl;
                for( uint32_t w : output_data ) {
                    std::cout << HEX32_STRING(w) << std::endl;
                }
            }

            output_data.resize(input_data.size());

            RC_ASSERT( input_data == output_data );
        }
        // return;


        // if( runs > 0) {
        //     exit(0);
        // }
        // runs++;

    });


    pass &= rc::check("AirPacket Intereleave 1", [&]() {
        const bool print2 = false;
        const bool print3 = false;

        // uint32_t code_length = 255;
        // uint32_t fec_length = 48; // 32
        // uint32_t fec_length = 32; // 32

        // int code_bad0, code_bad1;

        

        AirPacketOutbound tx;
        tx.print_settings_did_change = false;
        tx.print5 = false;
        tx.setTestDefaults();
        // code_bad0 = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        tx.set_interleave(20);

        AirPacketInbound airRx;
        airRx.print_settings_did_change = false;
        airRx.setTestDefaults();
        // code_bad1 = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        // airRx.reed_solomon->print_key = false;
        airRx.set_interleave(20);
        
        
        // RC_ASSERT(!code_bad0);
        // RC_ASSERT(!code_bad1);

        uint32_t val;
        std::vector<uint32_t> input_data;

        bool random = true;

        for(int i = 0; i < 7; i++) {

            // cout << (unsigned[3]){1,2,3}[*rc::gen::inRange(0u, 3u)] << "\n";

            const unsigned input_length = *rc::gen::inRange(1u, 3000u);
            input_data.resize(0);
            generate_input_data(input_data, input_length, random);

            // cout << "input length: " << input_data.size() << "\n";

            if( print3 ) {
                for(auto w: input_data) {
                    cout <<"  o   " << w <<"\n";
                }
            }

            // if( i == 1 || i == 3 || i == 5) {
            //     unsigned choice = (unsigned[3]){0,32,48}[*rc::gen::inRange(0u, 3u)];
            //     cout << "changing to code: " << choice << "\n";
            //     change_code(tx, airRx, choice);
            // }


            uint8_t seq;
            std::vector<uint32_t> d2 = tx.transform(input_data, seq, 0);
	    RC_ASSERT(d2.size() == tx.getLengthForInput(input_length));
            // cout << "transform length: " << d2.size() << "\n";

            if( print3 ) {
                for(const auto w : d2) {
                    cout << HEX32_STRING(w) << "\n";
                }
            }

            tx.padData(d2);
            // cout << "final pad length: " << d2.size() << "\n\n";

            auto sliced_words = tx.emulateHiggsToHiggs(d2);

            uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

            for(int i = 0; i < 128; i++) {
                final_counter++;
                sliced_words[0].push_back(final_counter);
                sliced_words[1].push_back(0);
            }


            // #define DATA_TONE_NUM 32
            // WARNING this class was hand designed around 32 full iq subcarriers
            // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
            // airRx.data_tone_num = DATA_TONE_NUM;


            std::vector<uint32_t> output_data;
            bool found_something = false;
            bool found_something_again = false;
            unsigned do_erase;
            uint32_t found_at_frame;
            unsigned found_at_index; // found_i
            uint32_t found_length;
            air_header_t found_header;

            airRx.demodulateSlicedSimple(
                sliced_words,
                output_data,
                found_something,
                found_length,
                found_at_frame,
                found_at_index,
                found_header,
                do_erase
                );

            RC_ASSERT(found_something);

            if( print2 ) {
                std::cout << std::endl << "output_data:" << std::endl;
                for( uint32_t w : output_data ) {
                    std::cout << HEX32_STRING(w) << std::endl;
                }
            }

            output_data.resize(input_data.size());

            RC_ASSERT( input_data == output_data );
            return;
        }


        // if( runs > 0) {
        //     exit(0);
        // }
        // runs++;

    });

    return !pass;
}
