#include <rapidcheck.h>
#include "driver/StatusCode.hpp"
#include "DemodThread.hpp"
#include "HiggsSettings.hpp"
#include "constants.hpp"
#include "FileUtils.hpp"
#include "random.h"
#include "vector_helpers.hpp"
#include "VerifyHash.hpp"
#include "AirPacket.hpp"



#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep

using namespace std;
using namespace siglabs::file;

// #define g_timeout ((double)0.001)


static std::vector<uint32_t> ideal_seq_one() {
    size_t total = 0;
    constexpr size_t pack = 350;


    std::vector<uint32_t> packet;
    unsigned int state = 0xdead0000;
    for(size_t i = 0; i < pack; i++ ) {
        state = xorshift32(state, &state, 1);
        packet.push_back(state);
    }

    return packet;
}

static std::vector<uint32_t> ideal_seq(const unsigned len) {
    size_t total = 0;
    constexpr size_t pack = 350;


    std::vector<uint32_t> packet;
    unsigned int state = 0xdead0000;
    for(size_t i = 0; i < pack; i++ ) {
        state = xorshift32(state, &state, 1);
        packet.push_back(state);
    }

    packet.push_back(0xfffff000);

    std::vector<uint32_t> ret;

    while(ret.size() < len ) {
        VEC_APPEND(ret, packet);
    }

    return ret;
}

static std::vector<uint32_t> ideal_seq_counter(const unsigned len) {
    size_t total = 0;
    constexpr size_t pack = 350;


    std::vector<uint32_t> packet;
    unsigned int state = 0xdead0000;
    for(size_t i = 0; i < pack; i++ ) {
        state = xorshift32(state, &state, 1);
        packet.push_back(state);
    }

    packet.push_back(0xfffff000);

    std::vector<uint32_t> ret;

    uint32_t j = 0;
    while(ret.size() < len ) {
        packet[0] = j;
        packet[1] = j;
        packet[2] = j;
        packet[3] = j;

        VEC_APPEND(ret, packet);
        j++;
    }

    return ret;
}

int main2() {

    auto seq = ideal_seq_one();

    seq.push_back(4);

    // for(auto w : seq) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    VerifyHash* verifyHashTx = 0;
    verifyHashTx = new VerifyHash();

    auto res = verifyHashTx->getHashForWords(seq);
    cout << HEX32_STRING(res) << "\n";




    return 0;
}


