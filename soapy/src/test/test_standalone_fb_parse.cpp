#include <rapidcheck.h>
#include <queue>
#include "AirPacket.hpp"
#include "cpp_utils.hpp"
// #include "AirMacBitrate.hpp"
#include "feedback_bus.h"
#include "StandAloneFbParse.hpp"
#include "EmulateMapmov.hpp"

using namespace std;

// internal type used as part of test
typedef std::function<void(const std::vector<uint32_t>&)> u32_vector_cb_t;


const auto randomCutVector = [] (const std::vector<uint32_t>& vec, u32_vector_cb_t cb) {
    unsigned progress = 0;
    while(progress < vec.size()) {
        const unsigned st = progress;
        const unsigned pull = *rc::gen::inRange(1u, 20u);

        unsigned len = pull;
        if( (len+st) > vec.size() ) {
            // cout << "final2\n";
            len = vec.size() - st;
        }

        // make a vector
        // pass to the callback
        std::vector<uint32_t> slice;
        slice.assign(vec.begin()+st, vec.begin()+st+len);
        cb(slice);

        progress += len;
    }
};


auto rr = [&] (void) {
    static unsigned i = 0;
    std::vector<uint32_t> v = {1,10,4,3};

    auto load = v[i];
    i = (i+1) % v.size();
    return load;
};


























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

































///
/// Test that feedback_mapmov_reverse_size() is working
/// 
bool test_mapmov_reverse_size(void);
bool test_mapmov_reverse_size(void) {


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    AirPacketInbound _airRx;
    // AirPacketInbound* const airRx = &_airRx;

    setup_air_packet(_airRx, _airTx);


    int exitRuns = -2;
    int runs = 0;
    return rc::check("Test MapMov reverse size", [&]() {
        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        };


        const uint32_t enabled_subcarriers = airTx->enabled_subcarriers;
        const uint32_t constellation = airTx->modulation_schema;
        // const uint32_t slicewc = airTx->get_sliced_word_count();

        std::vector<uint32_t> packet_words;

        const unsigned pre_pad_word_count = *rc::gen::inRange(1u, 99999u);

        for(unsigned i = 0; i < pre_pad_word_count; i++) {
            packet_words.push_back(0xff000000 | i);
        }

        std::vector<uint32_t> packet_tfm;

        uint8_t seq;

        packet_tfm = airTx->transform(packet_words, seq);
        airTx->padData(packet_tfm);


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


        uint32_t res = feedback_mapmov_reverse_size(packet[1],enabled_subcarriers,constellation);

        RC_ASSERT(res == packet_tfm.size());

        runs++;
    });
}













///
/// Test that feedback_word_length() works.  Also tests that the default behavior also works
/// 
bool test_feedback_word_length(void);
bool test_feedback_word_length(void) {


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    AirPacketInbound _airRx;
    // AirPacketInbound* const airRx = &_airRx;

    setup_air_packet(_airRx, _airTx);


    // exit(0);
    int exitRuns = -2;
    // exitRuns = 3;
    int runs = 0;
    return rc::check("Test MapMov feedback_word_length", [&]() {
        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        };


        const uint32_t enabled_subcarriers = airTx->enabled_subcarriers;
        const uint32_t constellation = airTx->modulation_schema;
        // const uint32_t slicewc = airTx->get_sliced_word_count();


        std::vector<uint32_t> packet_words;
        // std::vector<uint32_t> demodulated_data;

        const unsigned pre_pad_word_count = *rc::gen::inRange(1u, 99999u);

        for(unsigned i = 0; i < pre_pad_word_count; i++) {
            packet_words.push_back(0xff000000 | i);
        }

        std::vector<uint32_t> packet_tfm;

        uint8_t seq;

        packet_tfm = airTx->transform(packet_words, seq);
        airTx->padData(packet_tfm);


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


        // uint32_t res = feedback_mapmov_reverse_size(packet[1],enabled_subcarriers,constellation);

        // RC_ASSERT(res == packet_tfm.size());


        const feedback_frame_t* v = (feedback_frame_t*)packet.data();
        bool error;
        bool was_ud;
        const auto len = feedback_word_length(v, &error, enabled_subcarriers, constellation, &was_ud);

        RC_ASSERT(!error);
        RC_ASSERT(len == packet.size());
        RC_ASSERT(was_ud);

        // cout << "Error: " << ((int)error) << "\n";
        // cout << "len: " << len << "\n";


        const auto lenWrong = feedback_word_length(v, &error);

        // prove the negative as well
        RC_ASSERT(lenWrong != packet.size());

        runs++;
    });
}





















