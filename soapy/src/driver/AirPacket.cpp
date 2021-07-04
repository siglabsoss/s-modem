#include "AirPacket.hpp"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <set>
#include <chrono>
#include <ctime>
#include "cpp_utils.hpp"
#include "ReedSolomon.hpp"
#include "Interleaver.hpp"
#include "EmulateMapmov.hpp"

// AirPacket

AirPacket::AirPacket() :
 enabled_subcarriers(0)
,sliced_word_count(8)
,header_overhead_words(8*2)
,bits_per_subcarrier(2)
,subcarrier_allocation(MAPMOV_SUBCARRIER_128)
,modulation_schema(FEEDBACK_MAPMOV_QPSK)
,code_type(AIRPACKET_CODE_UNCODED)
,code_length(0)
,fec_length(0)
,code_puncturing(0)
,interleave_length(0)
,duplex_mode(false)
,emulate_mapmov(new EmulateMapmov())
{}


// not that efficent but easy to read funciton that sets
// print bools based on string
void AirPacket::enableDisablePrint(const std::string s, const bool v) {
    std::vector<std::pair<std::string,bool*>> cat;

    {
        const std::string n = "nondup";
        cat.emplace_back(n, &print1);
        cat.emplace_back(n, &print3);
        cat.emplace_back(n, &print2);
        cat.emplace_back(n, &print8);
        cat.emplace_back(n, &print9);
        cat.emplace_back(n, &print10);
        cat.emplace_back(n, &print11);
        cat.emplace_back(n, &print13);
        cat.emplace_back(n, &print14);
    }
    {
        const std::string n = "demod";
        cat.emplace_back(n, &print6);
        cat.emplace_back(n, &print15);
        cat.emplace_back(n, &print7);
        cat.emplace_back(n, &print16);
        cat.emplace_back(n, &print20);
        cat.emplace_back(n, &print21);
        cat.emplace_back(n, &print23);
        cat.emplace_back(n, &print24);
        cat.emplace_back(n, &print25);
        cat.emplace_back(n, &print26);
    }
    {
        const std::string n = "mod";
        cat.emplace_back(n, &print5);
        cat.emplace_back(n, &print17);
        cat.emplace_back(n, &print18);
        cat.emplace_back(n, &print19);
        cat.emplace_back(n, &print22);
        cat.emplace_back(n, &print27);
    }

    unsigned count = 0;
    for(const auto pp : cat) {
        bool *p;
        std::string name;
        std::tie(name,p) = pp;
        if( s == "all" || s == name ) {
            *p = v;
            count++;
        }
    }
    std::cout << (v?"Enabling":"Disabling") << " " << count  << " prints for key: " << s << "\n";

}
void AirPacket::enablePrint(const std::string s) {
    enableDisablePrint(s, true);
}
void AirPacket::disablePrint(const std::string s) {
    enableDisablePrint(s, false);
}


unsigned AirPacket::get_enabled_subcarriers(void) const {
    return enabled_subcarriers;
}




void AirPacket::set_interleave(const uint32_t x) {
    interleave_length = x;
    if( x == 0 ) {
        std::cout << "Disabling interleave\n";
        interleaver = 0;
        return;
    }

    if( every_interleaver.find(x) != every_interleaver.end() ) {
    // std::cout << "using existing object\n";
        interleaver = every_interleaver[x];
    } else {
    // std::cout << "making new object for " << _code_length << "," << _fec_length << "\n";
        interleaver = new Interleaver(x);
        every_interleaver[x] = interleaver;
    }
}

uint64_t AirPacket::rsKey(uint32_t a, uint32_t b) {
    return (((uint64_t)a)<<32) | b;
}
std::pair<uint32_t,uint32_t> AirPacket::rsSettings(uint64_t x) {
    uint32_t a;
    uint32_t b;
    a = (x>>32)&0xffffffff;
    b = x&0xffffffff;
    return std::make_pair(a,b);
}

int AirPacket::set_rs(const uint32_t len) {
    if( len == 0) {
        return set_code_type(AIRPACKET_CODE_UNCODED, 0, 0);
    } else {
        return set_code_type(AIRPACKET_CODE_REED_SOLOMON, 255, len);
    }
}

int AirPacket::get_rs(void) const {
    return fec_length;
}

int AirPacket::get_interleaver(void) const {
    return interleave_length;
}

unsigned AirPacket::get_sliced_word_count(void) const {
    return sliced_word_count;
}

// returns 0 for success
int AirPacket::set_code_type(const uint8_t _code_value,
                             const uint32_t _code_length,
                             const uint32_t _fec_length) {

    int error = 1;
    // check ok first
    if( _code_value == AIRPACKET_CODE_REED_SOLOMON ) {
        bool rs_valid = ReedSolomon::settings_valid(_code_length, _fec_length);
        if( rs_valid ) {
            code_type = _code_value;
            code_length = _code_length;
            fec_length = _fec_length;
            error = 0;

            auto key = rsKey(_code_length, _fec_length);
            // std::cout << HEX64_STRING(key) << "\n";
            if( every_reed_solomon.find(key) != every_reed_solomon.end() ) {
                // std::cout << "using existing object\n";
                reed_solomon = every_reed_solomon[key];
            } else {
                // std::cout << "making new object for " << _code_length << "," << _fec_length << "\n";
                reed_solomon = new ReedSolomon(_code_length, _fec_length);
                every_reed_solomon[key] = reed_solomon;
            }



        }
    } else if(_code_value == AIRPACKET_CODE_UNCODED) {
        code_type = _code_value;
        code_length = 0;
        fec_length = 0;
        error = 0;
    } else {
        error = 1;
    }


    if( error != 0) {
        std::cout << "set_code_type called with illegal arguments: " << (int)_code_value << ", " << _code_length << ", " << _fec_length << "\n";
    }

    return error;
}

void AirPacket::set_code_puncturing(const uint8_t puncturing_value) {
    code_puncturing = puncturing_value;
    settings_did_change();
}

void AirPacket::set_modulation_schema(const uint8_t schema_value) {

    switch(schema_value) {
        default:
            std::cout << "set_modulation_schema() got an unknown value of " << schema_value << "\n";
            [[gnu::fallthrough]]; // This is meant to fallthrough. It gives a warning on ben's gcc, but fixes warning on nuc gcc
        case FEEDBACK_MAPMOV_QPSK:
            bits_per_subcarrier = 2;
            break;
        case FEEDBACK_MAPMOV_QAM8:
            bits_per_subcarrier = 3;
            break;
        case FEEDBACK_MAPMOV_QAM16:
            bits_per_subcarrier = 4;
            break;
        case FEEDBACK_MAPMOV_QAM32:
            bits_per_subcarrier = 5;
            break;
        case FEEDBACK_MAPMOV_QAM64:
            bits_per_subcarrier = 6;
            break;
        case FEEDBACK_MAPMOV_QAM128:
            bits_per_subcarrier = 7;
            break;
    }

    modulation_schema = schema_value;
    settings_did_change();
}

uint8_t AirPacket::get_modulation_schema(void) const {
    return modulation_schema;
}

void AirPacket::set_subcarrier_allocation(const uint16_t allocation_value) {
    switch(allocation_value) {
        case MAPMOV_SUBCARRIER_128:
            enabled_subcarriers = 128;
            break;
        case MAPMOV_SUBCARRIER_320:
            enabled_subcarriers = 320;
            break;
        case MAPMOV_SUBCARRIER_640_LIN:
            enabled_subcarriers = 640;
            break;
        case MAPMOV_SUBCARRIER_512_LIN:
            enabled_subcarriers = 512;
            break;
        case MAPMOV_SUBCARRIER_64_LIN:
            enabled_subcarriers = 64;
            break;
        default:
            assert(false);
            break;
    }

    subcarrier_allocation = allocation_value;
    emulate_mapmov->set_subcarrier_allocation(allocation_value);
    settings_did_change();
}

