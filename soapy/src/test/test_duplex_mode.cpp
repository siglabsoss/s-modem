#include <rapidcheck.h>
#include <queue>
#include "driver/AirPacket.hpp"

#define INPUT_DATA_SIZE        10000
#define MAX_MBPS               15.625

#define DUPLEX_START_FRAME (DUPLEX_START_FRAME_TX)

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
uint32_t append_zeros(AirPacketOutbound &tx_packet,
                      std::vector<std::vector<uint32_t>> &sliced_words,
                      std::size_t frame_count,
                      uint32_t seq_num);

/**
 *
 */
uint32_t append_data(AirPacketOutbound &tx_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &modulated_data,
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
uint32_t update_data_queue(AirPacketOutbound &tx_packet,
                           std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           uint32_t seq_num);

/**
 *
 */
uint32_t create_data_stream(AirPacketOutbound &tx_packet,
                            std::queue<uint32_t> &header_queue,
                            std::queue<uint32_t> &data_queue,
                            std::vector<uint32_t> &data,
                            std::size_t packet_count,
                            uint32_t seq_num);

/**
 *
 */
void update_sliced_words(AirPacketOutbound &tx_packet,
                         std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count);

/**
 *
 */
bool test_short_packet_demodulation();

/**
 *
 */
bool test_long_packet_demodulation();

/**
 *
 */
bool test_short_packet_with_gap();

/**
 *
 */
bool test_long_packet_with_gap();

/**
 *
 */
bool test_demodulation_with_partial_data();

/**
 *
 */
bool test_zero_streaming();

int main(int argc, char const *argv[]) {
    bool pass = true;
    pass &= test_short_packet_demodulation();
    pass &= test_long_packet_demodulation();
    pass &= test_short_packet_with_gap();
    pass &= test_long_packet_with_gap();
    pass &= test_demodulation_with_partial_data();
    pass &= test_zero_streaming();

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
    rx_packet.print_settings_did_change = true;
}

uint32_t get_sliced_data(AirPacketOutbound &tx_packet,
                         std::size_t data_size,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::vector<uint32_t> &randomized_data) {
    uint8_t seq = DUPLEX_START_FRAME;
    uint32_t seq_num = DUPLEX_START_FRAME;
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> zeros;
    std::vector<uint32_t> header(frame_word_size, 0);
    generate_input_data(randomized_data, data_size, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data, seq);

    // Updating header with correct sequence value
    for (std::size_t i = 0; i < sliced_words[0].size() / frame_word_size; i++) {
        sliced_words[0][i * frame_word_size] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
    }

    return seq_num;
}

uint32_t append_zeros(AirPacketOutbound &tx_packet,
                      std::vector<std::vector<uint32_t>> &sliced_words,
                      std::size_t frame_count,
                      uint32_t seq_num) {
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    std::vector<uint32_t> header(frame_word_size, 0);
    std::vector<uint32_t> zeros(frame_word_size, 0);

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

uint32_t append_data(AirPacketOutbound &tx_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words,
                     std::vector<uint32_t> &modulated_data,
                     uint32_t seq_num) {
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    std::vector<uint32_t> header(frame_word_size, 0);
    std::size_t frame_count = modulated_data.size() / frame_word_size;

    for (std::size_t i = 0; i < frame_count; i++) {
        header[0] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME;
        sliced_words[0].insert(sliced_words[0].end(),
                               header.begin(),
                               header.end());
        sliced_words[1].insert(
                        sliced_words[1].end(),
                        modulated_data.begin() + (i * frame_word_size),
                        modulated_data.begin() + ((i + 1) * frame_word_size));
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

uint32_t update_data_queue(AirPacketOutbound &tx_packet,
                           std::queue<uint32_t> &header_queue,
                           std::queue<uint32_t> &data_queue,
                           std::vector<uint32_t> &data,
                           uint32_t seq_num) {
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    uint32_t header;
    for (std::size_t i = 0; i < data.size(); i++) {
        if (!(i % frame_word_size)) {
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

uint32_t create_data_stream(AirPacketOutbound &tx_packet,
                            std::queue<uint32_t> &header_queue,
                            std::queue<uint32_t> &data_queue,
                            std::vector<uint32_t> &data,
                            std::size_t packet_count,
                            uint32_t seq_num) {
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    std::size_t zero_size = DUPLEX_FRAMES - DUPLEX_START_FRAME -
    (data.size() % ((DUPLEX_FRAMES - DUPLEX_START_FRAME) * frame_word_size)) /
    frame_word_size;
    std::vector<uint32_t> zeros(zero_size * frame_word_size, 0);

    for (std::size_t i = 0; i < packet_count; i++) {
        seq_num = update_data_queue(tx_packet,
                                    header_queue,
                                    data_queue,
                                    data,
                                    seq_num);
        seq_num = update_data_queue(tx_packet,
                                    header_queue,
                                    data_queue,
                                    zeros,
                                    seq_num);
    }
    zeros.resize((DUPLEX_FRAMES - DUPLEX_START_FRAME) * frame_word_size, 0);
    seq_num = update_data_queue(tx_packet,
                                header_queue,
                                data_queue,
                                zeros,
                                seq_num);
    return seq_num;
}

void update_sliced_words(AirPacketOutbound &tx_packet,
                         std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count) {
    uint32_t frame_word_size = tx_packet.sliced_word_count;
    for (std::size_t j = 0; j < append_count * frame_word_size; j++) {
        sliced_words[0].push_back(header_queue.front());
        sliced_words[1].push_back(data_queue.front());
        header_queue.pop();
        data_queue.pop();
    }
}

bool test_short_packet_demodulation() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t max_frame = 66;

    setup_air_packet(rx_packet, tx_packet);
    std::cout << tx_packet.sliced_word_count << "\n";
    return rc::check("Test Short Packet Demodulation", [max_frame,
                                                        &rx_packet,
                                                        &tx_packet,
                                                        &sliced_words]() {
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        uint32_t seq_num;
        uint32_t frame_word_size = tx_packet.sliced_word_count;
        std::size_t frame_size = *rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * frame_word_size;

        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        seq_num = append_zeros(tx_packet, sliced_words, 25, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);

        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_long_packet_demodulation() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t min_frame = 66;
    uint32_t max_frame = 500;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Long Packet Demodulation", [&rx_packet,
                                                       &tx_packet,
                                                       &sliced_words,
                                                       min_frame,
                                                       max_frame]() {
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        uint32_t seq_num;
        uint32_t frame_word_size = tx_packet.sliced_word_count;
        std::size_t frame_size = *rc::gen::inRange(min_frame, max_frame);
        std::size_t data_size = frame_size * frame_word_size;
        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        seq_num = append_zeros(tx_packet, sliced_words, 25, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);

        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_short_packet_with_gap() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t max_frame = 66;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Short Packet Demodulation w/ Gap", [max_frame,
                                                               &rx_packet,
                                                               &tx_packet,
                                                               &sliced_words]() {
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::vector<uint32_t> modulated_data;
        uint32_t seq_num;
        uint32_t frame_word_size = tx_packet.sliced_word_count;
        std::size_t zero_size;
        std::size_t frame_size = *rc::gen::inRange(1u, max_frame);
        std::size_t data_size = frame_size * frame_word_size;
        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        modulated_data = sliced_words[1];
        zero_size = DUPLEX_FRAMES - DUPLEX_START_FRAME -
                    (sliced_words[0].size() / frame_word_size);
        seq_num = append_zeros(tx_packet, sliced_words, zero_size, seq_num);
        seq_num = append_data(tx_packet, sliced_words, modulated_data, seq_num);
        seq_num = append_zeros(tx_packet, sliced_words, zero_size, seq_num);
        seq_num = append_zeros(tx_packet, sliced_words,
                               DUPLEX_FRAMES - DUPLEX_START_FRAME,
                               seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);
        RC_ASSERT(randomized_data == demodulated_data);
        demodulated_data.resize(0);
        demodulate_data(rx_packet, sliced_words, demodulated_data);
        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_long_packet_with_gap() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t min_frame = 66;
    uint32_t max_frame = 500;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Long Packet Demodulation w/ Gap", [&rx_packet,
                                                              &tx_packet,
                                                              &sliced_words,
                                                              min_frame,
                                                              max_frame]() {
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::vector<uint32_t> modulated_data;
        uint32_t seq_num;
        uint32_t frame_word_size = tx_packet.sliced_word_count;
        std::size_t zero_size;
        std::size_t frame_size = *rc::gen::inRange(min_frame, max_frame);
        std::size_t data_size = frame_size * frame_word_size;
        seq_num = get_sliced_data(tx_packet,
                                  data_size,
                                  sliced_words,
                                  randomized_data);
        modulated_data = sliced_words[1];
        zero_size = DUPLEX_FRAMES - DUPLEX_START_FRAME -
        (sliced_words[0].size() %
        ((DUPLEX_FRAMES - DUPLEX_START_FRAME) * frame_word_size)) /
        frame_word_size;
        seq_num = append_zeros(tx_packet, sliced_words, zero_size, seq_num);
        seq_num = append_data(tx_packet, sliced_words, modulated_data, seq_num);
        seq_num = append_zeros(tx_packet, sliced_words, zero_size, seq_num);
        seq_num = append_zeros(tx_packet, sliced_words,
                               DUPLEX_FRAMES - DUPLEX_START_FRAME,
                               seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);
        RC_ASSERT(randomized_data == demodulated_data);
        demodulated_data.resize(0);
        demodulate_data(rx_packet, sliced_words, demodulated_data);
        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_demodulation_with_partial_data() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words;
    uint32_t min_frame = 66;
    uint32_t max_frame = 500;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Demodulation w/ Partial Data", [&rx_packet,
                                                           &tx_packet,
                                                           &sliced_words,
                                                           min_frame,
                                                           max_frame]() {
        uint32_t seq_num = DUPLEX_START_FRAME;
        uint32_t frame_word_size = tx_packet.sliced_word_count;
        std::queue<uint32_t> header_queue;
        std::queue<uint32_t> data_queue;
        std::vector<uint32_t> randomized_data;
        std::vector<uint32_t> demodulated_data;
        std::vector<uint32_t> modulated_data;
        std::size_t frame_size = *rc::gen::inRange(min_frame, max_frame);
        std::size_t packet_count = *rc::gen::inRange(1u, 10u);
        std::size_t data_size = frame_size * frame_word_size;
        std::size_t append_count;
        int max_append = 5;
        get_sliced_data(tx_packet,
                        data_size,
                        sliced_words,
                        randomized_data);
        modulated_data = sliced_words[1];
        sliced_words[0].resize(0);
        sliced_words[1].resize(0);
        seq_num = create_data_stream(tx_packet,
                                     header_queue,
                                     data_queue,
                                     modulated_data,
                                     packet_count,
                                     seq_num);
        while (max_append) {
            if (data_queue.size() / frame_word_size < 5) {
                append_count = 1;
            } else {
                append_count = *rc::gen::element(0, 1, 2, 3, 4, 5);
            }
            update_sliced_words(tx_packet,
                                data_queue,
                                header_queue,
                                sliced_words,
                                append_count);
            demodulate_data(rx_packet, sliced_words, demodulated_data);
            max_append = data_queue.size() / frame_word_size;
        }
        RC_ASSERT(randomized_data == demodulated_data);
    });
}

bool test_zero_streaming() {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;
    std::vector<std::vector<uint32_t>> sliced_words(2);

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Zero Streaming", [&rx_packet,
                                             &tx_packet,
                                             &sliced_words]() {
        uint32_t seq_num = DUPLEX_START_FRAME;
        std::vector<uint32_t> demodulated_data;
        std::size_t zero_size = *rc::gen::inRange(0u, 1000u);
        seq_num = append_zeros(tx_packet, sliced_words, zero_size, seq_num);
        demodulate_data(rx_packet, sliced_words, demodulated_data);
        RC_ASSERT(!sliced_words[0].size());
        RC_ASSERT(!sliced_words[1].size());
    });
}
