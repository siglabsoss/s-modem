#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <queue>
#include <rapidcheck.h>
#include "driver/AirPacket.hpp"

#define INPUT_DATA_SIZE      (10000)
#define DATA_FRAME_SIZE      (20)
#define PORT                 (40001)
#define MAX_MBPS             (15.625)
#define HEADER_SEQ_NUM_INDEX (5)
#define UDP_PACKET_SIZE      (368)
#define UDP_SEQ_NUM          (0xBB)
#define MAXLINE              (UDP_PACKET_SIZE * 4)

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
int create_tx_socket(sockaddr_in &serv_addr);

/**
 *
 */
void setup_air_packet(AirPacketOutbound &tx_packet);

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
int create_data_stream(double mbps,
                       std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_num,
                       uint32_t packet_iter);

/**
 *
 */
int append_zeros(std::queue<uint32_t> &data_stream,
                 std::vector<uint32_t> &header,
                 uint32_t packet_count,
                 int header_seq_num);

/**
 *
 */
int update_data_stream(std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_num);

/**
 *
 */
int _add_header(std::queue<uint32_t> &data_stream,
                std::vector<uint32_t> &header,
                int header_seq_num);

/**
 *
 */
void _add_data(std::queue<uint32_t> &data_stream,
               std::vector<uint32_t> &input_data,
               int section);

/**
 *
 */
int create_udp_packet(std::vector<uint32_t> &data_packet,
                      std::queue<uint32_t> &data_stream,
                      int udp_seq_num);


