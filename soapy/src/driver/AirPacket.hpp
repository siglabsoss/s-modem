#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <atomic>
#include "mapmov.h"
#include "common/convert.hpp"
#include "driver/MacHeader.hpp"
#include "duplex_schedule.h"
#include "feedback_bus_types.h"


#ifndef _BUFFER_TYPES
#define _BUFFER_TYPES
typedef std::vector<std::vector<uint32_t>> sliced_buffer_t;
typedef std::vector<std::vector<std::pair<uint32_t, uint32_t>>> iq_buffer_t;
#endif

/// These are the fields for air_header_t
///   length
///   seq
///   flags

typedef std::tuple<uint32_t, uint8_t, uint8_t> air_header_t;

class ReedSolomon;
class Interleaver;
class EmulateMapmov;

typedef std::map<uint64_t, ReedSolomon*> rs_map_t;
typedef std::map<uint32_t, Interleaver*> interleave_map_t;




// we need to go from a stream of data, through a converter

// AirPacket has data, but also has other members which give extra features
// there is another object which holds settings, the air packet is put through this settings
// object and the result is data. ready for feedback bus
// on the receive side



// typedef uint32_t (*fetch_subcarrier_bit_t)(unsigned int*, unsigned int*);

// data is transmitted from smodem
// mapmov tx settings transform it
// air transforms it
// reverse mover / cs21 transform it

// virtual void t(unsigned i, unsigned j, unsigned &i_out, unsigned &j_out) const =0;

// for description of virtual rules:
// https://stackoverflow.com/questions/8931612/do-all-virtual-functions-need-to-be-implemented-in-derived-classes

class AirPacketTransformSingle
{
public:
    bool cache_valid = false;
    std::map<unsigned, unsigned> cache_reverse;
    void build_cache(const unsigned max);

    virtual void t(const unsigned i, unsigned &i_out, bool &valid) const {(void)i;(void)i_out;(void)valid;};
    virtual std::pair<bool, unsigned> t(const unsigned i) const {(void)i;return std::pair<bool, unsigned>(false, 0);};
    void r(const unsigned i, unsigned &i_out, bool &valid) const;
};

class FirstTransform : public AirPacketTransformSingle {
public:
    void t(const unsigned i, unsigned &i_out, bool &valid) const;
};

class TransformWords128 : public AirPacketTransformSingle {
public:
    std::pair<bool, unsigned> t(unsigned i) const;
};

class TransformWords128Qam16 : public AirPacketTransformSingle {
public:
    std::pair<bool, unsigned> t(unsigned i) const;
};

class TransformWords128Qam16Sc320 : public AirPacketTransformSingle {
public:
    std::pair<bool, unsigned> t(unsigned i) const;
};

class TransformWords128QpskSc320 : public AirPacketTransformSingle {
public:
    std::pair<bool, unsigned> t(unsigned i) const;
};

class TransformWords128QpskSc64 : public AirPacketTransformSingle {
public:
    std::pair<bool, unsigned> t(unsigned i) const;
};

class TransformWordsGeneric1 : public AirPacketTransformSingle {
public:
    unsigned sliced_words;
    TransformWordsGeneric1(unsigned);
    std::pair<bool, unsigned> t(unsigned i) const;
};



class AirPacket
{
public:
    std::atomic<unsigned> enabled_subcarriers;
    std::atomic<unsigned> sliced_word_count;
    std::atomic<uint32_t> header_overhead_words;
    std::atomic<uint32_t> bits_per_subcarrier;

    // these describe the "mode" which the class is operating in
    std::atomic<uint16_t> subcarrier_allocation;
    std::atomic<uint8_t> modulation_schema;
    std::atomic<uint8_t> code_type;
    std::atomic<uint32_t> code_length;
    std::atomic<uint32_t> fec_length;
    std::atomic<uint8_t> code_puncturing;
    std::atomic<uint32_t> interleave_length;
    std::atomic<bool> duplex_mode;

