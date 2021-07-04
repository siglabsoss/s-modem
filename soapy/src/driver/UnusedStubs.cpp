#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"

using namespace std;


void HiggsDriver::setFrontendMapping(const int direction, const std::string &mapping) {
    cout << "setFrontendMapping()" << endl;
}
std::string HiggsDriver::getFrontendMapping(const int direction) const {
    cout << "getFrontendMapping()" << endl;
    return "";
}
size_t HiggsDriver::getNumChannels(const int direction) const {
    cout << "getNumChannels()" << endl;
    return 0;
}
SoapySDR::Kwargs HiggsDriver::getChannelInfo(const int direction, const size_t channel) const {
    cout << "getChannelInfo()" << endl;
    return {};
}
bool HiggsDriver::getFullDuplex(const int direction, const size_t channel) const {
    cout << "getFullDuplex()" << endl;
    return false;
}
size_t HiggsDriver::getNumDirectAccessBuffers(SoapySDR::Stream *stream) {
    cout << "getNumDirectAccessBuffers()" << endl;
    return 0;
}
int HiggsDriver::getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs) {
    cout << "getDirectAccessBufferAddrs()" << endl;
    return 0;
}
void HiggsDriver::setDCOffsetMode(const int direction, const size_t channel, const bool automatic) {
    cout << "setDCOffsetMode()" << endl;
}
bool HiggsDriver::getDCOffsetMode(const int direction, const size_t channel) const {
    cout << "getDCOffsetMode()" << endl;
    return false;
}
bool HiggsDriver::hasDCOffset(const int direction, const size_t channel) const {
    cout << "hasDCOffset()" << endl;
    return false;
}
void HiggsDriver::setDCOffset(const int direction, const size_t channel, const std::complex<double> &offset) {
    cout << "setDCOffset()" << endl;
}
std::complex<double> HiggsDriver::getDCOffset(const int direction, const size_t channel) const {
    cout << "getDCOffset()" << endl;
    return {0,0};
}
bool HiggsDriver::hasIQBalance(const int direction, const size_t channel) const {
    cout << "hasIQBalance()" << endl;
    return false;
}
void HiggsDriver::setIQBalance(const int direction, const size_t channel, const std::complex<double> &balance) {
    cout << "setIQBalance()" << endl;
}
std::complex<double> HiggsDriver::getIQBalance(const int direction, const size_t channel) const {
    cout << "getIQBalance()" << endl;
    return {0,0};
}
bool HiggsDriver::hasFrequencyCorrection(const int direction, const size_t channel) const {
    cout << "hasFrequencyCorrection()" << endl;
    return false;
}
void HiggsDriver::setFrequencyCorrection(const int direction, const size_t channel, const double value) {
    cout << "setFrequencyCorrection()" << endl;
}
double HiggsDriver::getFrequencyCorrection(const int direction, const size_t channel) const {
    cout << "getFrequencyCorrection()" << endl;
    return 0.0;
}
bool HiggsDriver::hasGainMode(const int direction, const size_t channel) const {
    cout << "hasGainMode()" << endl;
    return false;
}
void HiggsDriver::setGainMode(const int direction, const size_t channel, const bool automatic) {
    cout << "setGainMode()" << endl;
}
bool HiggsDriver::getGainMode(const int direction, const size_t channel) const {
    cout << "getGainMode()" << endl;
    return false;
}
void HiggsDriver::setGain(const int direction, const size_t channel, const double value) {
    cout << "setGain() 0" << endl;
    cout << "Setting gain to " << value << endl;
    cout << "FIXME: Gain not implemented yet" << endl;
}
void HiggsDriver::setGain(const int direction, const size_t channel, const std::string &name, const double value) {
    cout << "setGain() 1" << endl;
}
double HiggsDriver::getGain(const int direction, const size_t channel) const {
    cout << "getGain() 0" << endl;
    return 0.0;
}
double HiggsDriver::getGain(const int direction, const size_t channel, const std::string &name) const {
    cout << "getGain() 1" << endl;
    return 0.0;
}
SoapySDR::Range HiggsDriver::getGainRange(const int direction, const size_t channel) const {
    cout << "getGainRange() 0" << endl;
    return {};
}
SoapySDR::Range HiggsDriver::getGainRange(const int direction, const size_t channel, const std::string &name) const {
    cout << "getGainRange() 1" << endl;
    return {};
}
void HiggsDriver::setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args) {
    cout << "setFrequency() 0" << endl;

}
double HiggsDriver::getFrequency(const int direction, const size_t channel, const std::string &name) const {
    cout << "getFrequency() 1" << endl;
    return 0.0;
}
std::vector<std::string> HiggsDriver::listFrequencies(const int direction, const size_t channel) const {
    cout << "listFrequencies()" << endl;
    return {};
}
SoapySDR::RangeList HiggsDriver::getFrequencyRange(const int direction, const size_t channel) const {
    cout << "getFrequencyRange() 0" << endl;
    return {};
}
SoapySDR::RangeList HiggsDriver::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const {
    cout << "getFrequencyRange() 1" << endl;
    return {};
}
SoapySDR::ArgInfoList HiggsDriver::getFrequencyArgsInfo(const int direction, const size_t channel) const {
    cout << "getFrequencyArgsInfo()" << endl;
    return {};
}
double HiggsDriver::getSampleRate(const int direction, const size_t channel) const {
    cout << "getSampleRate()" << endl;
  return sampleRate;
}
std::vector<double> HiggsDriver::listSampleRates(const int direction, const size_t channel) const {
    cout << "listSampleRates()" << endl;
    return {};
}
SoapySDR::RangeList HiggsDriver::getSampleRateRange(const int direction, const size_t channel) const {
    cout << "getSampleRateRange()" << endl;
    return {};
}
void HiggsDriver::setBandwidth(const int direction, const size_t channel, const double bw) {
    cout << "setBandwidth()" << endl;
}
double HiggsDriver::getBandwidth(const int direction, const size_t channel) const {
    cout << "getBandwidth()" << endl;
    return 0;
}
std::vector<double> HiggsDriver::listBandwidths(const int direction, const size_t channel) const {
    cout << "listBandwidths()" << endl;
    return {};
}
SoapySDR::RangeList HiggsDriver::getBandwidthRange(const int direction, const size_t channel) const {
    cout << "getBandwidthRange()" << endl;
    return {};
}
void HiggsDriver::setMasterClockRate(const double rate) {
    cout << "setMasterClockRate()" << endl;
}
double HiggsDriver::getMasterClockRate(void) const {
    cout << "getMasterClockRate()" << endl;
    return 0;
}
SoapySDR::RangeList HiggsDriver::getMasterClockRates(void) const {
    cout << "getMasterClockRates()" << endl;
    return {};
}
std::vector<std::string> HiggsDriver::listClockSources(void) const {
    cout << "listClockSources()" << endl;
    return {};
}
void HiggsDriver::setClockSource(const std::string &source) {
    cout << "setClockSource()" << endl;
}
std::string HiggsDriver::getClockSource(void) const {
    cout << "getClockSource()" << endl;
    return "";
}
std::vector<std::string> HiggsDriver::listTimeSources(void) const {
    cout << "listTimeSources()" << endl;
    return {};
}
void HiggsDriver::setTimeSource(const std::string &source) {
    cout << "setTimeSource()" << endl;
}
std::string HiggsDriver::getTimeSource(void) const {
    cout << "getTimeSource()" << endl;
    return "";
}
bool HiggsDriver::hasHardwareTime(const std::string &what) const {
    cout << "hasHardwareTime()" << endl;
    return false;
}
long long HiggsDriver::getHardwareTime(const std::string &what) const {
    cout << "getHardwareTime()" << endl;
    return 0;
}
void HiggsDriver::setHardwareTime(const long long timeNs, const std::string &what) {
    cout << "setHardwareTime()" << endl;
}
void HiggsDriver::setCommandTime(const long long timeNs, const std::string &what) {
    cout << "setCommandTime()" << endl;
}