int main(int argc, char const *argv[]) {
    AirPacketOutbound tx_packet;
    int sock_fd;
    int udp_seq_num = UDP_SEQ_NUM;
    int header_seq_num = DUPLEX_START_FRAME_TX;
    double mbps = 2;
    double sleep_us = 417;
    struct sockaddr_in serv_addr;
    uint32_t counter_size = INPUT_DATA_SIZE;
    uint32_t packet_iter = 2;
    uint32_t packet_num = 0;
    char udp_payload[MAXLINE] = "";
    std::queue<uint32_t> data_stream;
    std::vector<std::vector<uint32_t>> sliced_data;
    std::vector<uint32_t> randomized_data;
    std::vector<uint32_t> udp_data_packet(UDP_PACKET_SIZE, 0);
    std::vector<uint32_t> header = {0x00000002,
                                    0x00000024,
                                    0x00000001,
                                    0x40000000,
                                    0x00000006,
                                    header_seq_num,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000,
                                    0x00000000};

    sock_fd = create_tx_socket(serv_addr);
    setup_air_packet(tx_packet);
    get_sliced_data(tx_packet, counter_size, sliced_data, randomized_data);
    header_seq_num = create_data_stream(mbps,
                                        data_stream,
                                        sliced_data[1],
                                        header,
                                        header_seq_num,
                                        packet_iter);
    // The number of UDP packets sent is the division of the data size by 360.
    // 360 is choosen because it is a multiple of our current frame size (20).
    packet_num = data_stream.size() / (UDP_PACKET_SIZE - 8);

    for (uint32_t i = 0; i < packet_num; i++) {
        udp_seq_num = create_udp_packet(udp_data_packet,
                                        data_stream,
                                        udp_seq_num);
        memcpy(udp_payload, &udp_data_packet[0], MAXLINE);
        usleep(sleep_us);
        sendto(sock_fd,
               (const char *) udp_payload,
               UDP_PACKET_SIZE * 4,
               MSG_CONFIRM,
               (const struct sockaddr *) &serv_addr,
               sizeof(serv_addr));
        std::cout << "Sent UDP packet: " << i << "\n"; 
    }
    return 0;
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

int create_tx_socket(sockaddr_in &serv_addr) {
    // Create socket file descriptor
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cout << "ERROR: Socket file descriptor was not allocated\n";
        return -1;
    } else {
        std::cout << "Socket construction complete\n";
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    // Filling server information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    return sock_fd;
}

void setup_air_packet(AirPacketOutbound &tx_packet) {
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
}

uint32_t get_sliced_data(AirPacketOutbound &tx_packet,
                         std::size_t data_size,
                         std::vector<std::vector<uint32_t>> &sliced_words,
                         std::vector<uint32_t> &randomized_data) {
    uint8_t seq = DUPLEX_START_FRAME_TX;
    uint32_t seq_num = DUPLEX_START_FRAME_TX;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> zeros;
    std::vector<uint32_t> header(DATA_FRAME_SIZE, 0);
    generate_input_data(randomized_data, data_size, false);
    modulated_data = tx_packet.transform(randomized_data, seq, 0);
    tx_packet.padData(modulated_data);
    sliced_words = tx_packet.emulateHiggsToHiggs(modulated_data, seq);

    // Updating header with correct sequence value
    for (std::size_t i = 0; i < sliced_words[0].size() / DATA_FRAME_SIZE; i++) {
        sliced_words[0][i * DATA_FRAME_SIZE] = seq_num;
        seq_num++;
        if (!(seq_num % DUPLEX_FRAMES)) seq_num += DUPLEX_START_FRAME_TX;
    }

    return seq_num;
}

int create_data_stream(double mbps,
                       std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_num,
                       uint32_t packet_iter) {
    double append_zero = (MAX_MBPS/mbps - 1) *
        input_data.size() / DATA_FRAME_SIZE;
    int zero_frames = ((int) append_zero) -
        ((int) append_zero % (DUPLEX_FRAMES - DUPLEX_START_FRAME_TX));
    std::size_t zero_size = DUPLEX_FRAMES - DUPLEX_START_FRAME_TX -
        (input_data.size() %
        ((DUPLEX_FRAMES - DUPLEX_START_FRAME_TX) * DATA_FRAME_SIZE)) /
        DATA_FRAME_SIZE;
    std::cout << "Append " << zero_frames << " packets of zero for a data "
              << "rate of " << mbps << " Mbps\n";
    for (std::size_t i = 0; i < packet_iter; i++) {
        header_seq_num = append_zeros(data_stream,
                                      header,
                                      zero_frames,
                                      header_seq_num);
        header_seq_num = update_data_stream(data_stream,
                                            input_data,
                                            header,
                                            header_seq_num);
        header_seq_num = append_zeros(data_stream,
                                      header,
                                      zero_size,
                                      header_seq_num);
    }
    return header_seq_num;
}

int append_zeros(std::queue<uint32_t> &data_stream,
                 std::vector<uint32_t> &header,
                 uint32_t packet_count,
                 int header_seq_num) {
    std::vector<uint32_t> zero(DATA_FRAME_SIZE, 0);

    for (uint32_t i = 0; i < packet_count; i++) {
        header_seq_num = update_data_stream(data_stream,
                                            zero,
                                            header,
                                            header_seq_num);
    }

    return header_seq_num;
}

int update_data_stream(std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_num) {
    int n = input_data.size() / DATA_FRAME_SIZE;
    if ((input_data.size() % DATA_FRAME_SIZE)) {
        std::cout << "Input data not a multiple of data size: "
                  << DATA_FRAME_SIZE << "\n";
    }
    for (std::size_t i = 0; i < n; i++) {
        header_seq_num = _add_header(data_stream, header, header_seq_num);
        _add_data(data_stream, input_data, i);
    }

    return header_seq_num;
}

int _add_header(std::queue<uint32_t> &data_stream,
                std::vector<uint32_t> &header,
                int header_seq_num) {
    header[HEADER_SEQ_NUM_INDEX] = header_seq_num;
    header_seq_num++;
    if (!(header_seq_num % DUPLEX_FRAMES)) {
        header_seq_num += DUPLEX_START_FRAME_TX;
    }

    for (std::size_t i = 0; i < header.size(); i++) {
        data_stream.push(header[i]);
    }

    return header_seq_num;
}

void _add_data(std::queue<uint32_t> &data_stream,
               std::vector<uint32_t> &input_data,
               int section) {
    for (std::size_t i = 0; i < DATA_FRAME_SIZE; i++) {
        data_stream.push(input_data[i + section*DATA_FRAME_SIZE]);
    }
}

int create_udp_packet(std::vector<uint32_t> &data_packet,
                      std::queue<uint32_t> &data_stream,
                      int udp_seq_num) {
    const std::size_t word_count = (UDP_PACKET_SIZE - 1) -
        ((UDP_PACKET_SIZE - 1) % DATA_FRAME_SIZE);
    data_packet[0] = udp_seq_num;
    udp_seq_num++;

    for (std::size_t i = 1; i < UDP_PACKET_SIZE; i++) {
        if (!data_stream.empty()) {
            data_packet[i] = data_stream.front();
            data_stream.pop();
        }
    }

    return udp_seq_num;
}
