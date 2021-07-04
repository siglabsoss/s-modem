#include <vector>
#include <queue>
#include <rapidcheck.h>
#include "cpp_utils.hpp"
#include "FileUtils.hpp"
#include "driver/AirPacket.hpp"


#define INPUT_DATA_SIZE      10000
#define INITIAL_HEADER_NUM   0xA
#define INITIAL_MBPS         2
#define DATA_SIZE            20
#define MAX_MBPS             15.625
using namespace siglabs::file;
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
bool test_demod_thread(std::size_t data_size);

/**
 *
 */
int get_sliced_data(std::queue<uint32_t> &data_queue,
                    std::queue<uint32_t> &header_queue,
                    AirPacketOutbound &tx_packet,
                    std::size_t data_size,
                    uint32_t iteration,
                    int header_num,
                    double mbps);

/**
 *
 */
int update_header_queue(std::queue<uint32_t> &header_queue,
                        std::size_t update_size,
                        int header_num);

/**
 *
 */
void update_data_queue(std::queue<uint32_t> &data_queue,
                       AirPacketOutbound &tx_packet,
                       std::size_t update_size,
                       double mbps,
                       uint32_t iteration);

/**
 *
 */
void update_sliced_words(std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count);

/**
 *
 */
void setup_air_packet(AirPacketInbound &rx_packet,
                      AirPacketOutbound &tx_packet);

/**
 *
 */
void demodulate_data(AirPacketInbound &air_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words);


int main() {
    std::size_t data_size = INPUT_DATA_SIZE;
    bool pass = true;
    pass &= test_demod_thread(data_size);

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

bool test_demod_thread(std::size_t data_size) {
    AirPacketInbound rx_packet;
    AirPacketOutbound tx_packet;

    setup_air_packet(rx_packet, tx_packet);
    return rc::check("Test Demodulation Thread for Dropped Packets",
                     [data_size, &rx_packet, &tx_packet]() {
        std::vector<std::vector<uint32_t>> sliced_words(2);
        std::vector<uint32_t> header_vector;
        std::vector<uint32_t> data_vector;
        std::queue<uint32_t> data_queue;
        std::queue<uint32_t> header_queue;
        std::size_t append_count;
        uint32_t iteration = 1;
        int max_append = 5;
        int header_num = INITIAL_HEADER_NUM;
        double mbps = INITIAL_MBPS;
        sliced_words[0] =  header_vector;
        sliced_words[1] = data_vector;
        header_num = get_sliced_data(data_queue,
                                     header_queue,
                                     tx_packet,
                                     data_size,
                                     iteration,
                                     header_num,
                                     mbps);
        while (max_append) {
            if (data_queue.size() / DATA_SIZE < 5) append_count = 1;
            else append_count = *rc::gen::element(0, 1, 2, 3, 4, 5);
            update_sliced_words(data_queue,
                                header_queue,
                                sliced_words,
                                append_count);
            demodulate_data(rx_packet, sliced_words);
            max_append = data_queue.size() / DATA_SIZE;
        }

    });
}

int get_sliced_data(std::queue<uint32_t> &data_queue,
                    std::queue<uint32_t> &header_queue,
                    AirPacketOutbound &tx_packet,
                    std::size_t data_size,
                    uint32_t iteration,
                    int header_num,
                    double mbps) {
    update_data_queue(data_queue, tx_packet, data_size, mbps, iteration);
    header_num = update_header_queue(header_queue,
                                     data_queue.size(),
                                     header_num);
    return header_num;
}

int update_header_queue(std::queue<uint32_t> &header_queue,
                        std::size_t update_size,
                        int header_num) {
    RC_ASSERT(!(update_size % DATA_SIZE));

    for (std::size_t i = 0; i < update_size / DATA_SIZE; i++) {
        header_queue.push(header_num);
        header_num++;
        // Appending 19 zeros because of how data is received in s-modem
        for (std::size_t j = 0; j < DATA_SIZE - 1; j++) {
            header_queue.push(0);
        }
    }

    RC_ASSERT(!(header_queue.size() % DATA_SIZE));

    return header_num;
}

void update_data_queue(std::queue<uint32_t> &data_queue,
                       AirPacketOutbound &tx_packet,
                       std::size_t update_size,
                       double mbps,
                       uint32_t iteration) {
    uint8_t seq;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> randomized_data;
    std::vector<std::vector<uint32_t>> sliced_words;

    generate_input_data(randomized_data, update_size, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data);

    int append_zero = (MAX_MBPS/mbps - 1) * sliced_words[1].size();
    // Making `append_zero` a multiple of 20
    append_zero += (DATA_SIZE - (append_zero % DATA_SIZE));
    for (std::size_t j = 0; j < DATA_SIZE * iteration; j++) {
        for (int i = 0; i < append_zero; i++) {
            data_queue.push(0);
        }
        for (std::size_t k = 0; k < sliced_words[1].size(); k++) {
            data_queue.push(sliced_words[1][k]);
        }
    }
    // Append more zeros for s-modem compatibility
    for (int i = 0; i < append_zero; i++) {
            data_queue.push(0);
    }
    std::cout << "Data Queue Size: " << data_queue.size() << "\n"
              << "Append Zeros: " << append_zero << "\n"
              << "Sliced Words: " << sliced_words[1].size() << "\n";
}

void update_sliced_words(std::queue<uint32_t> &data_queue,
                         std::queue<uint32_t> &header_queue,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::size_t append_count) {
    for (std::size_t j = 0; j < append_count * DATA_SIZE; j++) {
        sliced_words[0].push_back(header_queue.front());
        sliced_words[1].push_back(data_queue.front());
        header_queue.pop();
        data_queue.pop();
    }
}

void setup_air_packet(AirPacketInbound &rx_packet,
                      AirPacketOutbound &tx_packet) {
    // TX Air Packet setup
    uint32_t code_length = 255;
    uint32_t fec_length = 48;
    int code_bad;

    tx_packet.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    tx_packet.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    code_bad = tx_packet.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
                                       code_length,
                                       fec_length);
    tx_packet.set_interleave(1024);
    tx_packet.print_settings_did_change = false;
    // RX Air Packet setup
    rx_packet.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    rx_packet.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    code_bad = rx_packet.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
                                       code_length,
                                       fec_length);
    rx_packet.print_settings_did_change = false;
    rx_packet.set_interleave(1024);
}

void demodulate_data(AirPacketInbound &air_packet,
                     std::vector<std::vector<uint32_t>> &sliced_words) {
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
        uint32_t force_detect_index = air_packet.getForceDetectIndex(found_at_index);
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
        if(!found_something_again) {
            return;
        }
        found_packet++;
        std::cout << "Packet Number: " << found_packet << "\n"
                  << "Packet Frame: " << found_at_frame << "\n"
                  << "Packet Size: " << potential_words_again.size() << "\n";
    }
    if (do_erase) {
        // std::cout << "Erase Elements: " << do_erase << "\n";
        sliced_words[0].erase(sliced_words[0].begin(),
                              sliced_words[0].begin() + do_erase);
        sliced_words[1].erase(sliced_words[1].begin(),
                              sliced_words[1].begin() + do_erase);
    }
}