uint16_t AirPacket::get_subcarrier_allocation(void) const {
    return subcarrier_allocation;
}

void AirPacket::set_duplex_start_frame(const uint32_t start_frame) {
    duplex_start_frame = start_frame;
}

void AirPacket::settings_did_change(void) {

    sliced_word_count = enabled_subcarriers * bits_per_subcarrier / 32;

    const auto effective_sliced_word_count = getEffectiveSlicedWordCount();

    if(print_settings_did_change) {
        std::cout << "-----\n";
        std::cout << "-\n";
        std::cout << "- enabled_subcarriers          :    " << enabled_subcarriers << "\n";
        std::cout << "- bits_per_subcarrier          :    " << bits_per_subcarrier << "\n";
        std::cout << "- sliced_word_count            :    " << sliced_word_count << "\n";
        std::cout << "- effective_sliced_word_count  :    " << effective_sliced_word_count << ((effective_sliced_word_count!=sliced_word_count)?" !!":"") << "\n";
        std::cout << "-\n";
        std::cout << "-----\n";
    }

    setSyncMessage();
}

uint32_t AirPacket::getRbForRxSlicer(void) {
    return modulation_schema;
}

// some functions / modes were written before sliced_word_count was changing correctly
// so we fix this at old values for certain functions
uint32_t AirPacket::getEffectiveSlicedWordCount() const {
    uint32_t val = sliced_word_count;

    if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
        val = 8;
    }

    return val;
}

// Between demodulateSlicedHeader and demodulateSliced, some code outside airpacket makes some
// decisions.
// this is how many words between the first word of the vertical header till the first word of the data packet
// for qpsk this would be 16 frames for the sync word, and then 16 frames for the header
// note this is 16 frames regardless of how many subcarriers are enabled, HOWEVER the returned number
// must factor the number of subcarriers because this affects words per frame and the return value is in words
uint32_t AirPacket::getForceDetectIndex(const uint32_t fount_at_index) const {
    uint32_t bump;

    // bump = 32*8;
    if (duplex_mode) {
        bump = 1;
    } else if( subcarrier_allocation == MAPMOV_SUBCARRIER_320 && modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
        bump = enabled_subcarriers * 4; // not sure what this is
        // 1280
        // std::cout << "getForceDetectIndex() bump: " << bump << "\n";
        // maybe also sliced_word_count*34?
    } else if (subcarrier_allocation == MAPMOV_SUBCARRIER_320 && modulation_schema == FEEDBACK_MAPMOV_QPSK) {
        bump = (sliced_word_count*32.0/bits_per_subcarrier) * 2;
    } else {
        bump = (sliced_word_count*32.0);
    }

    if( print16 ) {
        std::cout << "getForceDetectIndex() bump: " << bump << "\n";
    }

    return fount_at_index + bump;
}

AirPacketOutbound::AirPacketOutbound() : AirPacket() {

}



// pass an fpga, a ringbus type, and a list of gains
// returns ringbus vector of vector
static std::vector<std::vector<uint32_t>> gain_for_fpga(const uint32_t fpga, const uint32_t rb_word, const uint32_t gain[5]) {

    uint32_t bits[5] = {
        0x00000
        ,0x10000
        ,0x20000
        ,0x30000
        ,0x40000
    };

    std::vector<std::vector<uint32_t>> out;
    for( auto i = 0u; i < 5u; i++ ) {
        out.emplace_back(
            std::initializer_list<uint32_t> {fpga,rb_word | bits[i] | gain[i]} // way for a "simple" add to vector of vector
        );
    }

    constexpr bool print = false;

    if(print) {
        for(auto w : out) {
            for(auto x : w) {
                std::cout << HEX32_STRING(x) << ",";
            }
            std::cout << "\n";
        }
    }

    return out;
}

/*
 * Call after switching modes on tx side
 * Returns a list of ringbus to send to adjust the tx gain stages of fft
 * returns a vector of vector where subvector is <fpga,ringbus> and parent
 * vector is a list.  Remember it can be empty if nothing to send
 * pass in some needed info about ringbus, this lets us avoid including ringbus.h here
 * and makes this more standalone
 */
std::vector<std::vector<uint32_t>> AirPacketOutbound::getGainRingbus(const uint32_t cs10_addr, const uint32_t cs10_gain_rb) const {
    if( subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
        uint32_t bs[5] = {0x0f, 0x10, 0x0f, 0x0f, 0x0f};

        return gain_for_fpga(cs10_addr, cs10_gain_rb, bs);
    } else {
        return {};
    }
}


static uint32_t _get_demod_2_bits(int32_t data) {
    uint32_t bit_from_real;
    uint32_t bit_from_imag;
    uint32_t bit_value;

    bit_from_imag = data > 0 ? 1:0;
    bit_from_real = (data << 16) > 0 ? 1:0;
    bit_value  = (bit_from_real << 1) | bit_from_imag;

    return bit_value;
}



static uint32_t _extract_single(const sliced_buffer_t &data, int j, int i) {
    uint32_t demod_word = 0;

    for(int k = i; k < i+16; k++)
    {
        uint32_t demod_bits = data[j][k];
        demod_word = (demod_word<<2 | demod_bits);
    }
    return demod_word;
}

static uint32_t _extract_single_smodem(
    const iq_buffer_t &data,
    int j, int i) {
    uint32_t demod_word = 0;

    for(unsigned k = i; (unsigned)k < (unsigned)i+16u; k++)
    {
        uint32_t demod_bits = _get_demod_2_bits(data[j][k].first);
        demod_word = (demod_word<<2 | demod_bits);
    }
    return demod_word;
}



///
/// Pushes a "vertical" header to the vector passed in argument
/// Note so far we always pass an empty output_vector so this is at the head
void AirPacketOutbound::attachHeader(const uint32_t data_size, std::vector<uint32_t>& output_vector, const uint8_t seq) const {
    if (duplex_mode) {
        uint8_t from = 0;
        uint8_t flags = 0;
        add_mac_header(output_vector, data_size, seq, from, flags);
    } else {
        const unsigned int header_sync_subcarriers_qpsk = enabled_subcarriers;

        // These 2 functions were written with qpsk in mind.

        // if qpsk, dont touch
        // if qam16 multiply by 2 because bits per subcarrier went up by 2
        const unsigned data_sync_arg =
            ( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) ?
                header_sync_subcarriers_qpsk*2 : header_sync_subcarriers_qpsk;

        
        // this the argument is "subcarriers" in qpsk.
        // enabled_subcarriers * 1 / (32/bits_per_subcarrier)

        // throw on sync word
        const uint32_t initial_sync_word = 0xbeefbabe;
        new_subcarrier_data_sync(&output_vector,
                                 initial_sync_word,
                                 data_sync_arg);

        // figure out header

        // uint32_t seq = getTxSeq();
        uint8_t flags = 0;
        const uint32_t header_word = AirPacket::packHeader(data_size, seq, flags);

        if( print11 ) {
            std::cout << "attachHeader() using: " << HEX32_STRING(header_word) << "\n";
        }


        new_subcarrier_data_sync(&output_vector,
                                 header_word,
                                 data_sync_arg);
    }


}
uint8_t AirPacketOutbound::attachHeader(const uint32_t data_size, std::vector<uint32_t>& output_vector) {
    uint8_t seq = getTxSeq();
    if (duplex_mode) {
        uint8_t from = 0;
        uint8_t flags = 0;
        add_mac_header(output_vector, data_size, seq, from, flags);
    } else {
        attachHeader(data_size, output_vector, seq);
    }
    return seq;
}

