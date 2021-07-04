#include <rapidcheck.h>
#include <queue>
#include "AirPacket.hpp"
#include "cpp_utils.hpp"
// #include "AirMacBitrate.hpp"
#include "feedback_bus.h"
#include "EmulateMapmov.hpp"

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

















bool test_emulate_solo(void);
bool test_emulate_solo(void) {

    cout << "hi" << "\n";


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    AirPacketInbound _airRx;
    AirPacketInbound* const airRx = &_airRx;


    // exit(0);
    int exitRuns = -1;
    int runs = 0;
    return rc::check("Test Emulate Mapmov", [&]() {

        bool print = false;
        bool print2 = true;
        bool print3 = false;
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





        // simpleSettings(_airTx);
        setup_air_packet(_airRx, _airTx);
        // std::vector<std::vector<uint32_t>> sliced_words;
        // const uint32_t max_frame = 66;



        const uint32_t enabled_subcarriers = airTx->enabled_subcarriers;
        const uint32_t constellation = airTx->modulation_schema;
        const uint32_t slicewc = airTx->get_sliced_word_count();

        // const uint32_t enabled_subcarriers = 20;

        cout << "enabled_subcarriers " << enabled_subcarriers << "\n";

        std::vector<uint32_t> packet_words;
        std::vector<uint32_t> demodulated_data;

        for(unsigned i = 0; i < 20; i++) {
            packet_words.push_back(0xff000000 | i);
        }

        std::vector<uint32_t> packet_tfm;

        uint8_t seq;

        packet_tfm = airTx->transform(packet_words, seq);
        airTx->padData(packet_tfm);


        EmulateMapmov _emulate;
        EmulateMapmov* emulate_mapmov = &_emulate;

        emulate_mapmov->set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);


        
            const auto a = packet_tfm;
            std::vector<uint32_t> mapmov_mapped;
            std::vector<uint32_t> mapmov_out;
            std::vector<uint32_t> rx_out;
            std::vector<uint32_t> vmem_mover_out;
            std::vector<uint32_t> sliced_out_data;
            sliced_buffer_t final_rx_data;

            // Call all the same functions as before, but to it on the object
            emulate_mapmov->map_input_data(a, mapmov_mapped);
            emulate_mapmov->move_input_data(mapmov_mapped, mapmov_out);
            rx_out.resize(mapmov_out.size());
            emulate_mapmov->receive_tx_data(rx_out, mapmov_out);
            emulate_mapmov->rx_reverse_mover(vmem_mover_out, rx_out);
            emulate_mapmov->slice_rx_data(vmem_mover_out, sliced_out_data);
        


        for(const auto y : mapmov_out) {
            cout << HEX32_STRING(y) << "\n";
        }

        cout << "\n\n";

        for(const auto y : sliced_out_data) {
            cout << HEX32_STRING(y) << "\n";
        }

/*

        auto packet = 
            feedback_vector_mapmov_scheduled_sized(
                FEEDBACK_VEC_TX_USER_DATA, 
                packet_tfm, 
                enabled_subcarriers, 
                0,
                FEEDBACK_DST_HIGGS,
                0, // timeslot fillin, overwritten later
                0,  // epoc fillin, overwritten later
                constellation
                );


        for(const auto y : packet) {
            cout << HEX32_STRING(y) << "\n";
        }

        std::vector<std::vector<uint32_t>> sliced_words;

        uint32_t lifetime;

        lifetime = get_sliced_data_outside(_airTx, sliced_words, packet_words);


        for(unsigned i = 0; i < packet.size(); i++) {
            // cout << HEX32_STRING(y) << "\n";
        }


        cout << "\n\n\n";

        for(const auto x : sliced_words[0] ) {
            cout << HEX32_STRING(x) << "\n";
        }

        for(const auto x : sliced_words[1] ) {
            cout << HEX32_STRING(x) << "\n";
        }




        lifetime = append_zeros(sliced_words, 25, lifetime);
        demodulate_data(_airRx, sliced_words, demodulated_data);

        cout << "\n\n---------\n\n";

        for(const auto y : demodulated_data) {
            cout << HEX32_STRING(y) << "\n";
        }

        RC_ASSERT(demodulated_data == packet_words);

*/


        if(runs > exitRuns && (exitRuns != -2) ) {
            exit(0);
        }
        runs++;
    });
}















bool test_emulate_conjoined(void);
bool test_emulate_conjoined(void) {

    cout << "hi" << "\n";

    // exit(0);
    int exitRuns = -2;
    int runs = 0;
    return rc::check("Test Emulate Mapmov", [&]() {

        bool print = false;
        bool print2 = true;
        bool print3 = false;
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


        // AirPacketInbound rx_packet;

        AirPacketOutbound _airTx;
        AirPacketOutbound* const airTx = &_airTx;

        AirPacketInbound _airRx;
        AirPacketInbound* const airRx = &_airRx;



        // simpleSettings(_airTx);
        setup_air_packet(_airRx, _airTx);
        // std::vector<std::vector<uint32_t>> sliced_words;
        // const uint32_t max_frame = 66;



        const uint32_t enabled_subcarriers = airTx->enabled_subcarriers;
        const uint32_t constellation = airTx->modulation_schema;
        const uint32_t slicewc = airTx->get_sliced_word_count();
        // const uint32_t constellation = FEEDBACK_MAPMOV_QPSK;

        // const uint32_t enabled_subcarriers = 20;

        cout << "enabled_subcarriers " << enabled_subcarriers << "\n";

        std::vector<uint32_t> packet_words;
        std::vector<uint32_t> demodulated_data;

        for(unsigned i = 0; i < 20; i++) {
            packet_words.push_back(0xff000000 | i);
        }

        std::vector<uint32_t> packet_tfm;

        uint8_t seq;

        packet_tfm = airTx->transform(packet_words, seq);


        auto packet = 
            feedback_vector_mapmov_scheduled_sized(
                FEEDBACK_VEC_TX_USER_DATA, 
                packet_tfm, 
                enabled_subcarriers, 
                0,
                FEEDBACK_DST_HIGGS,
                0, // timeslot fillin, overwritten later
                0,  // epoc fillin, overwritten later
                constellation
                );


        for(const auto y : packet) {
            cout << HEX32_STRING(y) << "\n";
        }

        std::vector<std::vector<uint32_t>> sliced_words;

        uint32_t lifetime;

        lifetime = get_sliced_data_outside(_airTx, sliced_words, packet_words);


        for(unsigned i = 0; i < packet.size(); i++) {
            // cout << HEX32_STRING(y) << "\n";
        }


        cout << "\n\n\n";

        for(const auto x : sliced_words[0] ) {
            cout << HEX32_STRING(x) << "\n";
        }

        for(const auto x : sliced_words[1] ) {
            cout << HEX32_STRING(x) << "\n";
        }




        lifetime = append_zeros(sliced_words, 25, lifetime);
        demodulate_data(_airRx, sliced_words, demodulated_data);

        cout << "\n\n---------\n\n";

        for(const auto y : demodulated_data) {
            cout << HEX32_STRING(y) << "\n";
        }

        RC_ASSERT(demodulated_data == packet_words);




        if(runs > exitRuns && (exitRuns != -2) ) {
            exit(0);
        };
        runs++;
    });
}




int main(int argc, char const *argv[]) {
    bool pass = true;
    pass &= test_emulate_conjoined();
    pass &= test_emulate_solo();
    // pass &= test_interface2();
    return !pass;
}
