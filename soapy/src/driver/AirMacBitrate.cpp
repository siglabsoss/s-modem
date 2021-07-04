#include "AirMacBitrate.hpp"
#include "cpp_utils.hpp"

#include <ctime>


#include <iostream>
using namespace std;


// was
// generate_input_data_base(_randomized_data, data_size, 0x77000000, false);
static void genhack(std::vector<uint32_t> &input_data,
                         const uint32_t data_length,
                         const uint32_t base) {
    uint32_t data;
    input_data.reserve(data_length);
    for(uint32_t i = 0; i < data_length; i++) {
        uint32_t j = i;

        j = (i) % 80;
        // if( i > 20 ) {
        //     j = (i - 20) % 80 + 20;
        // }
        data = (j * 77777);
        // data = (j * 2);
        // if( i < 40 ) {
        //     data = i;
        // } else {
        //     data = (i * 77777);
        // }
        input_data.push_back(base+data);
    }
}

static void _updateCache(const uint32_t count, std::vector<uint32_t>& cache, uint32_t& seed, bool& cacheValid) {
    if( count < cache.size() && cacheValid ) {
        return;
    }

    std::vector<uint32_t> out;

    const uint32_t base = (seed & 0xff) << 24;
    genhack(out, count, base);

    // same as cache = out
    // but allows us to be more efficient
    std::swap(cache, out);

    cacheValid = true;
    // cacheValid = false;

}


void AirMacBitrateTx::updateCache(const uint32_t count) {
    _updateCache(count, cache, seed, cacheValid);
}


std::vector<uint32_t> AirMacBitrateTx::getCache(void) {
    return cache;
}



void AirMacBitrateTx::setSeed(const uint32_t s) {
    seed = s;
}

std::vector<uint32_t> AirMacBitrateTx::generate(const uint32_t count, const uint32_t arg) {

    if( count == 0 ) {
        return {};
    }

    updateCache(count);


    std::vector<uint32_t> out;

    out.assign(cache.begin(), cache.begin()+count);

    if( out.size() > 0 ) {
        out[0] = arg;
    }

    for(unsigned i = 1; i < out.size(); i++) {
        if( i % 20 == 0 ) {
            out[i] = out[0];
        }
    }



    // const uint32_t base = (seed & 0xff) << 24;
    // genhack(out, count, base);

    return out;
}


///////////////////////////////////////

using system_clock = std::chrono::system_clock;
// using duration_cast = std::chrono::duration_cast;
using milliseconds = std::chrono::milliseconds;


static uint8_t hamming_32(const uint32_t a, const uint32_t b) {
    uint8_t out = 0;
    for(unsigned i = 0; i < 32; i++){
        if( ((a>>i)&0x1) != ((b>>i)&0x1) ) {
            out++;
        }

    }
    return out;
}

// skips beginning and compares ends
static uint32_t hamming_vector(const std::vector<uint32_t>& va, const std::vector<uint32_t>& vb, const unsigned skip = 1) {
    uint32_t out = 0;

    unsigned eend = std::min(va.size(), vb.size());

    for(unsigned i = skip; i < eend; i++) {
        out += hamming_32(va[i], vb[i]);
    }
    return out;
}

void AirMacBitrateRx::resetBer(void) {
    bits_correct = 0;
    bits_wrong = 0;
}

double AirMacBitrateRx::getBer(void) {
    auto total = bits_correct + bits_wrong;

    if( total == 0) {
        return 0.0;
    }

    return (double)bits_wrong / total;
}


void AirMacBitrateRx::got(const std::vector<uint32_t>& packet) {

    const uint32_t len = packet.size();

    const uint32_t got_bits = packet.size()-1;   // we don't compare the first one?
    const uint32_t got_seq = (len>0)?packet[0]:0;

    updateCache(len);


    for(unsigned i = 1; i < cache.size(); i++) {
        if( i % 20 == 0 ) {
            cache[i] = packet[0];
        }
    }

    if( got_seq != (last_seq+1)) {
        cout << "AirMac sequence was NON LINEAR\n";
        // FIXME bump incorrect counter
    }





    // cout << "cache: " << "\n";
    // for(const auto w : cache) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    // cout << "ggot: " << "\n";
    // for(const auto w : packet) {
    //     cout << HEX32_STRING(w) << "\n";
    // }



    bool same = std::equal(cache.begin()+1, cache.end(), packet.begin()+1);


    if( mac_print1 ) {
        if( same ) {
            cout <<"SAME" << "\n";
        } else {
            cout <<"NOT SAME" << "\n";
        }
    }

    unsigned errors = 0;
    if( !same ) {

        auto copy_cache = cache;
        copy_cache.resize(packet.size());

        errors = hamming_vector(cache, packet);

        if( mac_print1 ) {
            cout << "NOT SAME found " << errors << " hamming errors\n";
        }
    }

    bits_correct += got_bits - errors;
    bits_wrong += errors;

    if( mac_print1 ) {
        cout << "BER: " << getBer() << "\n";
    }



    milliseconds ms = 
        std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    updateDataRate(ms, len);
    
    // cout << ms.count() << "\n";

    // std::vector<uint32_t> out;
    // out.assign(cache.begin(), cache.begin()+len);


    last_seq = got_seq;

}


void AirMacBitrateRx::updateDataRate(const std::chrono::milliseconds ms, const uint32_t size) {
    if (!data_rate.empty()) {
            uint64_t time_diff = ms.count() - data_rate.back().first;
        double bits = size * 32;
        double instantaneous_data_rate = bits / time_diff * 1000.0 / 1048576.0;
        if( mac_print2 ) {
            std::cout << "\n";
            std::cout << "Time Difference: "
                      << time_diff << " ms\n"
                      << "Packet Size: " << size << " words\n" 
                      << "Total Bits: " << bits << " bits\n"
                      << "Instantaneous Data Rate: " << instantaneous_data_rate
                      << " Mbps\n";
            std::cout << "\n";
        }
    }


    std::pair<uint64_t, uint32_t> info(ms.count(), size);
    data_rate.push_back(info);

#ifdef ZZZZZZZZZZZZDFaSD

    // if (data_rate.back().first - data_rate[0].first >= 1000) {
    //     double total_words = 0.0;
    //     double rate;
    //     for (std::size_t i = 0; i < data_rate.size(); i++) {
    //         total_words += data_rate[i].second;
    //     }
    //     total_words *= 32;
    //     rate = total_words / (data_rate.back().first - data_rate[0].first) / 1000;
    //     if( print20 ) {
    //         std::cout << "Data Rate: " << rate << " Mbps t: "
    //                   << data_rate.back().first - data_rate[0].first << " ms "
    //                   << " Bits: " << total_words << "\n";
    //     }
    //     data_rate.clear();
    //     data_rate.push_back(info);
    // }

#endif

}




void AirMacBitrateRx::setSeed(const uint32_t s) {
    seed = s;
}

void AirMacBitrateRx::updateCache(const uint32_t count) {
    _updateCache(count, cache, seed, cacheValid);
}