bool test_air_emulate_consistent(void);
bool test_air_emulate_consistent(void) {


    cout << "hi" << "\n";


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    simpleSettings(_airTx);

    // exit(0);
    int exitRuns = -1;
    // exitRuns = 3;
    int runs = 0;
    return rc::check("Test AirPacket EmulateMapmov are consistent", [&]() {
        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        };

        std::vector<uint32_t> check = {
MAPMOV_SUBCARRIER_128    ,
MAPMOV_SUBCARRIER_320    ,
MAPMOV_SUBCARRIER_64_LIN
        };

        for(const auto mode : check) {

            EmulateMapmov _emulate;
            EmulateMapmov* emulate_mapmov = &_emulate;

            emulate_mapmov->set_subcarrier_allocation(mode);

            airTx->set_subcarrier_allocation(mode);

            auto airGot = airTx->get_enabled_subcarriers();

            auto emulateGot = emulate_mapmov->get_enabled_subcarriers();

            cout << "enum: " << mode << " enabled_subcarriers " << airGot << "\n";


            RC_ASSERT(airGot == emulateGot); 
       }



        runs++;
    });
}














































bool test_with_mapmov_unexpanded(void);
bool test_with_mapmov_unexpanded(void) {


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    AirPacketInbound _airRx;
    // AirPacketInbound* const airRx = &_airRx;

    setup_air_packet(_airRx, _airTx);

    // exit(0);
    int exitRuns = -2;
    // exitRuns = 3;
    int runs = 0;
    return rc::check("Test mapmov without expanding", [&]() {

        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        }

        bool print = false;
        bool print1 = false;
        bool print3 = false;
        // bool print2 = true;

        if( print || print1 || print3 ) {
            cout << "\n\n\n";
        }


        const uint32_t enabled_subcarriers = airTx->get_enabled_subcarriers();
        const uint32_t constellation = airTx->get_modulation_schema();
        // const uint32_t slicewc = airTx->get_sliced_word_count();
        const uint16_t allocation = airTx->get_subcarrier_allocation();


        std::vector<std::vector<uint32_t>> idealBody;
        std::vector<std::vector<uint32_t>> idealTotal;

        auto addVector = [&] (void) {


            std::vector<uint32_t> packet_words;
            std::vector<uint32_t> demodulated_data;

            const unsigned unpad_words = *rc::gen::inRange(1u, 99999u);

            for(unsigned i = 0; i < unpad_words; i++) {
                packet_words.push_back(0xff000000 | i);
            }

            std::vector<uint32_t> packet_tfm;

            uint8_t seq;

            packet_tfm = airTx->transform(packet_words, seq);
            airTx->padData(packet_tfm);


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

            idealBody.push_back(packet_tfm);
            idealTotal.push_back(packet);

        };




        const unsigned vectors = *rc::gen::inRange(1u, 5u);

        for(unsigned i = 0; i < vectors; i++) {
            addVector();
        }

        std::vector<uint32_t> allWords;
        for( const auto x : idealTotal ) {
            allWords.insert(allWords.end(), x.begin(), x.end());
        }



        unsigned vectors_got = 0;

        // maybePrintIdeal(mono);
        StandAloneFbParse parse;

        // cout << "a\n";

        parse.print3 = false;
        parse.print4 = false;
        parse.print5 = false;

        // auto xxxx = ;
        // cout << "b\n";


        parse.set_mapmov(allocation, constellation);
        // cout << "c\n";
        parse.enable_expand_mapmov = false;

        parse.registerCb([&](const uint32_t t0, const uint32_t t1, const std::vector<uint32_t>& header, const std::vector<uint32_t>& body) {
            if( print3 ) {

                cout << "got type 0: " << t0 << " type 1: " << t1 << "\n";
            }
            if( print1 ) {
                cout << "--------------\n";
                for( const auto x : header ) {
                    cout << HEX32_STRING(x) << "\n";
                }
                cout << "--------------\n";

                for( const auto x : body ) {
                    cout << HEX32_STRING(x) << "\n";
                }

                cout << "\n\n";
            }

            RC_ASSERT(body == idealBody[vectors_got]);
            vectors_got++;
        });


        randomCutVector(allWords, [&](const std::vector<uint32_t>& slice){
            // cout << "------------------\n";
            // for( const auto x : slice ) {
            //     cout << HEX32_STRING(x) << "\n";
            // }
            parse.addWords(slice);
            parse.parse();
        });


        RC_ASSERT(vectors_got == idealBody.size());


        runs++;
    });
}












