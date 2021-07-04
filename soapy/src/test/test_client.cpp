#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <queue>
#include "cpp_utils.hpp"
#include "driver/AirPacket.hpp"
#define BITS_PER_MEGABIT     1048576.0
#define PORT                 40001
#define DATA_SIZE            20
#define HEADER_SIZE          16
#define PACKET_SIZE          368
#define HEADER_SEQ_NUM_INDEX 5
#define MAX_MBPS             15.625
#define MAXLINE              (PACKET_SIZE * 4)

/**
 *
 */
int update_data_stream(std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_number);

/**
 *
 */
int _add_header(std::queue<uint32_t> &data_stream,
                std::vector<uint32_t> &header,
                int header_seq_number);

/**
 *
 */
void _add_data(std::queue<uint32_t> &data_stream,
               std::vector<uint32_t> &input_data,
               int section);

/**
 *
 */
int create_data_stream(double mbps,
                       std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_number,
                       uint32_t iteration);

/**
 *
 */
void create_data_packet(std::vector<uint32_t> &data_packet,
                        std::queue<uint32_t> &data_stream,
                        int section);

/**
 *
 */
int append_zeros(std::queue<uint32_t> &data_stream,
                 std::vector<uint32_t> &header,
                 uint32_t packet_count,
                 int header_seq_number);

/**
 *
 */
void get_file_data(std::string filepath, std::vector<uint32_t> file_data);

/**
 *
 */
template<class T>
void setup_air_packet(T *air_packet);

/**
 *
 */
void create_counter(std::vector<uint32_t> &counter_data, uint32_t counter_size);

/**
 *
 */
void get_sliced_data(AirPacketOutbound &air_packet,
                     std::vector<uint32_t> &sliced_data,
                     uint32_t counter_size);

/**
 *
 */
int create_tx_socket(sockaddr_in &serv_addr);

int main(int argc, char const *argv[]) {
    AirPacketOutbound tx_packet;
    int seq_number = 0xBB;
    int header_seq_number = 0xA;
    int sock_fd;
    uint32_t packet_num = 0;
    uint32_t start_data = 0;
    uint32_t iteration = 40;
    double mbps = 15.625;
    double sleep_us = 417;
    double data_rate;
    uint32_t counter_size = 10000;
    char hello[MAXLINE] = "";
    struct sockaddr_in serv_addr;
    std::queue<uint32_t> data_stream;
    std::vector<uint32_t> file_data;
    std::vector<uint32_t> sliced_data;
    std::vector<uint32_t> data_packet(PACKET_SIZE, 0);
    std::string filepath = "../sim/data/mapmov_320_qpsk_1_sliced.hex";
    std::vector<uint32_t> header = {0x00000002,
                                    0x00000024,
                                    0x00000001,
                                    0x40000000,
                                    0x00000006,
                                    header_seq_number,
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
    data_packet[0] = seq_number;
    setup_air_packet(&tx_packet);
    sock_fd = create_tx_socket(serv_addr);
    get_sliced_data(tx_packet, sliced_data, counter_size);
    get_file_data(filepath, file_data);
    header_seq_number = create_data_stream(mbps,
                                           data_stream,
                                           sliced_data,
                                           header,
                                           header_seq_number,
                                           iteration);
    packet_num = data_stream.size() / (PACKET_SIZE - 1);
    // Calculates index when a UDP data packet is sent
    start_data = (MAX_MBPS/mbps - 1) * sliced_data.size() / DATA_SIZE *
                 (DATA_SIZE + HEADER_SIZE) / (PACKET_SIZE - 1);
    std::cout << "Data stream size: " << data_stream.size() << "\n";
    std::cout << "Packet count: " << packet_num << "\n";
    auto begin = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < packet_num; i++) {
        auto start = std::chrono::steady_clock::now();
        create_data_packet(data_packet, data_stream, i);
        memcpy(hello, &data_packet[0], MAXLINE);
        seq_number++;
        data_packet[0] = seq_number;
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::micro> diff = end - start;
        usleep(sleep_us - diff.count());
        sendto(sock_fd,
               (const char *) hello,
               PACKET_SIZE * 4,
               MSG_CONFIRM,
               (const struct sockaddr *) &serv_addr,
               sizeof(serv_addr));
        if (i == start_data) {
            std::chrono::milliseconds ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());
            std::cout << "Sent data packet at " << ms.count() << "\n";
            // Calculates index when next UDP data packet is sent
            start_data += sliced_data.size() / DATA_SIZE *
                          (DATA_SIZE + HEADER_SIZE) / (PACKET_SIZE - 1);
            start_data += (MAX_MBPS/mbps - 1) * sliced_data.size() / DATA_SIZE *
                          (DATA_SIZE + HEADER_SIZE) / (PACKET_SIZE - 1);
        }
    }
    auto complete = std::chrono::steady_clock::now();
    std::chrono::duration<double> t = complete - begin;
    double total_bits = sliced_data.size() * 32 * iteration;
    data_rate = total_bits / t.count() / BITS_PER_MEGABIT;
    std::cout << "\n";
    std::cout << "Total Time: " << t.count() << " s\n"
              << "Packet Size: " << sliced_data.size() << " words\n"
              << "Total Words: " << sliced_data.size() * iteration << " words\n"
              << "Total Bits: " << total_bits << " bits\n"
              << "Data Rate: " << data_rate << " Mbps\n";
    close(sock_fd);

    return 0;
}