    ReedSolomon* reed_solomon = 0;
    Interleaver* interleaver = 0;
    EmulateMapmov* emulate_mapmov = 0;

    int get_rs(void) const;

    int get_interleaver(void) const;
    uint32_t getLengthForInput(const uint32_t length);
    
    // These two numbers are related
    // if we only want to send a small packet
    // I believe it's ok 
    // unsigned pad_to_size = 20000; //8*(512*5); // 20480
    int desired_cs20_early_frames = 15000;
    uint32_t duplex_frames = DUPLEX_FRAMES;
    uint32_t duplex_start_frame = DUPLEX_START_FRAME_TX;

    ///
    /// Non-Duplex prints
    ///
    bool print1 = false; // Non-Duplex count set from header
    bool print2 = false; // Non-Duplex header print words
    bool print3 = false; // Non-Duplex header votes
    bool print8 = false; // Non-Duplex cont set
    bool print9 = false; // Non-Duplex test only print (demodulateSlicedSimple)
    bool print10 = false; // non duplex print vertical argument
    bool print11 = false; // Non-Duplex attachHeader() 
    unsigned print12 = 0; // Non-Duplex subtracted for every word, prints full words (header)
    bool print13 = false; // // Non-Duplex print some indices
    bool print14 = false; // Non-Duplex passed to _demodulateSlicedHeaderHelperUntransformed() as print_all

    ///
    /// demodulation, demodulateSliced() prints (shared)
    /// 
    unsigned print4 = 0; // Subtract one and print demodulateSliced before we mangle (meaning it is already mangled by having been received)
    bool print6 = false; // print counters and indices
    bool print15 = false; // print un swizzle data during QAM16 demodulateSliced
    bool print7 = false; // force_detect_length and all words before interleave
    bool print16 = false; // prints the bump in getForceDetectIndex, shared
    bool print20 = true; // print data rate information (shared) (janson)
    bool print21 = true; // Duplex only, demodulateSlicedHeader() prints every time duplex is true
    bool print23 = true; // demodulateSliced, shared, print fec lencth if ReedSolomon is on
    bool print24 = false; // demodulateSliced, print runtimes 
    bool print25 = false; // Duplex only, print words of header, unmangled
    bool print26 = false; // Duplex only, print words of header, mangled

    ///
    /// modulation , transform (the tx one), pad
    ///
    bool print5 = true;  // print when stuffing header
    bool print17 = false; // print interleaver stuff as calling transform (shared)
    bool print18 = false; // Header attaching and info in transform (shared)
    bool print19 = true; // Duplex only, prints print demod_sliced_header detections and deletions
    bool print22 = true; // print final pad data size in tx padData()
    bool print27 = false; // shared, pring all of packer in padData


    bool print_settings_did_change = true;

    // used by rx side to match header in sliced data
    std::vector<uint32_t> sync_message;

    AirPacket();
    // virtual ~AirPacket(); // all c++ destructors must be virtual
    /**
     * This method sets the type of encoder/decoder to use, the code length, and
     * the FEC length in a bit mapped uint32_t number. The first eight least
     * significant bits denote the type of encoder/decoder to use.
     * Bits 07 - 15 and bits 16 - 23 denote the code length and FEC length
     * respectively
     *
     * @param[in] code_value: The type of encoder to use. Set this value to zero
     * to disable encoding/decoding
     * @param[in] code_length: Code length
     * @param[in] fec_length: FEC length
     */
    int set_code_type(const uint8_t code_value,
                      const uint32_t code_length,
                      const uint32_t fec_length);


    int set_rs(const uint32_t fec_length);

    /**
     * Set the amount of bytes to puncture in an encoded code word
     *
     * @param[in] puncturing_value: Amount of bytes to puncture in an encoded
     * code word
     */
    void set_code_puncturing(const uint8_t puncturing_value);

