#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <complex.h>
#include <iostream>
#include <stdint.h>

// #include "driver/HiggsSoapyDriver.hpp"
#include "driver/HiggsSoapyDriver.hpp"
#include "common/utilities.hpp"

using namespace std;
using namespace SoapySDR;




int main_stream_floats(void)
{
  Device D;


  cout << "Modem Boot" << endl;

  //enumerate devices
  SoapySDR::KwargsList results = D.enumerate();

  cout << "Enumeration found " << results.size() << " devices" << endl;

  for (size_t i = 0; i < results.size(); i++)
  {
    cout << "Found device #" << i << endl;

    for (auto const& x : results[i])
    {
        std::cout << x.first  // string (key)
                  << ':' 
                  << x.second // string's value 
                  << std::endl ;
    }
  }

  std::string argStr = "driver=HiggsDriver";

  std::cout << "Make device " << argStr << std::endl;
  try
  {
    auto device = SoapySDR::Device::make(argStr);
    std::cout << "  driver=" << device->getDriverKey() << std::endl;
    std::cout << "  hardware=" << device->getHardwareKey() << std::endl;
    for (const auto &it : device->getHardwareInfo())
    {
        std::cout << "  " << it.first << "=" << it.second << std::endl;
    }

    std::vector<size_t> channels = {0,1};

    // only supported
    // SOAPY_SDR_CF32
    // SOAPY_SDR_CS16
    auto rxStream = device->setupStream(
      SOAPY_SDR_RX,
      SOAPY_SDR_CF32,
      channels
      // default 4th arg
      );

    device->activateStream(
      rxStream,
      0,
      0,
      0
      );

    float fbuff[367];

    // uint32_t debug_check = 0;
    bool pass = true;

    while(1) {
        void *buffs[] = {fbuff}; //array of buffers
        int flags; //flags set by receive operation
        long long timeNs; //timestamp for receive buffer

        int ret = device->readStream( rxStream, buffs, 1024, flags, timeNs, 100000 );

        if( ret > 0 ) {
            cout << "got " << ret << " items." << endl;
            for(auto j = 0; j < ret*2; j+=2) {
                cout << fbuff[j] << ",";
            }
            cout << endl;
        }


    }

    device->deactivateStream(rxStream, 0, 0);
    device->closeStream(rxStream);

    cout << endl;
    if( pass ) {
      cout << "           PASS          " << endl;
    } else {
      cout << " !!!!!!!!!!FAIL!!!!!!!!!!" << endl;
    }

    SoapySDR::Device::unmake(device);
  }
  catch (const std::exception &ex)
  {
      std::cerr << "Error making device: " << ex.what() << std::endl;
      return EXIT_FAILURE;
  }
  std::cout << std::endl;



  cout << endl;
}


int main(void) {
    main_stream_floats();
}