

void setFrontendMapping(const int direction, const std::string &mapping);
std::string getFrontendMapping(const int direction) const;
size_t getNumChannels(const int direction) const;
SoapySDR::Kwargs getChannelInfo(const int direction, const size_t channel) const;
bool getFullDuplex(const int direction, const size_t channel) const;
size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream);
int getDirectAccessBufferAddrs(SoapySDR::Stream *stream, const size_t handle, void **buffs);
void setDCOffsetMode(const int direction, const size_t channel, const bool automatic);
bool getDCOffsetMode(const int direction, const size_t channel) const;
bool hasDCOffset(const int direction, const size_t channel) const;
void setDCOffset(const int direction, const size_t channel, const std::complex<double> &offset);
std::complex<double> getDCOffset(const int direction, const size_t channel) const;
bool hasIQBalance(const int direction, const size_t channel) const;
void setIQBalance(const int direction, const size_t channel, const std::complex<double> &balance);
std::complex<double> getIQBalance(const int direction, const size_t channel) const;
bool hasFrequencyCorrection(const int direction, const size_t channel) const;
void setFrequencyCorrection(const int direction, const size_t channel, const double value);
double getFrequencyCorrection(const int direction, const size_t channel) const;
std::vector<std::string> listGains(const int direction, const size_t channel) const;
bool hasGainMode(const int direction, const size_t channel) const;
void setGainMode(const int direction, const size_t channel, const bool automatic);
bool getGainMode(const int direction, const size_t channel) const;
void setGain(const int direction, const size_t channel, const double value);
void setGain(const int direction, const size_t channel, const std::string &name, const double value);
double getGain(const int direction, const size_t channel) const;
double getGain(const int direction, const size_t channel, const std::string &name) const;
SoapySDR::Range getGainRange(const int direction, const size_t channel) const;
SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;
void setFrequency(const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
void setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
double getFrequency(const int direction, const size_t channel) const;
double getFrequency(const int direction, const size_t channel, const std::string &name) const;
std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;
SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel) const;
SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;
SoapySDR::ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const;
void setSampleRate(const int direction, const size_t channel, const double rate);
double getSampleRate(const int direction, const size_t channel) const;
std::vector<double> listSampleRates(const int direction, const size_t channel) const;
SoapySDR::RangeList getSampleRateRange(const int direction, const size_t channel) const;
void setBandwidth(const int direction, const size_t channel, const double bw);
double getBandwidth(const int direction, const size_t channel) const;
std::vector<double> listBandwidths(const int direction, const size_t channel) const;
SoapySDR::RangeList getBandwidthRange(const int direction, const size_t channel) const;
void setMasterClockRate(const double rate);
double getMasterClockRate(void) const;
SoapySDR::RangeList getMasterClockRates(void) const;
std::vector<std::string> listClockSources(void) const;
void setClockSource(const std::string &source);
std::string getClockSource(void) const;
std::vector<std::string> listTimeSources(void) const;
void setTimeSource(const std::string &source);
std::string getTimeSource(void) const;
bool hasHardwareTime(const std::string &what = "") const;
long long getHardwareTime(const std::string &what = "") const;
void setHardwareTime(const long long timeNs, const std::string &what = "");
void setCommandTime(const long long timeNs, const std::string &what = "");

/// Below moved here after we realized we never call this and don't use Soapy

    /*******************************************************************
     * Stream API
     ******************************************************************/
  
    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;
  
    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
  
    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;

    size_t getStreamMTU(SoapySDR::Stream *stream) const;



    std::vector<std::string> listAntennas( const int direction, const size_t channel ) const;
  
  
    void setAntenna( const int direction, const size_t channel, const std::string &name );
  
  
    std::string getAntenna( const int direction, const size_t channel ) const;
  
  
    bool hasDCOffsetMode( const int direction, const size_t channel ) const;


    SoapySDR::Stream* const TX_STREAM = (SoapySDR::Stream*) 0x1;
    SoapySDR::Stream* const RX_STREAM = (SoapySDR::Stream*) 0x2;
  

    // RTLSDR
    uint32_t sampleRate, centerFrequency;
    int ppm, directSamplingMode;
    // size_t numBuffers, bufferLength, asyncBuffs;
    bool iqSwap, agcMode, offsetMode;
    double IFGain[6], tunerGain;


    std::string getDriverKey( void ) const;
    std::string getHardwareKey( void ) const;
    SoapySDR::Kwargs getHardwareInfo(void) const;


    int readStream(
            SoapySDR::Stream *stream,
            void * const *buffs,
            const size_t numElems,
            int &flags,
            long long &timeNs,
            const long timeoutUs = 100000);

    int readStreamStatus(
            SoapySDR::Stream *stream,
            size_t &chanMask,
            int &flags,
            long long &timeNs,
            const long timeoutUs
    );