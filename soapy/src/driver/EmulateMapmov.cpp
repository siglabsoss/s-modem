#include "EmulateMapmov.hpp"
#include "feedback_bus_types.h"


#include <assert.h>
#include <iostream>
#include <algorithm>


using namespace std;


static const int fft_size = 1024;


// static int _fftshift(int x) {
//     if( x < 512 ) {
//         return x + 512;
//     } else {
//         return x - 512;
//     }
// }

static int _tx_rx(int x) {
    if( x == 0 ) {
        return 0;
    }
    return 1024 - x;
}


EmulateMapmov::EmulateMapmov(uint16_t x) :
enabled_subcarriers(0)
,subcarrier_allocation(x)
,modulation_schema(FEEDBACK_MAPMOV_QPSK)
{
    settings_did_change();
}

void EmulateMapmov::settings_did_change(void) {
    // wipe and init with zeros
    enable_table.resize(0);
    enable_table.resize(fft_size);
    pilot_table.resize(0);
    pilot_table.resize(fft_size);

    build_enable_subcarrier_table();
    build_pilot_table();
}

void EmulateMapmov::set_subcarrier_allocation(const uint16_t allocation_value) {
    // cout << "EmulateMapmov setting " << allocation_value << "\n";

    subcarrier_allocation = allocation_value;
    settings_did_change();
}

uint16_t EmulateMapmov::get_subcarrier_allocation(void) const {
    return subcarrier_allocation;
}

void EmulateMapmov::set_modulation_schema(const uint8_t schema_value) {
    modulation_schema = schema_value;
    // settings_did_change(); // we dont need to call this now
}

uint8_t EmulateMapmov::get_modulation_schema(void) const {
    return modulation_schema;
}

unsigned EmulateMapmov::get_enabled_subcarriers(void) const {
    return enabled_subcarriers;
}




void EmulateMapmov::build_enable_subcarrier_table(void) {
    uint32_t l_count = 0; // local count
    for(int sc = 0; sc < fft_size; sc++) {
        switch(subcarrier_allocation) {
            case MAPMOV_SUBCARRIER_128:
                enable_table[sc] = ((sc & 0x1) && (sc < 129 || sc > 896));
                break;
            case MAPMOV_SUBCARRIER_320:
                enable_table[sc] = ((sc & 0x1) && (sc < 321 || sc > 704));
                break;
            case MAPMOV_SUBCARRIER_640_LIN:
                break;
            case MAPMOV_SUBCARRIER_512_LIN:
                break;
            case MAPMOV_SUBCARRIER_64_LIN:
                enable_table[sc] = (sc >= 16 && sc < 80);
                break;
            default:
                cout << "Illegal value of subcarrier_allocation: " << subcarrier_allocation << "\n";
                assert(false);
                break;
        }

        if( enable_table[sc] ) {
            l_count++;
        }
    }
    enabled_subcarriers = l_count; // do a single atomic assign to reduce lock spam
}


void EmulateMapmov::build_pilot_table(void) {
    
    const uint32_t pilot = 0x2000;
    // Pilot = 0, Data = 1
    for(int sc = 0; sc < fft_size; sc++) {
        switch (subcarrier_allocation) {
            case MAPMOV_SUBCARRIER_128:
                pilot_table[sc] = (sc <= 128 || sc >= 896) ? pilot : 0;
                break;
            case MAPMOV_SUBCARRIER_320:
                pilot_table[sc] = (!(sc & 0x1) &&
                                   (sc <= 323 || sc >= 701)) ? pilot : 0;
                break;
            case MAPMOV_SUBCARRIER_640_LIN:
                break;
            case MAPMOV_SUBCARRIER_512_LIN:
                break;
            case MAPMOV_SUBCARRIER_64_LIN:
                pilot_table[sc] = 0;
                break;
            default:
                cout << "Illegal value of subcarrier_allocation: " << subcarrier_allocation << "\n";
                assert(false);
                break;
        }
    }
}