int main() {

    std::string path = "../init.json";

    cout << "Demod Thread\n";

    HiggsSettings _settings;
    HiggsSettings* settings = &_settings;

    std::vector<std::string> settings_path = PATH_FOR(GET_RUNTIME_DATA_TO_JS());
    settings->vectorSet(true, settings_path);

    settings_path = PATH_FOR(GET_DATA_HASH_VALIDATE_ALL());
    settings->vectorSet(false, settings_path);

    const int sliced_word_sice = (40); // see SLICED_DATA_WORD_SIZE
    // std::string data_path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dump_demod_missing_63.hex";
    std::string data_path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dump_demod_counter.hex";

    std::vector<uint32_t> data = file_read_hex(data_path);

    cout << "data is " << data.size() << "\n";

    // for(auto w : data) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    // {

        // auto demodThread = std::make_unique<DemodThread*>(new DemodThread(settings));
    auto demodThread = new DemodThread(settings);
    demodThread->dropSlicedData = false;

    // seems like this test doesn't check airpacket at all?
    demodThread->airRx->disablePrint("all");

    uint32_t counter = 0;

    const unsigned byte_size = sliced_word_sice * sizeof(uint32_t);

    int max_read = data.size() - sliced_word_sice;
    max_read = 6E6;

    for(int i = 0; i < max_read; i += sliced_word_sice ) {
        // cout << "i " << i << "\n";
        bufferevent_write(
            demodThread->sliced_data_bev->in,
            reinterpret_cast<const char *>(&counter),
            sizeof(counter) );

        bufferevent_write(
            demodThread->sliced_data_bev->in,
            reinterpret_cast<const char *>(data.data() + i),
            byte_size );

        counter++;
        // usleep(10);
    }

        // usleep(5000);

    usleep(1000000*1);
    // usleep(1000000*1);

    bool print_all = false;

    bool valid;
    got_data_t packet;

    std::tie(valid,packet) = demodThread->getDataToJs();

    cout << "valid: " << valid << "\n";

    if( valid ) {
        std::vector<uint32_t> data;
        uint32_t x;
        air_header_t header;


        std::tie(data, x, header) = packet;

        cout << "data len: " << data.size() << "\n";

        std::vector<uint32_t> ideal = ideal_seq_counter(data.size());

        int check_len = min(ideal.size(), data.size());

        int same = 0;
        for(int i = 0; i < check_len; i++ ) {

            if( print_all ) {
                cout << HEX32_STRING(ideal[i]) << " " << HEX32_STRING(data[i]);
            }

            if( ideal[i] == data[i]) {
                same++;
            } else {
                if( print_all ) {
                    cout << "          XXXXXXXX";
                }
            }

            if( print_all ) {
                cout  << "\n";
            }
        }

        cout << "same: " << same << " max " << check_len << "\n";


        // for( auto w : data ) {
        //     cout << HEX32_STRING(w) << "\n";
        // }
    }

    // std::tie<bool,got_data_t>(valid,pa) = x;




    delete demodThread;
    // }

    // demodThread.reset();
    // cout << "Will destroy\n";
    // usleep(1000000*1);

    // // for(int i = 0; i < 10; i++) {
    // //     usleep(0.5E6);
    // // }

    // cout << "Will destroy\n";

    // demodThread




    // rc::check("check can add and will go stale using 1ms timeout", [](std::string message) {
    //     auto dut = StatusCode(false, false, (HiggsDriver*) NULL);

    //     char a = (char)*rc::gen::inRange('A', 'Z');

    //     std::string path = "foo.";
    //     path += a;

    //     // cout << "Path: " << path << endl;

    //     char b = (char)*rc::gen::inRange('a', 'z');

    //     std::string path2 = "bar_none.";
    //     path2 += b;


    //     dut.create(path, g_timeout);

    //     dut.set(path, STATUS_ERROR);

    //     int error;
    //     status_code_t found = dut.get(path, &error);
    //     RC_ASSERT(error == 0);
    //     auto code0 = std::get<0>(found);

    //     // cout << code0 << endl;
    //     RC_ASSERT( code0 == STATUS_ERROR);
        
    //     unsigned got0 = dut.getCode(path, &error);
    //     RC_ASSERT(error == 0);
    //     RC_ASSERT( got0 == STATUS_ERROR);

    // //     // sleep for longer than the timeout
    //     usleep(g_timeout*1E6*3);

    //     unsigned got1 = dut.getCode(path, &error);

    //     RC_ASSERT(got1 == STATUS_STALE);

    //     dut.set(path, STATUS_WARN, message);

    //     unsigned got2 = dut.getCode(path, &error);
    //     RC_ASSERT(error == 0);

    //     unsigned got3 = dut.getCode(path2, &error);
    //     RC_ASSERT(error != 0);

    //     status_code_t obj0 = dut.get(path, &error);
    //     RC_ASSERT(error == 0);

    //     RC_ASSERT(std::get<1>(obj0) == message);

    //     usleep(g_timeout*1E6*3);

    //     status_code_t obj1 = dut.get(path, &error);
    //     RC_ASSERT(error == 0);

    //     RC_ASSERT(std::get<0>(obj1) == STATUS_STALE);
    // });

    // rc::check("0 timeout is ok", [](std::string message) {
    //     auto dut = StatusCode(false, false, (HiggsDriver*) NULL);

    //     char a = (char)*rc::gen::inRange('A', 'Z');

    //     std::string path = "foo.";
    //     path += a;

    //     dut.create(path, 0, true);

    //     dut.set(path, STATUS_OK, "all ok boss");

    //     usleep(1000);

    //     int error;

    //     unsigned got2 = dut.getCode(path, &error);

    //     RC_ASSERT(got2 == STATUS_OK);

    //     // cout << dut.getPeriodicPrint();

    // });



    return 0;
}