void AirPacketOutbound::add_mac_header(std::vector<uint32_t> &output_vector,
                                       uint32_t length,
                                       uint16_t seq_num,
                                       uint8_t from,
                                       uint8_t flags) const {
    uint32_t upper_header;
    uint32_t lower_header;
    MacHeader mac_header;
    mac_header.create_header(upper_header,
                             lower_header,
                             length,
                             seq_num,
                             from,
                             flags);
    for (std::size_t i = 0; i < sliced_word_count / 2; i++) {
        output_vector.push_back(upper_header);
        output_vector.push_back(lower_header);
    }

}

uint32_t AirPacket::getLengthForInput(const uint32_t length) {
    auto roundToMultipleCeil = [](uint32_t num, uint32_t round) {
                uint32_t val = num;
                uint32_t mod = num % round;
                if(mod) {
                val += (round - mod);
                }
                return val;
            };
    auto roundToMultipleFloor = [](uint32_t num, uint32_t round) {
                 uint32_t val = num;
                 uint32_t mod = num % round;
                 val -= mod;
                 return val;
             };
    // round to fec length
    uint32_t interleave_length_ceil = (interleave_length > 0) ? interleaver->calculate_words_for_code(interleave_length):0;//roundToMultipleCeil(interleave_length, 4);
    //std::cout << "length: " << length << " fec_length: " << fec_length << " code_length: " << code_length << " code_type: " << int(code_type) << " interleave_length " << interleave_length << std::endl;
    uint32_t effective_rs_code_length = roundToMultipleFloor(255-fec_length, 4);
    uint32_t retLength = (code_type == AIRPACKET_CODE_REED_SOLOMON) ? roundToMultipleCeil(4*length, effective_rs_code_length) : 4*length;
    //std::cout << "here: " << retLength << "\n\n\n" << std::endl;
    //std::cout << "--------------------------------------" << std::endl;
    retLength = (code_type == AIRPACKET_CODE_REED_SOLOMON) ? retLength/(effective_rs_code_length)*256:retLength;
    //std::cout << "rs_length: " << retLength << std::endl;
    // round to word boundary
    retLength = (code_type == AIRPACKET_CODE_REED_SOLOMON) ? roundToMultipleCeil(retLength, 4): retLength;
    //std::cout << "rs_ByteWordPad: " << retLength << std::endl;
    // round to interleave size
    retLength = (interleave_length > 0) ? roundToMultipleCeil(retLength,4*interleave_length_ceil):retLength;
    //std::cout << "interleavePad: " << retLength << std::endl;
    //convert to words and add seq length
    retLength = retLength/4 + sliced_word_count*32;
    //std::cout << "finall: " << retLength << std::endl;
    return retLength;
}

//
// Pass in a std vector of words
// A new vector is produced on the output which is aware of how many
// subcarriers there are and has arranged words vertically per subcarrier.
// Two words are added: a sync word and a header word. Padding is available if
// enabled.
//
std::vector<uint32_t> AirPacketOutbound::transform(const std::vector<uint32_t> &a, uint8_t& seq_used, const unsigned pad_to) {
    // Use meaning names, "o" doesn't provide any meaningful information
    // std::vector<uint32_t> o;
    std::vector<uint32_t> output_vector;
    std::vector<uint32_t> encoded_vector; // only used for reed solomon
    std::vector<uint32_t> interleave_vector; // only used for interleaver





    // pointer to the data we will loop and push onto the packet
    const std::vector<uint32_t>* to_push = &a;

    if( code_type == AIRPACKET_CODE_REED_SOLOMON ) {

        // Encode vector of data before transmitting
        reed_solomon->encode_message(a,
                                    encoded_vector,
                                    code_length,
                                    fec_length);

        to_push = &encoded_vector;

    } else {
        to_push = &a;
    }
    if( print17 ) { 
        std::cout << "Interleave Length is: " << interleave_length << "\n";
    }
    if( interleave_length != 0 ) {

        // for(const auto w : *to_push) {
        //     std::cout << "  x   " << HEX32_STRING(w) << "\n";
        // }
        // std::cout << "\n";
        if( print17 ) {
            std::cout << "Begin interleaving. Interleaving vector has size: "
                      << to_push->size() << "\n";
        }
        interleaver->interleave(*to_push, interleave_vector);
        to_push = &interleave_vector;
        if( print17 ) {
            std::cout << "interleave_vector size: " << interleave_vector.size()  << " interleave_length brah " << interleave_length << "\n";
        }
        // for(const auto w : *to_push) {
        //     std::cout << "  y   " << HEX32_STRING(w) << "\n";
        // }
    }


    /// below this line is only dealing with the header


    if( print5 ) {
        std::cout << "packing header with " << enabled_subcarriers
        << " enabled_subcarriers "
        << " header_sync_subcarriers "
        << enabled_subcarriers // print may be wrong (2* in attachHeader())
        << "\n";
    }




    /// output_vector is empty, this pushes the header
    /// note if vector is not empty this will push on to the END which
    /// is not what we want
    ///  returns the sequence number, we write directly to the argument& (which goes out)
    ///
    if( print18 ) {
        std::cout << "Before attaching header my size is: " << to_push->size() << "\n";
    }
    attachHeader(to_push->size(), output_vector, seq_used);
    if( print18 ) {
        std::cout << "I added a header of size: " << output_vector.size() << "\n";
    }

    for(auto w : *to_push) {
        output_vector.push_back(w);
    }

    if( pad_to != 0 ) {
        while(output_vector.size() < pad_to) {
            output_vector.push_back(0);
        }
    }
    if( print18 ) {
        std::cout << "After transform my size is: " << output_vector.size() << "\n";
    }
    return output_vector;
}


void AirPacket::set_duplex_mode(bool set_mode) {
    std::cout << "Setting duplex mode to: " << set_mode << "\n";
    duplex_mode = set_mode;
}

// Warning there is an different function with the same name (get_duplex_mode) in duplex_schedule.c
bool AirPacket::get_duplex_mode() const {
    return duplex_mode;
}


void AirPacket::format_rx_data(sliced_buffer_t &final_rx_data,
                               const std::vector<uint32_t> &sliced_out_data,
                               const uint32_t counter_start) const {
    // Format into final format, (Add counter)
    // This is "20" words and we only change the first, and then add it over and over to sliced_data[0]
    std::vector<uint32_t> header(sliced_word_count, 0);
    final_rx_data.resize(2);
    uint32_t output_counter = counter_start;
    const uint32_t data_chunk = sliced_out_data.size() / sliced_word_count;
    for (std::size_t i = 0; i < data_chunk; i++) {
        header[0] = output_counter;
        final_rx_data[0].insert(final_rx_data[0].end(),
                                header.begin(),
                                header.end());
        final_rx_data[1].insert(
                         final_rx_data[1].end(),
                         sliced_out_data.begin() + (i * sliced_word_count),
                         sliced_out_data.begin() + ((i + 1) * sliced_word_count));
        output_counter++;
    }
}
///
/// Copies the steps that occur between data bound for mapmov, and data received on the other end
///
sliced_buffer_t AirPacket::emulateHiggsToHiggs(const std::vector<uint32_t> &a,
                                               const uint32_t counter_start) {
    
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

    // this is AirPacket Specific, so it's not part of emulate_mapmov
    format_rx_data(final_rx_data, sliced_out_data, counter_start);

    return final_rx_data;
}



// some tests use this, added when removed setModulationIndex
void AirPacket::setTestDefaults(void) {
    set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    set_subcarrier_allocation(MAPMOV_SUBCARRIER_128);
}

// void AirPacket::_setModulateIndex(uint32_t) {
//     std::cout << "AirPacket::_setModulateIndex" << std::endl;
// }

void AirPacket::printWide(const std::vector<uint32_t> &d, const unsigned sc) const {
    std::cout << "Printing vector with " << d.size() << " words, with " << sc << " subcarriers" << std::endl;
    std::cout << "------------" << std::endl;
    // std::// cout << "d was: " << d.size() << std::endl;
    int i = 0;
    int wrap = sc/16;
    for( auto w : d ) {
        std::cout << HEX32_STRING(w) << " ";
        if( i % wrap == (wrap-1) ) {
            std::cout << std::endl;
        }
        i++;
    }
}


