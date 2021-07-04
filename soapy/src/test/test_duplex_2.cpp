#include <rapidcheck.h>
#include <queue>
#include "AirPacket.hpp"
#include "cpp_utils.hpp"
#include "AirMacBitrate.hpp"

#define INPUT_DATA_SIZE        10000
#define FRAME_WORD_SIZE        20
#define MAX_MBPS               15.625

#define DUPLEX_START_FRAME (DUPLEX_START_FRAME_TX)
#define DATA_FRAME_SIZE (20)

using namespace std;

/**
 * This method populates a standard vector with random or counter data
 *
 * @param[in,out] input_data: Vector to populate data
 * @param[in] data_length: The amount of data to populate
 * @param[in] random: A boolean of whether the data should be random
 */
void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random);

/**
 *
 */
uint32_t append_zeros(std::vector<std::vector<uint32_t>> &sliced_words,
                      std::size_t frame_count,
                      uint32_t seq_num);

/**
 *
 */
uint32_t append_data(std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &modulated_data,
                     uint32_t seq_num);


/**
 *
 */
uint32_t update_data_queue(std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           uint32_t seq_num);

/**
 *
 */
uint32_t create_data_stream(std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           std::size_t packet_count,
                           uint32_t seq_num);

/**
 *
 */
void update_sliced_words(std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count);




void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random) {
    uint32_t data;
    for(uint32_t i = 0; i < data_length; i++) {
        data = random ?  *rc::gen::inRange(0u, 0xffffffffu) : i;
        input_data.push_back(data);
    }
}

void generate_input_data_base(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         uint32_t base,
                         bool random=false) {
    uint32_t data;
    for(uint32_t i = 0; i < data_length; i++) {
        data = random ?  *rc::gen::inRange(0u, 0xffffffffu) : i;
        input_data.push_back(base+data);
    }
}


template <typename T>
void simpleSettings(T& xx_packet) {
    // int code_bad;
    // uint32_t code_length = 255;
    // uint32_t fec_length = 48;
    xx_packet.print_settings_did_change = false;

    // TX Air Packet setup
    xx_packet.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    xx_packet.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);

    // code_bad = xx_packet.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
    //                                    code_length,
    //                                    fec_length);

    xx_packet.set_interleave(0);
    xx_packet.set_duplex_mode(true);

    // xx_packet.enablePrint("mod");
    // xx_packet.enablePrint("demod");
    // xx_packet.enablePrint("all");
    xx_packet.print25 = true;
    xx_packet.print26 = true;

    xx_packet.disablePrint("all");



}


void setup_air_packet(AirPacketInbound &rx_packet,
                      AirPacketOutbound &tx_packet) {
    simpleSettings(rx_packet);
    simpleSettings(tx_packet);
}

///
/// @param [inout] sliced_words is added to
/// @param [out] randomized_data, do not pass randomized_data with any value
uint32_t get_sliced_data(AirPacketOutbound &tx_packet,
                         const std::size_t data_size,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::vector<uint32_t> &randomized_data,
                         const uint32_t base = 0) {
    uint8_t seq = DUPLEX_START_FRAME;
    uint32_t seq_num = DUPLEX_START_FRAME;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> zeros;
    std::vector<uint32_t> header(FRAME_WORD_SIZE, 0);
    // generate_input_data(randomized_data, data_size, false);
    generate_input_data_base(randomized_data, data_size, base, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data, seq);

    // Updating header with correct sequence value
    for (std::size_t i = 0; i < sliced_words[0].size() / FRAME_WORD_SIZE; i++) {
        sliced_words[0][i * FRAME_WORD_SIZE] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
    }

    return seq_num;
}