bool test_with_mapmov_expanded(void);
bool test_with_mapmov_expanded(void) {


    AirPacketOutbound _airTx;
    AirPacketOutbound* const airTx = &_airTx;

    AirPacketInbound _airRx;
    // AirPacketInbound* const airRx = &_airRx;

    setup_air_packet(_airRx, _airTx);

    // exit(0);
    int exitRuns = -2;
    // exitRuns = 3;
    int runs = 0;
    return rc::check("Test mapmov with expanding", [&]() {

        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        }

        bool print1 = false;
        bool print3 = false;
        // bool print2 = true;

        if(print1 || print3 ) {
            cout << "\n\n\n";
        }


        const uint32_t enabled_subcarriers = airTx->get_enabled_subcarriers();
        const uint32_t constellation = airTx->get_modulation_schema();
        // const uint32_t slicewc = airTx->get_sliced_word_count();
        const uint16_t allocation = airTx->get_subcarrier_allocation();


        std::vector<std::vector<uint32_t>> idealBody;
        std::vector<std::vector<uint32_t>> idealTotal;

        auto addVector = [&] (void) {


            std::vector<uint32_t> packet_words;
            std::vector<uint32_t> demodulated_data;

            for(unsigned i = 0; i < 20; i++) {
                packet_words.push_back(0xff000000 | i);
            }

            std::vector<uint32_t> packet_tfm;

            uint8_t seq;

            packet_tfm = airTx->transform(packet_words, seq);
            airTx->padData(packet_tfm);


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

            idealBody.push_back(packet_tfm);
            idealTotal.push_back(packet);
        };


        auto addZeros = [&] (void) {
            const unsigned zeros = *rc::gen::inRange(1u, 128u);
            // const unsigned zeros = 32;

            std::vector<uint32_t> zeroBunch;

            for(unsigned i = 0; i < zeros; i++) {
                zeroBunch.push_back(0);
            }

            idealTotal.push_back(zeroBunch);
        };




        // const unsigned vectors = *rc::gen::inRange(1u, 5u);
        const unsigned vectors = 2;

        for(unsigned i = 0; i < vectors; i++) {

            const bool pre_zero = *rc::gen::inRange(0u, 2u);
            if(pre_zero) {
                addZeros();
            }


            addVector();


            const bool post_zero = *rc::gen::inRange(0u, 2u);
            if(post_zero) {
                addZeros();
            }
        }

        std::vector<uint32_t> allWords;
        for( const auto x : idealTotal ) {
            allWords.insert(allWords.end(), x.begin(), x.end());
        }



        unsigned vectors_got = 0;

        // maybePrintIdeal(mono);
        StandAloneFbParse parse;

        // cout << "a\n";

        parse.print3            = false;
        parse.print4            = false;
        parse.print5            = false;
        parse.fbp_print_jamming = false;

        // auto xxxx = ;
        // cout << "b\n";


        parse.set_mapmov(allocation, constellation);
        // cout << "c\n";
        // parse.enable_expand_mapmov = false;

        parse.registerCb([&](const uint32_t t0, const uint32_t t1, const std::vector<uint32_t>& header, const std::vector<uint32_t>& body) {
            if( print3 ) {

                cout << "got type 0: " << t0 << " type 1: " << t1 << "\n";
            }
            if( print1 ) {
                cout << "--------------\n";
                for( const auto x : header ) {
                    cout << HEX32_STRING(x) << "\n";
                }
                cout << "--------------\n";

                for( const auto x : body ) {
                    cout << HEX32_STRING(x) << "\n";
                }

                cout << "\n\n";
            }

            // RC_ASSERT(body == idealBody[vectors_got]);
            vectors_got++;
        });


        randomCutVector(allWords, [&](const std::vector<uint32_t>& slice){
            // cout << "------------------\n";
            // for( const auto x : slice ) {
            //     cout << HEX32_STRING(x) << "\n";
            // }
            parse.addWords(slice);
            parse.parse();
        });


        RC_ASSERT(vectors_got == idealBody.size());


        runs++;
    });
}



