template <class T>
static std::tuple<unsigned, unsigned, uint32_t> _demodulateSlicedHeaderHelper(
    const sliced_buffer_t &as_iq,
    const T &transform,
    const unsigned sliced_word_count,
    const unsigned i,
    const uint32_t sync_word,
    const uint8_t modulation_schema) {

    // for each 8, count matches
    unsigned count_set = 0;
    unsigned lookup_i = 0;
    uint32_t word0;
    for(unsigned j = 0; j < sliced_word_count; j++) {

        auto optional_result = transform.t(i+j);
        lookup_i = optional_result.second;
        uint32_t word;
        if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
            word = bitMangleSlicedQam16(as_iq[1][lookup_i]);
        } else {
            word = bitMangleSliced(as_iq[1][lookup_i]);
        }

        if( j == 0 ) {
            word0 = word;
        }

        // std::cout << i+j << ": " << HEX32_STRING(word) << std::endl;

        uint32_t xor_result = sync_word ^ word;
        count_set += count_bit_set(xor_result);
    }

    return std::make_tuple(count_set,lookup_i,word0);
}

static std::tuple<unsigned, unsigned, uint32_t> _demodulateSlicedHeaderHelperUntransformed(
    const sliced_buffer_t &as_iq,
    const unsigned sliced_word_count,
    const unsigned i,
    const uint32_t sync_word,
    const uint8_t modulation_schema,
    const bool print_all = false) {

    // for each 8, count matches
    unsigned count_set = 0;
    unsigned lookup_i = 0;
    uint32_t word0;
    for(unsigned j = 0; j < sliced_word_count; j++) {

        // auto optional_result = transform.t(i+j);
        lookup_i = i+j;//optional_result.second;
        uint32_t word;
        if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
            word = bitMangleSlicedQam16(as_iq[1][lookup_i]);
        } else {
            word = bitMangleSliced(as_iq[1][lookup_i]);
        }

        if( j == 0 ) {
            word0 = word;
        }


        uint32_t xor_result = sync_word ^ word;
        count_set += count_bit_set(xor_result);

        if( print_all ) {
            std::cout << i+j << ": " << HEX32_STRING(word) << " (" << count_set << ")" << std::endl;
        }
    }

    return std::make_tuple(count_set,lookup_i,word0);
}


/// This function sorts and votes on the input words
///
/// @param[in] vector of words
std::tuple<bool, air_header_t> AirPacketInbound::parseLengthHeader(const std::vector<uint32_t>& r) const {

    const auto ret_invalid = std::make_tuple(false, std::make_tuple((uint32_t)0, (uint8_t)0, (uint8_t)0));

    if( r.size() != enabled_subcarriers ) {
        std::cout << "illegal input size in parseLengthHeader() " << r.size() << std::endl;
        return ret_invalid;
    }

    // for( auto x : r ) {
    //     std::cout << "R: " << HEX32_STRING(x) << std::endl;
    // }

    // std::transform(
    //     r.begin(), r.end(), r.begin(), 
    //     [](uint32_t d) -> uint32_t { return d * 3; });

    // vector<double> r;
    
    std::vector<uint32_t> r2;
    r2.resize(r.size());

    std::map<uint32_t, uint32_t> mymap;

    // int outside = 0;

    // myset.find(x) != myset.end()
    std::set<int> myset;

    std::transform(r.begin(), r.end(), r2.begin(), [&mymap](uint32_t v)
    {
        auto found = mymap.find(v);
        if( found == mymap.end() ) {
            mymap.insert(std::make_pair(v,1));
        } else {
            found->second++;
        }
        // if(outside < 5) {
        //     outside++;
        // }
        // return v*3.0 + outside;
        return 0;
    });

    // auto foo = ;

    decltype(mymap)::iterator maxit;

    uint32_t max_value = 0;

    for (auto it = mymap.begin(); it != mymap.end(); ++it){
        if( print3 ) {
            std::cout << "Votes: " << it->second << "    Value: " << HEX32_STRING(it->first) << std::endl;
        }

        if( it->second > max_value ) {
            maxit = it;
            max_value = it->second;
        }
        // this->handleDataNegIndex(it->second);
    }

    if( print3 ) {
        std::cout << "max votes " << maxit->second << std::endl;
        std::cout << "max value " << HEX32_STRING(maxit->first) << std::endl;
    }

    const unsigned vote_tol = enabled_subcarriers/2;
    // if more than half of the subcarriers agree
    if( maxit->second > vote_tol ) {

        uint32_t word = maxit->first;

        auto unp1 = AirPacket::unpackHeader(word);


        // uint32_t length = std::get<0>(unp1);
        // uint8_t seq = std::get<1>(unp1);
        // uint8_t flags = std::get<2>(unp1);

        return std::make_tuple(true, unp1);
    } else {
        std::cout << "No clear winner of votes, maximum was  " << maxit->second << " out of required " << vote_tol << std::endl;

        for (auto it = mymap.begin(); it != mymap.end(); ++it){
            std::cout << "Votes: " << it->second << "    Value: " << HEX32_STRING(it->first) << std::endl;
        }

        return ret_invalid;
    }


    // int foo = 4;

    // std::transform(r.begin(), r.end(), r2.begin(), [foo](uint32_t v)
    // {
    //     return v*4.0 + foo;
    // });


    // for( auto x : r2 ) {
    //     std::cout << "R2: " << HEX32_STRING(x) << std::endl;
    // }



    return ret_invalid;


}

void AirPacket::setSyncMessage(void) {

    // hand converted from deadbeef
    // std::vector<uint32_t> sync_message;

    // if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
        // sync_message = {0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xffffffff};
    // } else {


    // if( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
    //     // sync_message = {0xaaaaaaaa,0x00000000,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0x00000000,0x00000000,0xaaaaaaaa,0xffffffff,0xffffffff,0xffffffff,0x00000000,0xffffffff,0xffffffff,0xffffffff,0x55555555};
    //     sync_message = {0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa};
    // } else if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
    //     sync_message = {0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa};
    // } else {
        // default is copied from qam 16 / 360
        sync_message = {0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa,0xffffffff,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xffffffff,0xffffffff,0xaaaaaaaa};
    // }

}

static uint32_t extract_vertical_from_sliced2(const std::vector<uint32_t> &slc, const int j, const int i, const unsigned enabled_subcarriers) {
    uint32_t demod_word = 0;

    for(int k = i; k < i+16; k++)
    {
        uint32_t demod_bits = get_bit_pair_sliced<false>(slc, j, k, enabled_subcarriers);
        demod_word = (demod_word<<2 | demod_bits);
    }

    return demod_word;
}


#include <fstream>