    /**
     * This method set the modulation schema. Currently the supported modulation
     * schemas are QPSK and 16-QAM.
     *
     * @param[in] schema_value: An enum value representing the modulation schema
     */
    void set_modulation_schema(const uint8_t schema_value);

    uint8_t get_modulation_schema(void) const;

    // 
    unsigned get_enabled_subcarriers(void) const;

    void set_interleave(const uint32_t x);

    ///
    /// Read only function to get slice word count (no external setter)
    /// a lot of code existing skips this getter
    unsigned get_sliced_word_count(void) const;

    /**
     * This method set the amount of active subcarriers and the region
     * it will occupy in the frequency spectrum. An enum data structure will be
     * used to represent the amount of subcarriers to set active and the region
     * it will occupy in the frequency spectrum.
     *
     * @param[in] allocation_value: An enum value representing the subcarrier
     * mode. This enum value dictates the amount of active subcarriers and the
     * region it is active.
     */
    void set_subcarrier_allocation(const uint16_t allocation_value);
    uint16_t get_subcarrier_allocation(void) const;

    void set_duplex_start_frame(const uint32_t start_frame);
    uint32_t getEffectiveSlicedWordCount() const;

    void setTestDefaults(void);

    static uint64_t rsKey(uint32_t, uint32_t);
    static std::pair<uint32_t,uint32_t> rsSettings(uint64_t);

private:

    rs_map_t every_reed_solomon;
    interleave_map_t every_interleaver;

    /**
     * This method is called internally after the above settings have changes
     * we update thing that are calculated based on multiple settings 
     * such as sliced_word_count
     */
    void settings_did_change(void);
    void setSyncMessage(void);
public:
    uint32_t getRbForRxSlicer(void);
    uint32_t getForceDetectIndex(const uint32_t fount_at_index) const;
    // void setModulationIndex(const uint32_t);
    void printWide(const std::vector<uint32_t> &d, const unsigned sc) const;
    std::string getMapMovVectorInfo(const size_t dsize) const;
    std::vector<double> getSizeInfo(const size_t dsize) const;
    // void _setModulateIndex(uint32_t);
    sliced_buffer_t emulateHiggsToHiggs(const std::vector<uint32_t> &a, const uint32_t counter_start = 0);

    static uint32_t packHeader(const uint32_t length, const uint8_t seq, const uint8_t flags);
    static uint32_t packHeader(const air_header_t&);
    static air_header_t unpackHeader(const uint32_t);

    static std::vector<uint32_t> packJointZmq(const uint32_t from_peer, const uint32_t epoc, const uint32_t ts, const uint8_t seq, const std::vector<std::vector<uint32_t>>& in);
    static void unpackJointZmq(      const std::vector<uint32_t>& in, uint32_t& from_peer, uint32_t& epoc, uint32_t& ts, uint8_t& seq, std::vector<std::vector<uint32_t>>& out);
    static void unpackJointZmqHeader(const std::vector<uint32_t>& in, uint32_t& from_peer, uint32_t& epoc, uint32_t& ts, uint8_t& seq);
    static std::vector<uint32_t> vectorJoin(const std::vector<std::vector<uint32_t>>& in);
    static std::vector<std::vector<uint32_t>> vectorSplit(const std::vector<uint32_t>& in);
    void set_duplex_mode(bool set_mode);
    bool get_duplex_mode() const;

    void enableDisablePrint(const std::string s, const bool v);
    void enablePrint(const std::string s);
    void disablePrint(const std::string s);

private:
    void format_rx_data(sliced_buffer_t &final_rx_data,
                        const std::vector<uint32_t> &sliced_out_data,
                        const uint32_t counter_start) const;
};


class AirPacketOutbound : public AirPacket
{
public:
    AirPacketOutbound();
    AirPacketTransformSingle* outbound_transform = 0;

    uint32_t pad_till = 0;
    uint8_t tx_seq = 0;

    uint8_t getTxSeq(const bool inc=true);
    uint8_t getPrevTxSeq() const;
    void resetTxSeq();
    