void EmulateMapmov::map_input_data(const std::vector<uint32_t> &input_data,
                               std::vector<uint32_t> &mapmov_data) {
    const uint32_t map[4] = {
        0xe95fe95f,
        0x16a1e95f,
        0xe95f16a1,
        0x16a116a1
    };

    for (uint32_t word : input_data) {
        for (int i = 0; i < 16; i++) {
            const int bits = (word >> (i*2)) & 0x3;
            mapmov_data.push_back(map[bits]);
        }
    }
}

void EmulateMapmov::move_input_data(const std::vector<uint32_t> &mapmov_mapped,
                                std::vector<uint32_t> &mapmov_out) {
    unsigned consumed = 0;

    while (consumed < mapmov_mapped.size()) {
        for (unsigned int sc = 0; sc < fft_size; sc++) {
            if (enable_table[sc]) {
                mapmov_out.push_back( mapmov_mapped[consumed] );
                consumed++;

                if (consumed > mapmov_mapped.size()) {
                    std::cout << "Function out of bound" << std::endl;
                }

            } else {
                mapmov_out.push_back( pilot_table[sc] );
            }
        }
    }
}


void EmulateMapmov::receive_tx_data(std::vector<uint32_t> &rx_out,
                                const std::vector<uint32_t> &mapmov_out) {
    // Move data from TX to RX
    for( unsigned i = 0; i < mapmov_out.size(); i++ ) {
        const unsigned mod = i % fft_size;
        const unsigned base = i - mod;
        const unsigned output_index = base + _tx_rx(mod);

        // Emulate FFT transform
        rx_out[output_index] = mapmov_out[i];
    }
}



void EmulateMapmov::rx_reverse_mover(std::vector<uint32_t> &vmem_mover_out,
                                 const std::vector<uint32_t> &rx_out) const {
    std::vector<uint32_t> reverse_mover;
    bool linear = false;
    uint32_t lower_bound = 0;
    uint32_t upper_bound = 0;

    switch(subcarrier_allocation) {
        case MAPMOV_SUBCARRIER_128:
            lower_bound = 129;
            upper_bound = 896;
            linear = false;
            break;
        case MAPMOV_SUBCARRIER_320:
            lower_bound = 321;
            upper_bound = 704;
            linear = false;
            break;
        case MAPMOV_SUBCARRIER_640_LIN:
            break;
        case MAPMOV_SUBCARRIER_512_LIN:
            break;
        case MAPMOV_SUBCARRIER_64_LIN:
            lower_bound = 945;
            upper_bound = 1009;
            linear = true;
            break;
        default:
            assert(false);
            break;
    }
    if (linear) {
        for (std::size_t i = lower_bound; i < upper_bound; i++) {
            reverse_mover.push_back(i);
        }
    } else {
        for (std::size_t i = 1; i < fft_size; i += 2) {
            if (i == lower_bound) i = upper_bound + 1;
            reverse_mover.push_back(i);
        }
    }
    for( unsigned i = 0; i < rx_out.size(); i++ ) {
        unsigned mod = i % fft_size;

        auto it = find (reverse_mover.begin(), reverse_mover.end(), mod);
        if (it != reverse_mover.end()) {
            vmem_mover_out.push_back(rx_out[i]);
        }

    }
}

void EmulateMapmov::slice_rx_data(const std::vector<uint32_t> &vmem_mover_out,
                              std::vector<uint32_t> &sliced_out_data) {
    uint32_t word = 0;
    for(unsigned i = 0; i < vmem_mover_out.size(); i++) {
        unsigned j = i % 16;
        uint32_t input = vmem_mover_out[i];
        uint32_t bits = ( (input >> 30) & 0x2 ) | ( (input >> 15) & 0x1 );
        word = (word >> 2) | (bits<<30);

        if( j == 15 ) {
            sliced_out_data.push_back(word);
            word = 0;
        }
    }
}


void EmulateMapmov::print_enable_subcarrier_table(const std::vector<bool> &_enable_table,
                                              const int subcarriers_enabled) {
    std::cout << "Printing enabled subcarriers table" << std::endl;
    for (int sc = 0; sc < subcarriers_enabled; sc++) {
        std::cout << (int) _enable_table[sc] << std::endl;
    }
}
