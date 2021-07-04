#include <rapidcheck.h>
#include <iostream>
#include <vector>
#include "duplex_schedule.h"
#include "driver/AirPacket.hpp"

#define FRAME_WORD_SIZE         (20)
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
void setup_air_packet(AirPacketInbound &rx_packet,
                      AirPacketOutbound &tx_packet);

/**
 *
 */
uint32_t get_sliced_data(AirPacketOutbound &tx_packet,
                         std::size_t data_size,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::vector<uint32_t> &randomized_data);

/**
 *
 */
uint32_t append_zeros(std::vector<std::vector<uint32_t>> &sliced_words,
                      std::size_t frame_count,
                      uint32_t seq_num);

/**
 *
 */
void demodulate_data(AirPacketInbound &air_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &demodulated_data);

/**
 *
 */
bool test_short_packet_demodulation();

/**
 *
 */
bool test_medium_packet_demodulation();

/**
 *
 */
bool test_long_packet_demodulation();

int main(int argc, char const *argv[]) {
    bool pass = true;

    pass &= test_short_packet_demodulation();
    pass &= test_medium_packet_demodulation();
    pass &= test_long_packet_demodulation();

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

void setup_air_packet(AirPacketInbound &rx_packet,
                      AirPacketOutbound &tx_packet) {
    int code_bad;
    uint32_t code_length = 255;
    uint32_t fec_length = 48;
    bool duplex_mode = true;

    // TX Air Packet setup
    tx_packet.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    tx_packet.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    code_bad = tx_packet.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
                                       code_length,
                                       fec_length);
    tx_packet.set_interleave(0);
    tx_packet.set_duplex_mode(duplex_mode);
    tx_packet.print_settings_did_change = true;
    // RX Air Packet setup
    rx_packet.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    rx_packet.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    code_bad = rx_packet.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
                                       code_length,
                                       fec_length);
    rx_packet.set_interleave(0);
    rx_packet.set_duplex_mode(duplex_mode);
    rx_packet.set_duplex_start_frame(DUPLEX_START_FRAME_RX);
    rx_packet.print_settings_did_change = true;
}

uint32_t get_sliced_data(AirPacketOutbound &tx_packet,
                         std::size_t data_size,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::vector<uint32_t> &randomized_data) {
    uint8_t seq = DUPLEX_START_FRAME_RX;
    uint32_t seq_num = DUPLEX_START_FRAME_RX;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> zeros;
    std::vector<uint32_t> header(FRAME_WORD_SIZE, 0);
    generate_input_data(randomized_data, data_size, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data, seq);

    // Updating header with correct sequence value
    for (std::size_t i = 0; i < sliced_words[0].size() / FRAME_WORD_SIZE; i++) {
        sliced_words[0][i * FRAME_WORD_SIZE] = seq_num;
        seq_num++;
        if ((seq_num % DUPLEX_FRAMES) == DUPLEX_ULFB_START) {
            seq_num += (DUPLEX_ULFB_END - DUPLEX_ULFB_START);
        } else if ((seq_num % DUPLEX_FRAMES) == DUPLEX_B1) {
            seq_num += (DUPLEX_FRAMES - DUPLEX_B1 + DUPLEX_START_FRAME_RX); 
        }
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
        if ((seq_num % DUPLEX_FRAMES) == DUPLEX_ULFB_START) {
            seq_num += (DUPLEX_ULFB_END - DUPLEX_ULFB_START);
        } else if ((seq_num % DUPLEX_FRAMES) == DUPLEX_B1) {
            seq_num += (DUPLEX_FRAMES - DUPLEX_B1 + DUPLEX_START_FRAME_RX); 
        }
        sliced_words[0].insert(sliced_words[0].end(),
                               header.begin(),
                               header.end());
        sliced_words[1].insert(sliced_words[1].end(),
                               zeros.begin(),
                               zeros.end());
    }

    return seq_num;
}

void demodulate_data(AirPacketInbound &air_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &demodulated_data) {
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
        std::cout << "Packet Number: " << found_packet << "\n"
                  << "Packet Frame: " << found_at_frame << "\n"
                  << "Packet Size: " << potential_words_again.size() << "\n";
    }
    if (do_erase || do_erase_sliced) {
        unsigned erase = do_erase < do_erase_sliced ? do_erase_sliced : do_erase;
        std::cout << "Erase Elements: " << erase << "\n";
        sliced_words[0].erase(sliced_words[0].begin(),
                              sliced_words[0].begin() + erase);
        sliced_words[1].erase(sliced_words[1].begin(),
                              sliced_words[1].begin() + erase);
    }
}

bool test_short_packet_demodulation() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t max_frame = 3;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Short Packet Demodulation", [max_frame,
                                                        &rx_packet,
                                                        &tx_packet,
                                                        &sliced_words]() {
        uint32_t seq_num;
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::size_t frame_size = *rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * FRAME_WORD_SIZE;

        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        seq_num = append_zeros(sliced_words, 25, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);

        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_medium_packet_demodulation() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t min_frame = 1;
    uint32_t max_frame = 25;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Medium Packet Demodulation", [&rx_packet,
                                                         &tx_packet,
                                                         &sliced_words,
                                                         min_frame,
                                                         max_frame]() {
        uint32_t seq_num;
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::size_t frame_size = *rc::gen::inRange(min_frame, max_frame);
        std::size_t data_size = frame_size * FRAME_WORD_SIZE;
        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        seq_num = append_zeros(sliced_words, 25, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);

        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_long_packet_demodulation() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t min_frame = 25;
    uint32_t max_frame = 200;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Long Packet Demodulation", [&rx_packet,
                                                       &tx_packet,
                                                       &sliced_words,
                                                       min_frame,
                                                       max_frame]() {
        uint32_t seq_num;
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::size_t frame_size = *rc::gen::inRange(min_frame, max_frame);
        std::size_t data_size = frame_size * FRAME_WORD_SIZE;
        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        seq_num = append_zeros(sliced_words, 25, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);

        RC_ASSERT(randomized_data == demodulated_data);
    });
}