bool test_standalone_parse(void);
bool test_standalone_parse(void) {


    cout << "hi" << "\n";

    // exit(0);
    int exitRuns = 1;
    // exitRuns = 3;
    int runs = 0;
    return rc::check("Test FB Split", [&]() {
        if(runs > exitRuns && (exitRuns != -2)) {
            return;
        };

        bool print = false;
        bool print1 = false;
        bool print3 = false;
        // bool print2 = true;

        if( print || print1 || print3 ) {

            cout << "\n\n\n";
        }


        std::vector<std::vector<uint32_t>> idealBody;
        std::vector<std::vector<uint32_t>> idealTotal;

        auto addVector = [&] (void) {
            const unsigned pick = 0;


            switch(pick) {
                case 0:
                    {
                        const unsigned sz = *rc::gen::inRange(1u, 999u);
                        std::vector<uint32_t> zrs;
                        // zrs.resize(sz);
                        for(size_t i = 0; i < sz; i++) {
                           zrs.push_back(0xff000000 + i);
                        }

                        auto packet = feedback_vector_packet(
                            FEEDBACK_VEC_TX_EQ,
                            zrs,
                            1,
                            FEEDBACK_DST_HIGGS);

                        idealBody.push_back(zrs);
                        idealTotal.push_back(packet);
                    }
                    break;
            }
        };




        const unsigned vectors = *rc::gen::inRange(1u, 5u);

        for(unsigned i = 0; i < vectors; i++) {
            addVector();
        }

        std::vector<uint32_t> allWords;
        if( 1 ) {
            for( const auto x : idealTotal ) {
                allWords.insert(allWords.end(), x.begin(), x.end());
            }
        } else {
            for(unsigned i = 0; i < 0x20; i++) {
                allWords.push_back(i);
            }
        }



        

        unsigned vectors_got = 0;

        // maybePrintIdeal(mono);
        StandAloneFbParse parse;

        parse.registerCb([&](const uint32_t t0, const uint32_t t1, const std::vector<uint32_t>& header, const std::vector<uint32_t>& body) {
            if( print3 ) {

                cout << "got type 0: " << t0 << " type 1: " << t1 << "\n";
            }
            if( print1 ) {
                cout << "--------------\n";
                for( const auto x : header ) {
                    cout << HEX32_STRING(x) << "\n";
                }
                cout << "--------------\n";

                for( const auto x : body ) {
                    cout << HEX32_STRING(x) << "\n";
                }

                cout << "\n\n";
            }

            RC_ASSERT(body == idealBody[vectors_got]);
            vectors_got++;
        });


        randomCutVector(allWords, [&](const std::vector<uint32_t>& slice){
            // cout << "------------------\n";
            // for( const auto x : slice ) {
            //     cout << HEX32_STRING(x) << "\n";
            // }
            parse.addWords(slice);
            parse.parse();
        });


        RC_ASSERT(vectors_got == idealBody.size());

        runs++;
    });
}






int main(int argc, char const *argv[]) {
    bool pass = true;
    pass &= test_air_emulate_consistent();
    pass &= test_standalone_parse();
    pass &= test_with_mapmov_unexpanded();
    pass &= test_with_mapmov_expanded();
    pass &= test_mapmov_reverse_size();
    pass &= test_feedback_word_length();
    return !pass;
}
