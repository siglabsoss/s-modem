#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"


using namespace std;

std::vector<std::string> HiggsDriver::listAntennas( const int direction, const size_t channel ) const
{
  cout << "listAntennas()" << endl;
  (void)direction;
  (void)channel;
  std::vector<std::string> options;
  options.push_back( "A" );
  options.push_back( "B" );
  return(options);
}


void HiggsDriver::setAntenna( const int direction, const size_t channel, const std::string &name )
{
  cout << "setAntenna()" << endl;
  /* TODO delete this function or throw if name != RX... */
  (void)direction;
  (void)channel;
  (void)name;
}


std::string HiggsDriver::getAntenna( const int direction, const size_t channel ) const
{
  cout << "getAntenna()" << endl;
  (void)direction;
  (void)channel;
  return("A");
}


/*******************************************************************
 * Frontend corrections API
 ******************************************************************/


bool HiggsDriver::hasDCOffsetMode( const int direction, const size_t channel ) const
{
  (void)direction;
  (void)channel;
  return(false);
}









std::vector<std::string> HiggsDriver::listGains(const int direction, const size_t channel) const {
    cout << "listGains()" << endl;
    // We should list gain elements
    std::vector<std::string> results;

    // if rx, list both attenuators
    if(direction == SOAPY_SDR_RX) {
        results.push_back("DSA");
    }

    // if tx, list dac current (NO MORE THAN 0x7)

    return results; // FIXME: returning nothing
}

void HiggsDriver::setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const SoapySDR::Kwargs &args) {
    cout << "setFrequency() 1" << endl;
    centerFrequency = (uint32_t) frequency;
    if( centerFrequency != (uint32_t)915E6 ) {
        cout << "Frequency set to value WHICH IS NOT 915Mhz" << endl;
    }
}
double HiggsDriver::getFrequency(const int direction, const size_t channel) const {
    cout << "getFrequency() 0 " << endl;
    return centerFrequency;
}

void HiggsDriver::setSampleRate(const int direction, const size_t channel, const double rate) {
    (void)direction;
    (void)channel;
    cout << "setSampleRate()" << endl;

    sampleRate = rate;
    cout << "Setting sample rate: " << sampleRate << endl;
    // SoapySDR_logf(SOAPY_SDR_DEBUG, "Setting sample rate: %d", sampleRate);

}