///
/// @param [inout] sliced_words is added to
/// @param [in] pass data
uint32_t get_sliced_data_outside(AirPacketOutbound &tx_packet,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         const std::vector<uint32_t> &randomized_data) {
    uint8_t seq = DUPLEX_START_FRAME;
    uint32_t seq_num = DUPLEX_START_FRAME;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> zeros;
    std::vector<uint32_t> header(FRAME_WORD_SIZE, 0);
    // generate_input_data(randomized_data, data_size, false);
    // generate_input_data_base(randomized_data, data_size, base, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data, seq);

    // Updating header with correct sequence value
    for (std::size_t i = 0; i < sliced_words[0].size() / FRAME_WORD_SIZE; i++) {
        sliced_words[0][i * FRAME_WORD_SIZE] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
    }

    return seq_num;
}


uint32_t append_zeros(std::vector<std::vector<uint32_t>> &sliced_words,
                      std::size_t frame_count,
                      uint32_t seq_num) {
    std::vector<uint32_t> header(FRAME_WORD_SIZE, 0);
    std::vector<uint32_t> zeros(FRAME_WORD_SIZE, 0);

    for (std::size_t i = 0; i < frame_count; i++) {
        header[0] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
        sliced_words[0].insert(sliced_words[0].end(),
                               header.begin(),
                               header.end());
        sliced_words[1].insert(sliced_words[1].end(),
                               zeros.begin(),
                               zeros.end());
    }

    return seq_num;
}

uint32_t append_data(std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &modulated_data,
                     uint32_t seq_num) {
    std::vector<uint32_t> header(FRAME_WORD_SIZE, 0);
    std::size_t frame_count = modulated_data.size() / FRAME_WORD_SIZE;

    for (std::size_t i = 0; i < frame_count; i++) {
        header[0] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
        sliced_words[0].insert(sliced_words[0].end(),
                               header.begin(),
                               header.end());
        sliced_words[1].insert(
                        sliced_words[1].end(),
                        modulated_data.begin() + (i * FRAME_WORD_SIZE),
                        modulated_data.begin() + ((i + 1) * FRAME_WORD_SIZE));
    }

    return seq_num;
}

void demodulate_data(AirPacketInbound &air_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &demodulated_data, bool print=false) {
    bool found_something = false;
    bool found_something_again = false;
    unsigned do_erase = 0;
    static uint32_t found_packet = 0;
    unsigned do_erase_sliced = 0;
    uint32_t found_at_frame;
    unsigned found_at_index;
    uint32_t found_length;
    air_header_t found_header;
    air_packet.demodulateSlicedHeader(sliced_words,
                                      found_something,
                                      found_length,
                                      found_at_frame,
                                      found_at_index,
                                      found_header,
                                      do_erase);
    if (found_something) {
        std::vector<uint32_t> potential_words_again;
        uint32_t force_detect_index = 
                 air_packet.getForceDetectIndex(found_at_index);
        uint32_t found_at_frame_again;
        unsigned found_at_index_again;
        air_packet.demodulateSliced(sliced_words,
                                    potential_words_again,
                                    found_something_again,
                                    found_at_frame_again,
                                    found_at_index_again,
                                    do_erase_sliced,
                                    force_detect_index,
                                    found_length);
        demodulated_data = potential_words_again;
        if(!found_something_again) {
            return;
        }
        found_packet++;
        if( print ) {
            std::cout << "Packet Number: " << found_packet << "\n"
                      << "Packet Frame: " << found_at_frame << "\n"
                      << "Packet Size: " << potential_words_again.size() << "\n";
          }
    }
    if (do_erase || do_erase_sliced) {
        unsigned erase = do_erase < do_erase_sliced ? do_erase_sliced : do_erase;
        if( print ) {
            std::cout << "Erase Elements: " << erase << "\n";
        }
        sliced_words[0].erase(sliced_words[0].begin(),
                              sliced_words[0].begin() + erase);
        sliced_words[1].erase(sliced_words[1].begin(),
                              sliced_words[1].begin() + erase);
    }
}