    std::vector<uint32_t> transform(const std::vector<uint32_t> &a, uint8_t& seq_used, const unsigned pad_to = 0);
    // void _setModulateIndex(uint32_t);
    // void setModulationIndex(const uint32_t);
    // std::pair<bool, int> suggestedSleep(const int measurement);
    // std::pair<bool, int> suggestedSleep2(const int measurement);

    // only on tx/outbound for now, can be moved virtual to base class
    std::vector<std::vector<uint32_t>> getGainRingbus(const uint32_t cs10_addr, const uint32_t cs10_gain_rb) const;

    // appends zeros to make sure data fed into mapmov is a multiple of an ofdm frame
    void padData(std::vector<uint32_t>& d) const;

    void attachHeader(const uint32_t data_size, std::vector<uint32_t>& output_vector, const uint8_t seq) const;
    uint8_t attachHeader(const uint32_t data_size, std::vector<uint32_t>& output_vector);
    void add_mac_header(std::vector<uint32_t> &output_vector,
                        uint32_t length,
                        uint16_t seq_num,
                        uint8_t from,
                        uint8_t flags) const;
};


class AirPacketInbound : public AirPacket
{
public:
    uint32_t found_state = 0;
    uint32_t found_at = 0;
    uint32_t saved_found_frame;
    uint32_t demod_bits_for = 16*10;
    // unsigned data_tone_num = 32;
    uint32_t expected_seq = 0;

    const unsigned detect_sync_thresh = 18;
    const uint32_t sync_word = 0xbeefbabe;

    unsigned target_buffer_fill = 64;

    bool dump1 = false;           // non duplex, header dumps a maybe almost formatted data file of current buffer
    unsigned dump1_count = 0;

    std::vector<std::pair<uint64_t, uint32_t>> data_rate;

    // uint32_t sync_detects = 0;
    // unsigned run = as_bits[0].size();


    // void demodulate(const std::vector<std::vector<uint32_t>> &as_bits, std::vector<uint32_t> &dout, bool &dout_valid, unsigned &erase);
    // void demodulate(const iq_buffer_t &as_iq, std::vector<uint32_t> &dout, bool &dout_valid, uint32_t &found_length, uint32_t &found_frame, uint32_t &found_array_index, unsigned &erase);
    void demodulateSliced(const sliced_buffer_t &as_iq, std::vector<uint32_t> &dout, bool &dout_valid, uint32_t &found_frame, uint32_t &found_array_index, unsigned &erase, const uint32_t force_detect_index, const uint32_t force_detect_length);
    void demodulateSlicedHeader(
        const sliced_buffer_t &as_iq,
        bool &dout_valid,
        uint32_t &found_length,
        uint32_t &found_frame,
        uint32_t &found_array_index,  // which exact array index did we find
        air_header_t &header,
        unsigned &erase);
    void demodulateSlicedHeaderDuplex(const sliced_buffer_t &sliced_words,
                                      bool &dout_valid,
                                      uint32_t &found_length,
                                      uint32_t &found_frame,
                                      uint32_t &found_array_index,
                                      air_header_t &header,
                                      unsigned &erase);
    std::vector<uint32_t> bitMangleHeader(const std::vector<uint32_t>& in) const;
    void demodulateSlicedSimple(
        const sliced_buffer_t &as_iq,
        std::vector<uint32_t> &dout,
        bool &dout_valid,
        uint32_t &found_length,
        uint32_t &found_frame,
        uint32_t &found_array_index,  // which exact array index did we find
        air_header_t &header,
        unsigned &erase);

    std::tuple<bool, air_header_t> parseLengthHeader(const std::vector<uint32_t>& r) const;

    // void tt(const std::vector<std::vector<std::pair<uint32_t, uint32_t>>> &(as_iq[32]) );
};




typedef std::function<void(AirPacketInbound* const)> air_set_inbound_t;
typedef std::function<void(AirPacketOutbound* const)> air_set_outbound_t;