int update_data_stream(std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_number) {
    int n = input_data.size() / DATA_SIZE;
    if ((input_data.size() % DATA_SIZE)) {
        std::cout << "Input data not a multiple of data size: "
                  << DATA_SIZE << "\n";
    }
    for (std::size_t i = 0; i < n; i++) {
        header_seq_number = _add_header(data_stream, header, header_seq_number);
        _add_data(data_stream, input_data, i);
    }

    return header_seq_number;
}

int _add_header(std::queue<uint32_t> &data_stream,
                std::vector<uint32_t> &header,
                int header_seq_number) {
    header[HEADER_SEQ_NUM_INDEX] = header_seq_number;
    header_seq_number++;

    for (std::size_t i = 0; i < header.size(); i++) {
        data_stream.push(header[i]);
    }

    return header_seq_number;
}

void _add_data(std::queue<uint32_t> &data_stream,
               std::vector<uint32_t> &input_data,
               int section) {
    for (std::size_t i = 0; i < DATA_SIZE; i++) {
        data_stream.push(input_data[i + section*DATA_SIZE]);
    }
}

int create_data_stream(double mbps,
                       std::queue<uint32_t> &data_stream,
                       std::vector<uint32_t> &input_data,
                       std::vector<uint32_t> &header,
                       int header_seq_number,
                       uint32_t iteration) {
    double append_zero = (MAX_MBPS/mbps - 1) * input_data.size() / DATA_SIZE;

    std::cout << "Append " << append_zero << " packets of zero for a data "
              << "rate of " << mbps << " Mbps\n";
    for (std::size_t i = 0; i < iteration; i++) {
        header_seq_number = append_zeros(data_stream,
                                         header,
                                         (int) append_zero,
                                         header_seq_number);
        header_seq_number = update_data_stream(data_stream,
                                               input_data,
                                               header,
                                               header_seq_number);
    }
    return header_seq_number;
}

void create_data_packet(std::vector<uint32_t> &data_packet,
                        std::queue<uint32_t> &data_stream,
                        int section) {
    bool debug = false;
    for (std::size_t i = 1; i < PACKET_SIZE; i++) {
        if (!data_stream.empty()) {
            data_packet[i] = data_stream.front();
            data_stream.pop();
        }
    }
    if (debug) {
        std::cout << "Data Packet " << section << ": \n";
        for (uint32_t x : data_packet) {
            std::cout << std::hex << x << std::dec <<"\n";
        }
    }
}

int append_zeros(std::queue<uint32_t> &data_stream,
                 std::vector<uint32_t> &header,
                 uint32_t packet_count,
                 int header_seq_number) {
    std::vector<uint32_t> zero(DATA_SIZE, 0);

    for (uint32_t i = 0; i < packet_count; i++) {
        header_seq_number = update_data_stream(data_stream,
                                               zero,
                                               header,
                                               header_seq_number);
    }

    return header_seq_number;
}

void get_file_data(std::string filepath, std::vector<uint32_t> file_data) {
    std::string hex_value;
    std::ifstream input_file(filepath);
    while (std::getline(input_file, hex_value)) {
        file_data.push_back((uint32_t) std::stoul(hex_value, nullptr, 16));
    }
}

template<class T>
void setup_air_packet(T *air_packet) {
    uint32_t code_length = 255;
    uint32_t fec_length = 48;
    uint32_t interleave_length = 64;
    int code_bad;

    air_packet->set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    air_packet->set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    air_packet->set_interleave(interleave_length);
    code_bad = air_packet->set_code_type(AIRPACKET_CODE_REED_SOLOMON,
                                         code_length,
                                         fec_length);
}

void create_counter(std::vector<uint32_t> &counter_data,
                    uint32_t counter_size) {
    for (std::size_t i = 0; i < counter_size; i++) {
        counter_data.push_back(i);
    }
}

void get_sliced_data(AirPacketOutbound &air_packet,
                     std::vector<uint32_t> &sliced_data,
                     uint32_t counter_size) {
    uint8_t seq;
    std::vector<uint32_t> modulated_data;
    std::vector<uint32_t> counter_data;
    std::vector<std::vector<uint32_t>> sliced_words;

    create_counter(counter_data, counter_size);
    modulated_data = air_packet.transform(counter_data, seq, 0);
    air_packet.padData(modulated_data);
    sliced_words = air_packet.emulateHiggsToHiggs(modulated_data);
    sliced_data = sliced_words[1];
}

int create_tx_socket(sockaddr_in &serv_addr) {
    int sock_fd;
    // Create socket file descriptor
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