uint32_t update_data_queue(std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           uint32_t seq_num) {
    uint32_t header;
    for (std::size_t i = 0; i < data.size(); i++) {
        if (!(i % DATA_FRAME_SIZE)) {
            header = seq_num;
            seq_num++;
            if (!(seq_num % DUPLEX_FRAMES)) {
                seq_num += DUPLEX_START_FRAME;
            }
        } else {
            header = 0;
        }
        header_queue.push(header);
        data_queue.push(data[i]);
    }

    return seq_num;
}

uint32_t create_data_stream(std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           std::size_t packet_count,
                           uint32_t seq_num) {
    std::size_t zero_size = DUPLEX_FRAMES - DUPLEX_START_FRAME -
    (data.size() % ((DUPLEX_FRAMES - DUPLEX_START_FRAME) * FRAME_WORD_SIZE)) /
    FRAME_WORD_SIZE;
    std::vector<uint32_t> zeros(zero_size * DATA_FRAME_SIZE, 0);

    for (std::size_t i = 0; i < packet_count; i++) {
        seq_num = update_data_queue(header_queue,
                                    data_queue,
                                    data,
                                    seq_num);
        seq_num = update_data_queue(header_queue,
                                    data_queue,
                                    zeros,
                                    seq_num);
    }
    zeros.resize((DUPLEX_FRAMES - DUPLEX_START_FRAME) * FRAME_WORD_SIZE, 0);
    seq_num = update_data_queue(header_queue,
                                data_queue,
                                zeros,
                                seq_num);
    return seq_num;
}

void update_sliced_words(std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count) {
    for (std::size_t j = 0; j < append_count * DATA_FRAME_SIZE; j++) {
        sliced_words[0].push_back(header_queue.front());
        sliced_words[1].push_back(data_queue.front());
        header_queue.pop();
        data_queue.pop();
    }
}

// template<typename T>
// constexpr T pi = T{3.14L};

// template<typename T>
// constexpr T pi = T{3.14L};


bool test_packet_for_ota() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t max_frame = 66;

    setup_air_packet(rx_packet, tx_packet);

// ?    cout << pi<float> << "\n";

    // exit(0);

    return rc::check("Test Short Packet Demodulation", [max_frame,
                                                        &rx_packet,
                                                        &tx_packet,
                                                        &sliced_words]() {


        
        bool print = false;
        bool print2 = true;
        uint32_t seq_num;
        std::vector<uint32_t> _randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::size_t frame_size = 3; //*rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * tx_packet.get_sliced_word_count();

        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  _randomized_data,
                                  0x77000000);
        const auto ideal_data = _randomized_data;

        if( print ) {
            cout << "seq_num " << seq_num << "\n";
            // for( const auto v : sliced_words) {
                for( const auto w : sliced_words[0] ) {
                    // cout << HEX32_STRING(w) << " ";
                    cout << w << " ";
                }
                cout << "\n";
                for( const auto w : sliced_words[1] ) {
                    cout << HEX32_STRING(w) << "\n";
                    // cout << w << " ";
                }
            // }
        }

        seq_num = append_zeros(sliced_words, 25, seq_num);

        if( print ) {
            cout << "seq_num " << seq_num << "\n";
        }


        demodulate_data(rx_packet, sliced_words, demodulated_data);

        if (print2) {
            cout << "ideal data:\n";
                            for( const auto w : ideal_data ) {
                    cout << HEX32_STRING(w) << "\n";
                    // cout << w << " ";
                }
        }

        RC_ASSERT(ideal_data == demodulated_data);
        exit(0);
    });
}