void AirPacketInbound::demodulateSlicedHeader(
    const sliced_buffer_t &as_iq,
    bool &dout_valid,
    uint32_t &found_length,
    uint32_t &found_frame,
    uint32_t &found_array_index,  // which exact array index did we find
    air_header_t &header,
    unsigned &erase) {
    if (duplex_mode) {
        if( print21 ) {
                std::cout << "Going to execute demodulateSlicedHeaderDuplex\n";
        }
        demodulateSlicedHeaderDuplex(as_iq,
                                     dout_valid,
                                     found_length,
                                     found_frame,
                                     found_array_index,
                                     header,
                                     erase);
        return;
    }
    // defaults
    erase = 0;
    dout_valid = false;


    constexpr unsigned tol = 45;

    TransformWords128 transform;
    TransformWords128Qam16 transformQam16;
    TransformWords128Qam16Sc320 transformQam16Sc320;

    if(dump1) {

        std::ofstream myfile;
        std::string fname = "dheader_";
        fname += std::to_string(dump1_count);
        fname += ".txt";

        myfile.open (fname);

        myfile << "std::vector<uint32_t> a = {\n";
        for( auto w : as_iq[0] ) {
            myfile << "0x" << HEX32_STRING(w) << ",";
        }
        myfile << "}\n";

        myfile << "std::vector<uint32_t> b = {\n";
        
        for( auto w : as_iq[1] ) {
            myfile << "0x" << HEX32_STRING(w) << ",";
        }
        myfile << "}\n";

        myfile.close();

        dump1_count++;
        // exit(0);
    }




    unsigned local_state = 0;

    unsigned read_max = as_iq[1].size();

    // std::cout << "read_max " << read_max << "\n";
    // exit(0);

    const auto effective_sliced_word_count = getEffectiveSlicedWordCount();

    unsigned early = effective_sliced_word_count*16*2;

    if( read_max > early ) {
        read_max -= early; // back off by 8, because loop below reads 8 past i
    } else {
        read_max = 0; // weird error, just stop
    }

    // std::cout << "Effective Sliced word count: " << effective_sliced_word_count << "\n";
    // std::cout << "demodulateSlicedHeader read_max " << read_max << " as_iq[1].size(); " << as_iq[1].size() << std::endl;
    // bump by 8
    for(unsigned i = 0; i < read_max; i+=effective_sliced_word_count) {
        const uint32_t _sync_word = sync_message[local_state];

        std::tuple<unsigned, unsigned, uint32_t> packed;
        unsigned count_set;
        unsigned lookup_i;
        uint32_t a_word;

        if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
            if( subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
                packed = _demodulateSlicedHeaderHelper<TransformWords128Qam16>(as_iq, transformQam16, effective_sliced_word_count, i, _sync_word, modulation_schema);
            } else {
                // std::cout << "here effective_sliced_word_count " << effective_sliced_word_count << "\n";
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, _sync_word, modulation_schema, print14);
            }
        } else {
            if( subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, _sync_word, modulation_schema, print14);
            } else if ( subcarrier_allocation == MAPMOV_SUBCARRIER_640_LIN || subcarrier_allocation == MAPMOV_SUBCARRIER_512_LIN ) {
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, _sync_word, modulation_schema, print14);
            } else {
                packed = _demodulateSlicedHeaderHelper<TransformWords128>(as_iq, transform, effective_sliced_word_count, i, _sync_word, modulation_schema);
            }
        }

        count_set = std::get<0>(packed);
        lookup_i = std::get<1>(packed);
        a_word = std::get<2>(packed);

        (void) lookup_i;
        (void) a_word;

        if( print8 ) {
            std::cout << "Count set " << count_set << " at " << i << " st " << local_state << " max " << read_max << " " << as_iq[0][lookup_i] << std::endl;
        }


        // for 8 words, that makes 256 bits
        // out of 256 bits, if less than 16 were wrong (10 %)
        if( count_set < tol ) {
            if( print1 ) {
                std::cout << "Count set " << count_set << " at " << i << " st " << local_state << " max " << read_max << " " << as_iq[0][lookup_i] << std::endl;
            }

            if( local_state < 15 ) {
                local_state++;
            } else if( local_state == 15 ) {
                // subtract this value back out
                const int back = 15 * effective_sliced_word_count;
                // match
                // std::cout << "!!!!!! Match at " << i << " " << as_iq[0][i-back] << ", " << as_iq[0].size() << std::endl;







                int index_start;
                // if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
                    // index_start = ((i/effective_sliced_word_count)-(effective_sliced_word_count-1))+effective_sliced_word_count;
                // } else {
                    index_start = ((i/effective_sliced_word_count)+1);
                // }
                int index_end = index_start+16;

                // const int index_start2 = index_start * effective_sliced_word_count;

                if(print12 > 0) {
                    for(unsigned j = 0; j < print12; j++) {
                        std::cout << HEX32_STRING(as_iq[1][index_start+j]) << "\n";
                    }
                }

                if( print13 ) {
                    std::cout << "i: " << i << std::endl;
                    std::cout << "index_start " << index_start << std::endl;
                    std::cout << "index_end " << index_end << std::endl;
                }


                unsigned extract_vertical_argument;

                // if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
                //     extract_vertical_argument = enabled_subcarriers * 2;
                // }

                if( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
                    // std::cout << "here 3\n";
                    extract_vertical_argument = sliced_word_count*32.0/bits_per_subcarrier;
                } else if( modulation_schema == FEEDBACK_MAPMOV_QAM16 && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
                    extract_vertical_argument = enabled_subcarriers * 2;
                } else if( modulation_schema == FEEDBACK_MAPMOV_QPSK ) {
                    // this is this for qpsk 320:
                    // 16*20;

                    extract_vertical_argument = sliced_word_count*32.0/bits_per_subcarrier;
                } else if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
                    // std::cout << "here 4\n";
                    // extract_vertical_argument = sliced_word_count*32.0/(bits_per_subcarrier);
                    extract_vertical_argument = enabled_subcarriers * 2;
                } else {
                    extract_vertical_argument = sliced_word_count*32.0/bits_per_subcarrier;
                }

                if( print10 ) {
                    std::cout << "extract_vertical_argument " << extract_vertical_argument << "\n";
                }


                std::vector<uint32_t> length_words;
                // length_words.resize(16);

                for(int k = index_start; k < index_end; k+=16) {
                    // std::cout << "k: " << k << std::endl;
                    for(unsigned j = 0; j < enabled_subcarriers; j++) {
                        uint32_t w;
                        if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
                            w = extract_vertical_from_sliced_qam16(as_iq[1], j, k, extract_vertical_argument);
                        } else {
                            w = extract_vertical_from_sliced(as_iq[1], j, k, extract_vertical_argument);
                        }
                        if( print2 ) {
                            std::cout << "w: " << HEX32_STRING(w) << std::endl;
                        }
                        length_words.emplace_back(w);
                    }
                }
                // exit(0);

                // std::cout << "length_words " << length_words.size() << std::endl;

                auto header_packed = parseLengthHeader(length_words);
                bool header_valid = std::get<0>(header_packed);
                const air_header_t got_header = std::get<1>(header_packed);

                const uint32_t packet_length = std::get<0>(got_header);
                uint8_t header_seq = std::get<1>(got_header);
                uint8_t header_flags = std::get<2>(got_header);

                if( print10 ) {
                    std::cout << "packet length: " << packet_length << " seq: " << (int) header_seq << " flags: " <<  HEX_STRING((int)header_flags) << "\n";
                }

                if( !header_valid ) { 
                    std::cout << "Invalid header, ERASING!!!" << std::endl;
                    erase = i-back;

                    if( (erase + (32*effective_sliced_word_count)) < as_iq[0].size() ) {
                        erase += (32*effective_sliced_word_count);
                    }
                    return;
                }

                int index_of_start = i-back;

                const int must_have = index_of_start + (32*effective_sliced_word_count) + packet_length + (20 - (packet_length % 20));
                // std::cout << "must_have: " << must_have << " " << (signed)as_iq[0].size() << "\n";

                // if the buffer has more words than we require, we have enough
                // to demod the entire packet
                if( must_have < (signed)as_iq[0].size() ) {
                    erase = must_have;
                    found_length = packet_length;
                    found_frame = as_iq[0][i-back];
                    found_array_index = i-back;
                    header = got_header;
                    dout_valid = true;
                    std::cout << "\n";
                    /*std::cout << "index_of_start: " << index_of_start << "\n"
                              << "effective_sliced_word_count: " << effective_sliced_word_count << "\n"
                              << "32*effective_sliced_word_count: " << 32*effective_sliced_word_count << "\n"
                              << "packet_length: " << packet_length << "\n"
                              << "must_have: " << must_have << "\n"
                              << "erase: " << erase << "\n";*/
                }


                // for(unsigned m = index_start; m < index_end; m++) {
                //     for(unsigned k = 0; k < enabled_subcarriers; k++) {
                //         auto w = extract_vertical_from_sliced(as_iq[1], k, m, enabled_subcarriers);
                //         std::cout << "a VOTE1 WAS " << w << std::endl;
                //     }
                // }

                


                if( print10 ) {
                    std::cout << "found_frame " << found_frame << "\n";
                    std::cout << "found_array_index " << found_array_index << "\n";
                }

                return;
            }

        } else {

            // if we were already matching, but we got a failure
            // we need to re-wind the number of matches minus 1
            if( local_state > 0 ) {
                i -= local_state*effective_sliced_word_count;
            }

            local_state = 0;
            // std::cout << "resetting" << std::endl;
        }
    }


    // we didn't find any header
    // we need to leave at least 64 * sliced_word_count
    // in the buffer
    const unsigned leave_in = 64;

    if( as_iq[1].size() > (effective_sliced_word_count*leave_in) ) {
        erase = as_iq[1].size() - (effective_sliced_word_count*leave_in);
    } else {
        // we dont' even have 64 frames in the buffer, erase nothing
        erase = 0;
    }

    // std::cout << "\nerase AirPacket: " << erase << "\n"
    //           << "as_iq[1].size(): " << as_iq[1].size() << "\n"
    //           << "effective_sliced_word_count*32: " << effective_sliced_word_count*64 << "\n";
}