std::vector<std::string> HiggsDriver::getStreamFormats(const int direction, const size_t channel) const
{
  (void)direction;
  (void)channel;

  std::vector<std::string> formats;

  //! Complex signed 16-bit integers (complex int16)
  formats.push_back(SOAPY_SDR_CS16);
  formats.push_back(SOAPY_SDR_CF32);

  return formats;
}

std::string HiggsDriver::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
  (void)direction;
  (void)channel;
  fullScale = 32767;
  return SOAPY_SDR_CS16;
}

SoapySDR::ArgInfoList HiggsDriver::getStreamArgsInfo(const int direction, const size_t channel) const
{
  (void)direction;
  (void)channel;
  SoapySDR::ArgInfoList streamArgs;

  SoapySDR::ArgInfo buffersArg;
  buffersArg.key="buffers";
  buffersArg.value = "0";//std::to_string(BUF_NUM);
  buffersArg.name = "Buffer Count";
  buffersArg.description = "Number of buffers per read.";
  buffersArg.units = "buffers";
  buffersArg.type = SoapySDR::ArgInfo::INT;
  streamArgs.push_back(buffersArg);

  return streamArgs;
}

size_t HiggsDriver::getStreamMTU( SoapySDR::Stream *stream ) const
{
    return PACKET_DATA_MTU_WORDS;
    // if(stream == RX_STREAM){
    //     return _rx_stream.buf_len/BYTES_PER_SAMPLE;
    // } else if(stream == TX_STREAM){
    //     return _tx_stream.buf_len/BYTES_PER_SAMPLE;
    // } else {
    //     throw std::runtime_error("Invalid stream");
    // }
}




std::string HiggsDriver::getDriverKey( void ) const
{
  return("Higgs");
}


std::string HiggsDriver::getHardwareKey( void ) const
{
  return("GravAndCs");
}


SoapySDR::Kwargs HiggsDriver::getHardwareInfo(void) const {
  SoapySDR::Kwargs info;
  return info;
}




// return the number of elements written to output buffer
// we write out entire floats.  This means that one sample is 2 floats
// 4 input bytes goes to 1 word which is actually 2 floats
// we read from the ev buffer in bytes.
// DEFAULT_BUFFER_LENGTH is bytes.  this is also set as ev buffer drain level
// This means we should tell grc to call us with that size
// elements are floats
int HiggsDriver::readStream(
        SoapySDR::Stream *stream,
        void * const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs)
{
    return 0;
}


int HiggsDriver::readStreamStatus(
        SoapySDR::Stream *stream,
        size_t &chanMask,
        int &flags,
        long long &timeNs,
        const long timeoutUs
){
    (void)stream;
    (void)chanMask;
    (void)flags;
    (void)timeNs;
    (void)timeoutUs;
    std::cout << "readStreamStatus()" << std::endl;
    return 0;
}