bool test_interface_simple() {

    // exit(0);
    int runs = 0;
    return rc::check("Test Internal cache", [&]() {
        AirMacBitrateTx dut;

        dut.setSeed(0x44);

        auto out_a = dut.generate(10);

        auto c = dut.getCache();

        cout << "out_a: " << "\n";
        for(const auto w : out_a ) {
            cout << HEX32_STRING(w) << "\n";
        }


        cout << "cache: " << "\n";
        for(const auto w : c ) {
            cout << HEX32_STRING(w) << "\n";
        }




        auto out_b = dut.generate(4);

        cout << "out_b: " << "\n";
        for(const auto w : out_b ) {
            cout << HEX32_STRING(w) << "\n";
        }


        cout << "cache: " << "\n";
        c = dut.getCache();
        for(const auto w : c ) {
            cout << HEX32_STRING(w) << "\n";
        }



        exit(0);
    });

}


// uint8_t hamming_32(const uint32_t a, const uint32_t b) {
//     uint8_t out = 0;
//     for(unsigned i = 0; i < 32; i++){
//         if( ((a>>i)&0x1) != ((b>>i)&0x1) ) {
//             out++;
//         }

//     }
//     return out;
// }

// bool test_ham() {

//     uint32_t a = 0xf0001000;
//     uint32_t b = a;//0xf0000000;

//     unsigned res = hamming_32(a,b);

//     cout << "res: " << res << "\n";

//     return true;
// }


bool test_interface() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    const uint32_t max_frame = 66;

    setup_air_packet(rx_packet, tx_packet);


    // exit(0);
    int runs = 0;
    return rc::check("Test Short Packet Demodulation", [&]() {



        bool print = true;
        bool print2 = true;
        uint32_t seq_num;


        auto maybePrintIdeal = [&] (const std::vector<uint32_t> foo) {

            if (print2) {
                cout << "ideal data:\n";
                                for( const auto w : foo ) {
                        cout << HEX32_STRING(w) << "\n";
                        // cout << w << " ";
                    }
            }
        };


        auto blackbox = [&] (const std::vector<uint32_t> ideal_data) {

            seq_num = get_sliced_data_outside(tx_packet,
                                      sliced_words,
                                      ideal_data);

            if( print ) {
                cout << "seq_num " << seq_num << "\n";
                // for( const auto v : sliced_words) {
                    for( const auto w : sliced_words[0] ) {
                        // cout << HEX32_STRING(w) << " ";
                        cout << w << " ";
                    }
                    cout << "\n";
                    for( const auto w : sliced_words[1] ) {
                        cout << HEX32_STRING(w) << "\n";
                        // cout << w << " ";
                    }
                // }
            }

            seq_num = append_zeros(sliced_words, 25, seq_num);

            if( print ) {
                cout << "seq_num " << seq_num << "\n";
            }


            std::vector<uint32_t> bb_got;
            demodulate_data(rx_packet, sliced_words, bb_got);
            return bb_got;
        };


        AirMacBitrateTx duta;
        duta.setSeed(0x44);

        AirMacBitrateRx dutb;
        dutb.setSeed(0x44);

        // uint32_t lifetime32 = 0xf000;
        // const uint32_t lifetime32_bump = 0x10000 + 77;


        const std::size_t frame_size = 3; //*rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * tx_packet.get_sliced_word_count();

        data_size = 1000000/32;

        data_size = 20*10;

        dutb.mac_print2 = true;


        const uint32_t count = 2;
        for(uint32_t i = 0; i < count; i++) {


            const std::vector<uint32_t> ideal_data = duta.generate(data_size, i);
            maybePrintIdeal(ideal_data);

            std::vector<uint32_t> transmitted_data = ideal_data;

            uint32_t add = *rc::gen::inRange(0u, 0x10u);

            // 3 gives chance not to run
            for(unsigned j = 3; j < add; j++) {
                const uint32_t v = *rc::gen::inRange(0u, 0xffffffffu);
                transmitted_data.push_back(v);
            }

            uint32_t sub = *rc::gen::inRange(0u, 0x10u);

            // 3 gives chance not to run
            for(unsigned j = 3; j < sub; j++) {
                transmitted_data.erase(transmitted_data.end());
            }

            uint32_t mod = *rc::gen::inRange(0u, 0x10u);

            if( mod < 3 ) {
                uint32_t loc = *rc::gen::inRange(0u, 0xffffffffu);

                loc = loc % transmitted_data.size();

                uint32_t flip = *rc::gen::inRange(0u, 0xffffffffu);

                transmitted_data[loc] ^= flip;
            }

            
            // transmitted_data[3] ^= 0xffffffff;
            // transmitted_data[1000] ^= 0xffffffff;

            std::vector<uint32_t> demodulated = blackbox(transmitted_data);

            dutb.got(demodulated);

            bool same = ideal_data != demodulated;

            cout << "packet " << i << (same?" had errors " : " ok ") << "\n" << "\n";

            // lifetime32 += lifetime32_bump;
            usleep(1000*250);
        }


        exit(0);

        // RC_ASSERT(ideal_data == demodulated);
        if(runs > 4) {
            exit(0);
        };
        runs++;
    });
}