// fixme refactor this and use in demodulate sliced
std::vector<uint32_t> AirPacketInbound::bitMangleHeader(const std::vector<uint32_t>& in) const {
    std::vector<uint32_t> out;


    if ( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {

        TransformWords128QpskSc320 transformQpskSc320;
        for(unsigned i = 0; i < sliced_word_count; i++) {

            const auto optional_result = transformQpskSc320.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(in[lookup_i]);
            out.push_back(w2);
        }

        return out;

    } else if (modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_64_LIN) {
        TransformWords128QpskSc64 transformQpskSc64;
        for(unsigned i = 0; i < sliced_word_count; i++) {

            const auto optional_result = transformQpskSc64.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(in[lookup_i]);
            out.push_back(w2);
        }

        return out;
    }

    return {};

}



void AirPacketInbound::demodulateSlicedHeaderDuplex(const sliced_buffer_t &sliced_words,
                                                    bool &dout_valid,
                                                    uint32_t &found_length,
                                                    uint32_t &found_frame,
                                                    uint32_t &found_array_index,
                                                    air_header_t &header,
                                                    unsigned &erase) {
    std::vector<uint32_t> header_array;
    std::pair<bool, uint64_t> valid_header;
    MacHeader mac_header;
    bool valid_checksum = false;
    const auto effective_sliced_word_count = getEffectiveSlicedWordCount();
    erase = 0;
    for (std::size_t i = 0; i < sliced_words[0].size(); i++) {
        if ((sliced_words[0][i] % duplex_frames) == duplex_start_frame) {
            header_array.assign(sliced_words[1].begin() + i,
                                sliced_words[1].begin() + i + sliced_word_count);

            if( print25 ) {
                for(const auto w : header_array) {
                    std::cout << HEX32_STRING(w) << "\n";
                }
            }

            header_array = bitMangleHeader(header_array);

            if( print26 ) {
                for(const auto w : header_array) {
                    std::cout << HEX32_STRING(w) << "\n";
                }
            }

            valid_header = mac_header.vote_header(header_array);

            if (valid_header.first) {
                valid_checksum = mac_header.valid_checksum(valid_header.second);
                if (valid_checksum) {
                    dout_valid = true;
                    if( print19 ) {
                        std::cout << "Found a valid header with valid checksum\n";
                    }
                    uint32_t upper_header = valid_header.second >> 32;
                    uint32_t lower_header = valid_header.second;
                    uint32_t length;
                    uint16_t seq_num;
                    uint8_t from;
                    uint8_t flags;
                    mac_header.unpack_header(upper_header,
                                             lower_header,
                                             length,
                                             seq_num,
                                             from,
                                             flags);
                    found_length = length;
                    found_frame = (sliced_words[0][i] % duplex_frames) + 1;
                    found_array_index = i + effective_sliced_word_count - 1;
                 //   std::cout << "i: " << i << "\n"
                 //             << "effective_sliced_word_count: " << effective_sliced_word_count << "\n"
                 //             << "length: " << length << "\n";
                    mac_header.convert_air_header(found_length,
                                                  seq_num,
                                                  flags,
                                                  header);
                    return;
                }
            }
        }
    }
    // Delete everything if no valid header is detected
    // if( print19 ) {
    //     std::cout << "I am deleting everything\n";
    // }
    erase = sliced_words[0].size();
}

// this version is called by s-modem
void AirPacketInbound::demodulateSliced(
    const sliced_buffer_t &as_iq,
    std::vector<uint32_t> &dout,
    bool &dout_valid,
    uint32_t &found_frame,
    uint32_t &found_array_index,  // which exact array index did we find
    unsigned &erase,
    const uint32_t force_detect_index,
    const uint32_t force_detect_length) {

    // auto t1 = std::chrono::steady_clock::now();
    // transform = TransformWords128();
    TransformWords128 transform;
    // defaults
    erase = 0;
    dout_valid = false;

    // find minimum subcarrier length

    const auto effective_sliced_word_count = getEffectiveSlicedWordCount();


    const unsigned ofdm_frames2 = std::min(as_iq[0].size(),as_iq[1].size()) / effective_sliced_word_count;
    // we have a function below which looks 16 frames into the future
    // so we limit this variable
    const unsigned ofdm_frames = ofdm_frames2 > 16 ? std::max((ofdm_frames2 - 16u), 0u) : 0;
    // auto t2 = std::chrono::steady_clock::now();
    if( print6 ) {
        std::cout << "demodulateSliced() ofdm_frames " << ofdm_frames << '\n';
    }

    /// 
    /// found sync state
    ///

    const unsigned start_at = force_detect_index;
    const unsigned stop_at = start_at + force_detect_length;

    // std::cout << "start_at: " << start_at << "\n";
    // std::cout << "stop_at: " << stop_at << "\n";
    if(print4 > 0) {
        for(unsigned i = 0; i < print4; i++) {
            std::cout << HEX32_STRING(as_iq[1][start_at+i]) << "\n";
        }
    }


    if( stop_at > (ofdm_frames*effective_sliced_word_count) ) {
        if( print6 ) {
            std::cout << "Not enough data to demodulate sliced data. Exiting\n"
                      << "Stop at: " << stop_at << "\n"
                      << "Needed: " << ofdm_frames*effective_sliced_word_count << "\n"
                      << "Effective Sliced Word Count: " << effective_sliced_word_count << "\n"
                      << "OFDM Frame: " << ofdm_frames << "\n";
        }
        return;
    }

    if( print6 ) {
        std::cout << "ofdm_frames2 " << ofdm_frames2 << "\n";
        std::cout << "ofdm_frames  " << ofdm_frames << "\n";
        std::cout << "force_detect_length " << force_detect_length << "\n";
    }

    // auto t3 = std::chrono::steady_clock::now();
    if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
        for(unsigned i = start_at; i < stop_at; i++) {
            unsigned j = i;//-start_at; // index at zero
            unsigned lookup;

            if( subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
                lookup = ((j/16)*16) + 15-(j%16);
            } else if(subcarrier_allocation == MAPMOV_SUBCARRIER_320) {
                lookup = ((j/40)*40) + 39-(j%40);
            } else {
                const unsigned x = sliced_word_count;
                const unsigned y = sliced_word_count-1;
                lookup = ((j/x)*x) + y-(j%x);
            }

            // lookup += start_at;
            auto w2 = bitMangleSlicedQam16(as_iq[1][lookup]);

            // constexpr bool print = false;
            if( print15 ) {
                std::cout << "j: " 
                << j << " i: " << i 
                << " lookup: " << lookup 
                << " a " << HEX32_STRING(as_iq[1][j])
                << " b " << HEX32_STRING(w2) << '\n';
            }

            dout.push_back(w2);
        }
    } else if ( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
        TransformWords128QpskSc320 transformQpskSc320;
        for(unsigned i = start_at; i < stop_at; i++) {

            const auto optional_result = transformQpskSc320.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(as_iq[1][lookup_i]);
            dout.push_back(w2);
        }
    } else if ( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
        for(unsigned i = start_at; i < stop_at; i++) {

            const auto optional_result = transform.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(as_iq[1][lookup_i]);
            dout.push_back(w2);
        }
    } else {
        TransformWordsGeneric1 transform2(sliced_word_count);
        for(unsigned i = start_at; i < stop_at; i++) {

            const auto optional_result = transform2.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(as_iq[1][lookup_i]);
            dout.push_back(w2);
        }
    }

    if(print7) {
        std::cout << "----Packet:-----\n";
        std::cout << "force_detect_length " << force_detect_length << "\n\n";
        for(const auto w : dout) {
            std::cout << HEX32_STRING(w) << "\n";
        }
    }


    /// Above here is transform schemes etc
    /// Below here is rs and interleaver

    // auto t4 = std::chrono::steady_clock::now();
    if( interleave_length != 0 ) {
        // std::cout << "Begin deinterleaving\n";
        std::vector<uint32_t> deinterleaved;

        interleaver->deinterleave(dout, deinterleaved);

        dout = std::move(deinterleaved);
    }
    auto t5 = std::chrono::steady_clock::now();

    if( code_type == AIRPACKET_CODE_REED_SOLOMON ) {
        if( print23 ) {
                std::cout << "fec_length: " << fec_length << "\n";
                std::cout << "dout size: " << dout.size() << "\n";
        }
        std::vector<uint32_t> decoded_message;
        reed_solomon->decode_message(dout, decoded_message, code_length, fec_length);
        dout = std::move(decoded_message);
    }
    auto t6 = std::chrono::steady_clock::now();
    dout_valid = true;

    // auto t7 = std::chrono::steady_clock::now();
    //std::cout << "Comparing counter complete\n";
    std::chrono::milliseconds ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    
    if( print20 ) {
        std::cout << "Current realtime: " << ms.count() << "\n";
    }

    // erase needs to be a multiple of effective_sliced_word_count
    erase = stop_at + (effective_sliced_word_count - (stop_at % effective_sliced_word_count));
    if (!data_rate.empty()) {
        uint64_t time_diff = ms.count() - data_rate.back().first;
        double bits = dout.size() * 32;
        double instantaneous_data_rate = bits / time_diff * 1000.0 / 1048576.0;
        if( print20 ) {
            std::cout << "\n";
            std::cout << "Time Difference: "
                      << time_diff << " ms\n"
                      << "Packet Size: " << dout.size() << " words\n" 
                      << "Total Bits: " << bits << " bits\n"
                      << "Instantaneous Data Rate: " << instantaneous_data_rate
                      << " Mbps\n";
            std::cout << "\n";
        }
    }

    std::pair<uint64_t, uint32_t> info(ms.count(), dout.size());
    data_rate.push_back(info);
    if (data_rate.back().first - data_rate[0].first >= 1000) {
        double total_words = 0.0;
        double rate;
        for (std::size_t i = 0; i < data_rate.size(); i++) {
            total_words += data_rate[i].second;
        }
        total_words *= 32;
        rate = total_words / (data_rate.back().first - data_rate[0].first) / 1000;
        if( print20 ) {
            std::cout << "Data Rate: " << rate << " Mbps t: "
                      << data_rate.back().first - data_rate[0].first << " ms "
                      << " Bits: " << total_words << "\n";
        }
        data_rate.clear();
        data_rate.push_back(info);
    }
    // auto t8 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> t;
    if( print24 ) {
        // std::cout << "T2 - T1: " << t.count() << " us" << "\n";
        // t = t3 - t2;
        // std::cout << "T3 - T2: " << t.count() << " us" << "\n";
        // t = t4 - t3;
        // std::cout << "T4 - T3: " << t.count() << " us" << "\n";
        // t = t5 - t4;
        // std::cout << "T5 - T4: " << t.count() << " us" << "\n";
        t = t6 - t5;
        std::cout << "T6 - T5: " << t.count() << " us" << "\n";
        // t = t7 - t6;
        // std::cout << "T7 - T6: " << t.count() << " us" << "\n";
        // t = t8 - t7;
        // std::cout << "T8 - T7: " << t.count() << " us" << "\n";
    }
}

