#include <boost/program_options.hpp>
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

#define ENABLE_RAW_KEYBOARD



std::string g_init;
std::string g_patch;

int main_stream_ints(void)
{
  Device D;


  cout << "Modem Boot" << endl;
#ifdef ENABLE_RAW_KEYBOARD
  enable_raw_keyboard_mode();
#endif


  SoapySDR::Kwargs kwfind;
  kwfind["init_path"] = g_init;
  kwfind["patch_path"] = g_patch;

  //enumerate devices
  SoapySDR::KwargsList results = findHiggs(kwfind);//D.enumerate(kwfind);

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

    // for (size_t j = 0; j < results[i].size(); j++)
    // {
    //     printf("%s=%s, ", results[i].keys[j], results[i].vals[j]);
    // }
    // printf("\n");
  }
  // SoapySDRKwargsList_clear(results, length);

  // std::string argStr = "driver=HiggsDriver";


  // std::cout << "Make device " << argStr << std::endl;
  try
  {
    auto device = SoapySDR::Device::make(kwfind);
    std::cout << "  driver=" << device->getDriverKey() << std::endl;
    std::cout << "  hardware=" << device->getHardwareKey() << std::endl;
    for (const auto &it : device->getHardwareInfo())
    {
        std::cout << "  " << it.first << "=" << it.second << std::endl;
    }

    std::vector<size_t> channels = {0,1};

    // Note that these settings are all string type
    SoapySDR::Kwargs kwsettings;

    kwsettings["init_path"] = g_init;
    kwsettings["patch_path"] = g_patch;

    auto rxStream = device->setupStream(
      SOAPY_SDR_RX,
      SOAPY_SDR_CS16,
      channels,
      kwsettings
      );

    // HiggsDriver* hack = (HiggsDriver*)hack;

    // hack->debugEthDmaOut(0);

    device->activateStream(
      rxStream,
      0,
      0,
      0
      );

    uint32_t buff[1024];

    bool pass = true;

#ifdef COUNTER_STREAM
    uint32_t debug_check = 0;
    for (size_t i = 0; i < 10; i++)
    {
      void *buffs[] = {buff}; //array of buffers
      int flags; //flags set by receive operation
      long long timeNs; //timestamp for receive buffer
      // int ret = SoapySDRDevice_readStream(sdr, rxStream, buffs, 1024, &flags, &timeNs, 100000);
      int ret = device->readStream(
        rxStream, 
        buffs, 
        1024, 
        flags, 
        timeNs, 
        100000
        );
      printf("ret=%d, flags=%d, timeNs=%lld\n", ret, flags, timeNs);
      for (auto j = 0; j < 1024; j++) {
        // cout << buff[j] << endl;
        if( buff[j] != debug_check) {
          pass = false;
          cout << "FAILED AT " << j << " expected " << debug_check << " got " << buff[j] << endl;
          break;
        }
        debug_check++;
      }
    }
#endif

    // device->debugEthDmaOut(0);

    while(1) {
        void *buffs[] = {buff}; //array of buffers
        int flags; //flags set by receive operation
        long long timeNs; //timestamp for receive buffer

        int ret = device->readStream( rxStream, buffs, 1024, flags, timeNs, 100000 );
        (void)ret;

        // if( ret > 0 ) {
        //     cout << "got " << ret << endl;
        //     for(auto j = 0; j < ret; j++) {
        //         cout << HEX32_STRING(buff[j]) << ",";
        //     }
        //     cout << endl;
        // }

        
        if(kbhit()) {
            char c = getch();
            cout << "you pressed: " << c << endl;
            if (c == 'x' || c == 'c') {
                break; // EXIT THE WHILE
            }
        }

        usleep(10);

        //FIXME: remove
        // break;

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
#ifdef ENABLE_RAW_KEYBOARD
        disable_raw_keyboard_mode();
        flush_raw_keyboard_mode();
#endif
        return EXIT_FAILURE;
  }
  std::cout << std::endl;
#ifdef ENABLE_RAW_KEYBOARD
    disable_raw_keyboard_mode();
    flush_raw_keyboard_mode();
#endif

  cout << endl;
  return 0;
}

using namespace boost::program_options;

// void on_age(int age)
// {
//   std::cout << "On age: " << age << '\n';
// }


bool handle_args(int argc, const char *argv[]) {
  try
  {
    options_description desc{"Options"};
    desc.add_options()
      ("help,h", "Help screen")
      ("config", value<std::string>()->default_value(DEFAULT_JSON_PATH), "Path to init.json")
      ("patch", value<std::string>()->default_value(""), "Path to a patch which will be applied ontop");
      // ("age", value<int>()->notifier(on_age), "Age");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
        std::cout << desc << '\n';
        return true;
    } else {
        if(vm.count("config")) {
            std::cout << "config: " << vm["config"].as<std::string>() << '\n';
            g_init = vm["config"].as<std::string>();

        }
        if(vm.count("patch")) {
            std::cout << "patch: " << vm["patch"].as<std::string>() << '\n';
            g_patch = vm["patch"].as<std::string>();
        }
    }
  }
  catch (const error &ex)
  {
    std::cerr << ex.what() << '\n';
    return true;
  }
  return false;
}


int main(int argc, const char *argv[]) {
    bool error = handle_args(argc, argv);
    if( !error ) {
        main_stream_ints();
    }
}