bool test_interface2() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    const uint32_t max_frame = 66;

    setup_air_packet(rx_packet, tx_packet);


    // exit(0);
    int runs = 0;
    return rc::check("Test Short Packet Demodulation", [&]() {



        bool print = false;
        bool print2 = true;
        uint32_t seq_num;


        auto maybePrintIdeal = [&] (const std::vector<uint32_t> foo) {

            if (print2) {
                cout << "ideal data:\n";
                                for( const auto w : foo ) {
                        cout << HEX32_STRING(w) << "\n";
                        // cout << w << " ";
                    }
            }
        };


        auto blackbox = [&] (const std::vector<uint32_t> ideal_data) {

            seq_num = get_sliced_data_outside(tx_packet,
                                      sliced_words,
                                      ideal_data);

            if( print ) {
                cout << "seq_num " << seq_num << "\n";
                // for( const auto v : sliced_words) {
                    for( const auto w : sliced_words[0] ) {
                        // cout << HEX32_STRING(w) << " ";
                        cout << w << " ";
                    }
                    cout << "\n";
                    for( const auto w : sliced_words[1] ) {
                        cout << HEX32_STRING(w) << "\n";
                        // cout << w << " ";
                    }
                // }
            }

            seq_num = append_zeros(sliced_words, 25, seq_num);

            if( print ) {
                cout << "seq_num " << seq_num << "\n";
            }


            std::vector<uint32_t> bb_got;
            demodulate_data(rx_packet, sliced_words, bb_got);
            return bb_got;
        };


        AirMacBitrateTx duta;
        duta.setSeed(0x44);

        AirMacBitrateRx dutb;
        dutb.setSeed(0x44);

        // uint32_t lifetime32 = 0xf000;
        // const uint32_t lifetime32_bump = 0x10000 + 77;


        const std::size_t frame_size = 3; //*rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * tx_packet.get_sliced_word_count();

        data_size = 1000000/32;

        data_size = 20*10;


        const uint32_t count = 4;
        for(uint32_t i = 0; i < count; i++) {


            const std::vector<uint32_t> ideal_data = duta.generate(data_size, i);
            maybePrintIdeal(ideal_data);

            std::vector<uint32_t> transmitted_data = ideal_data;

            // if( RC)

            std::vector<uint32_t> demodulated = blackbox(transmitted_data);

            dutb.got(demodulated);

            bool same = ideal_data != demodulated;

            cout << "packet " << i << (same?" had errors " : " ok ") << "\n" << "\n";

            // lifetime32 += lifetime32_bump;
            usleep(1000*250);
        }


        exit(0);

        // RC_ASSERT(ideal_data == demodulated);
        if(runs > 4) {
            exit(0);
        };
        runs++;
    });
}






int main(int argc, char const *argv[]) {
    bool pass = true;
    // pass &= test_packet_for_ota();
    // pass &= test_interface_simple();
    // pass &= test_ham();
    pass &= test_interface();
    // pass &= test_interface2();
    return !pass;
}