/// This file is only for inclusion by the legacy tests
/// This is a NonDuplex function for finding words, and caused us to miss
/// The erase bug by relying on it
void AirPacketInbound::demodulateSlicedSimple(
    const sliced_buffer_t &sliced_words,
    std::vector<uint32_t> &output_data,
    bool &found_something_again,
    uint32_t &found_length,
    uint32_t &found_frame,
    uint32_t &found_array_index,  // which exact array index did we find
    air_header_t &found_header,
    unsigned &do_erase) {

        bool found_something;
        found_something = found_something_again = false;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i

        this->demodulateSlicedHeader(
            sliced_words,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        unsigned _junk_do_erase_sliced = 0;
        if( found_something ) {
            /// replace above demodulate with this
            const uint32_t force_detect_index = this->getForceDetectIndex(found_at_index);
            // std::cout << "force_detect_index " << force_detect_index << " found_at_index " << found_at_index << "\n";
            uint32_t _junk_found_at_frame_again;
            (void)   _junk_found_at_frame_again;
            unsigned _junk_found_at_index_again;
            (void)   _junk_found_at_index_again;
            
            this->demodulateSliced(
                sliced_words,
                output_data,
                found_something_again,
                _junk_found_at_frame_again,
                _junk_found_at_index_again,
                _junk_do_erase_sliced,
                force_detect_index,
                found_length);
        } else {
            if( print9 ) {
                std::cout << "Exiting because nothing found!\n";
            }
        }
}




void FirstTransform::t(const unsigned i, unsigned &i_out, bool &valid) const {
    // unsigned row = i % 128;

    unsigned ret = i*2; // ret is iout

    const unsigned bound = 46;

    const unsigned bound_half = bound/2;

    const unsigned b = i / bound_half;
    // std::cout << "b: " << b << std::endl;
    // std::cout << "ret: " << ret << std::endl;

    ret += b*(128-bound);

    i_out = ret;
    valid = true;
}

std::pair<bool, unsigned> TransformWords128::t(unsigned in) const {

    // how many words were generated from the slicer
    constexpr unsigned enab_words = 8;

    unsigned div = in/enab_words;

    unsigned sub = (2*enab_words)*div+(enab_words-1);

    unsigned ret = sub - in;

    return std::pair<bool, unsigned>(true, ret);
}


std::pair<bool, unsigned> TransformWords128Qam16::t(unsigned in) const {

    // how many words were generated from the slicer
    constexpr unsigned enab_words = 16;

    unsigned div = in/enab_words;

    unsigned sub = (2*enab_words)*div+(enab_words-1);

    unsigned ret = sub - in;

    return std::pair<bool, unsigned>(true, ret);
}

std::pair<bool, unsigned> TransformWords128Qam16Sc320::t(unsigned in) const {

    // how many words were generated from the slicer
    constexpr unsigned enab_words = 40;

    unsigned div = in/enab_words;

    unsigned sub = (2*enab_words)*div+(enab_words-1);

    unsigned ret = sub - in;

    return std::pair<bool, unsigned>(true, ret);
}

std::pair<bool, unsigned> TransformWords128QpskSc320::t(unsigned in) const {

    // how many words were generated from the slicer
    constexpr unsigned enab_words = 20;

    unsigned div = in/enab_words;

    unsigned sub = (2*enab_words)*div+(enab_words-1);

    unsigned ret = sub - in;

    return std::pair<bool, unsigned>(true, ret);
}

std::pair<bool, unsigned> TransformWords128QpskSc64::t(unsigned in) const {

    // how many words were generated from the slicer
    constexpr unsigned enab_words = 4;

    unsigned div = in/enab_words;

    unsigned sub = (2*enab_words)*div+(enab_words-1);

    unsigned ret = sub - in;

    return std::pair<bool, unsigned>(true, ret);
}

TransformWordsGeneric1::TransformWordsGeneric1(unsigned x) : sliced_words(x) {}

std::pair<bool, unsigned> TransformWordsGeneric1::t(unsigned in) const {

    unsigned div = in/sliced_words;

    unsigned sub = (2*sliced_words)*div+(sliced_words-1);

    unsigned ret = sub - in;

    // std::cout << "in: " << in << " out: " << ret << "\n";

    return std::pair<bool, unsigned>(true, ret);
}




void AirPacketTransformSingle::build_cache(const unsigned max) {

    bool valid;
    unsigned val;
    for (unsigned i = 0; i < max; i++) {
        // int val = 4;

        // unsigned out;
        this->t(i, val, valid);
        if(valid) {

            auto p = std::make_pair(val, i);
            cache_reverse.insert(p);
        }

    }

    cache_valid = true;
    // this->outs.insert(std::make_pair("ringbusout", ringbusout));
}

void AirPacketTransformSingle::r(const unsigned i, unsigned &i_out, bool &valid) const {
    auto it = cache_reverse.find(i);
    if (it != cache_reverse.end()) {
        valid = true;
        i_out = it->second;
        // mymap.erase (it);
    } else {
        valid = false;
    }
    // cache_reverse.
}

// no endl at the end, just to mess with you
std::string AirPacket::getMapMovVectorInfo(const size_t dsize) const {
    std::stringstream ss;

    ss << "About 2 send " << dsize << " words " << std::endl;
    ss << "or " << dsize*32 << " bits" << std::endl;
    ss << "or " << dsize*32.0/bits_per_subcarrier << " subcarriers worth " << std::endl;
    ss << "or " << (dsize*32.0/bits_per_subcarrier)/((double)enabled_subcarriers) << " frames worth ";

    return ss.str();
}

std::vector<double> AirPacket::getSizeInfo(const size_t dsize) const {
    const double bits                = dsize*32;
    const double subcarriers_worth   = dsize*32.0/bits_per_subcarrier;
    const double frames              = (dsize*32.0/bits_per_subcarrier)/((double)enabled_subcarriers);
    return {bits, subcarriers_worth, frames};
}




uint32_t AirPacket::packHeader(const uint32_t length, const uint8_t seq, const uint8_t flags) {
    air_header_t p = std::make_tuple(length, seq, flags);
    return packHeader(p);
}
uint32_t AirPacket::packHeader(const air_header_t& h) {

    uint32_t length = std::get<0>(h);
    uint8_t seq = std::get<1>(h);
    uint8_t flags = std::get<2>(h);

    assert(length <= 0xfffff);
    assert(flags <= 0xf);

    
    uint32_t word = 0;

    word |= length & 0xfffff;
    word |= ((uint32_t)seq&0xff) << 20;
    word |= ((uint32_t)flags&0xf) << 28;

    return word;
}
air_header_t AirPacket::unpackHeader(const uint32_t w) {

    uint32_t length = w & 0xfffff;
    uint8_t seq = (w & 0xff00000) >> 20;
    uint8_t flags = (w & 0xf0000000) >> 28;

    air_header_t ret = std::make_tuple(length,seq,flags);
    return ret;
}

uint8_t AirPacketOutbound::getTxSeq(const bool inc) {
    if( inc ) {
        uint8_t copy = tx_seq;
        tx_seq = (tx_seq+1) & 0xff;
        return copy;
    } else {
        return tx_seq;
    }
}

uint8_t AirPacketOutbound::getPrevTxSeq() const {
    return (tx_seq-1) & 0xff;
}

void AirPacketOutbound::resetTxSeq() {
    tx_seq = 0;
}

void AirPacketOutbound::padData(std::vector<uint32_t>& d) const {
    if( this->modulation_schema == FEEDBACK_MAPMOV_QPSK &&
        this->subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
        double padFactor = 32/2;
        
        while( ((unsigned)(d.size()*padFactor) % 128) != 0 ) {
            d.push_back(0);
        }
        return;
    }

    if( this->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
        this->subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {

        double padFactor = 32/4;
        while( ((unsigned)(d.size()*padFactor) % 128) != 0 ) {
            d.push_back(0);
        }
        return;
    }

    // if( this->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
    //     this->subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {

    while( ((unsigned)(d.size()) % this->sliced_word_count) != 0 ) {
        d.push_back(0);
    }
    if( print22 ) {
        std::cout << "After padData my size is: " << d.size() << "\n";
    }
    if( print27 ) {
        for(const auto w : d) {
            std::cout << HEX32_STRING(w) << "\n";
        }
    }


    // }
}

std::vector<uint32_t> AirPacket::vectorJoin(const std::vector<std::vector<uint32_t>>& in) {
    std::vector<uint32_t> ret;
    unsigned i = 0;
    for(const auto& v : in ) {
        // if( v.size() == 0 ) {
            // std::cout << "Warning vectorJoin() called where " << i << "th vector was empty.\n";
            // i++;
            // continue;
        // }
        ret.push_back((uint32_t)v.size());
        for(const auto w: v) {
            ret.push_back(w);
        }
        i++;
    }
    return ret;
}

std::vector<std::vector<uint32_t>> AirPacket::vectorSplit(const std::vector<uint32_t>& in) {
    std::vector<std::vector<uint32_t>> ret;
    std::vector<uint32_t> partial;

    if( in.size() == 0) {
        return ret;
    }

    bool first = true;
    uint32_t working = 0;
    for(const auto w : in) {
        if( first ) {
            working = w;
            first = false;
            continue;
        }
        if( working == 0 ) {
            ret.emplace_back(partial);
            partial.resize(0);
            working = w;
        } else {
            partial.emplace_back(w);
            working--;
        }
    }

    ret.push_back(partial);

    return ret;
}

std::vector<uint32_t> AirPacket::packJointZmq(const uint32_t from_peer, const uint32_t epoc, const uint32_t ts, const uint8_t seq, const std::vector<std::vector<uint32_t>>& in) {
    std::vector<uint32_t> ret;
    ret.emplace_back(from_peer);
    ret.emplace_back(epoc);
    ret.emplace_back(ts);
    ret.emplace_back(seq);

    std::vector<uint32_t> joined = vectorJoin(in);
    for(const auto w : joined) {
        ret.emplace_back(w);
    }

    return ret;
}

void AirPacket::unpackJointZmq(const std::vector<uint32_t>& in, uint32_t& from_peer, uint32_t& epoc, uint32_t& ts, uint8_t& seq, std::vector<std::vector<uint32_t>>& out) {
    unpackJointZmqHeader(in, from_peer, epoc, ts, seq);

    std::vector<uint32_t> wasteful;
    for(unsigned i = 4; i < in.size(); i++) {
        wasteful.emplace_back(in[i]);
    }

    out = vectorSplit(wasteful);
}

void AirPacket::unpackJointZmqHeader(const std::vector<uint32_t>& in, uint32_t& from_peer, uint32_t& epoc, uint32_t& ts, uint8_t& seq) {
    from_peer = in[0];
    epoc = in[1];
    ts = in[2];
    seq = in[3];
}
