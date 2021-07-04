#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "driver/EventDsp.hpp"
// #include "driver/JsonServer.hpp"
#include "HiggsUdpOOO.hpp"
#include "duplex_schedule.h"
#include "AirPacket.hpp"
#include "TxSchedule.hpp"
#include "DemodThread.hpp"

#include <SoapySDR/Registry.hpp>

using namespace std;


HiggsDriver::HiggsDriver(const std::string config_path, const std::string patch_path) :
                        //// threads
                         higgsRunBoostIO             (true)
                        ,higgsRunThread              (true)
                        ,higgsRunOOOThread           (true)
                        ,higgsRunRbPrintThread       (true)
                        ,higgsRunZmqThread           (true)
                        ,higgsRunTunThread           (true)
                        ,higgsRunDemodThread         (true)
                        ,higgsRunDspThread           (true)
                        ,higgsRunFlushHiggsThread    (true)
                        ,higgsRunTxScheduleThread    (true)
                        ,higgsRunHiggsFineSyncThread (true)
                        ,higgsRunEVMThread           (true)
                        //// threads
                        ,higgsAttachedViaEth(true)
                        ,_ooo_memory_p(new HiggsUdpOOO<UDP_RX_BYTES,UDP_OOO_TOLERANCE>)
                        ,rx_sock_fd(0)
                        ,grc_udp_sock_fd(0)
                        ,grc_udp_sock_fd_2(0)
                        ,_settings(config_path, patch_path)
                        ,settings(&_settings)
                        ,status(0)
                        ,rb(0)
                        ,zmq_context(1)
                        ,zmq_sock_pub(zmq_context, ZMQ_PUB)
                        ,zmq_sock_sub(zmq_context, ZMQ_SUB)
                        ,zmq_sock_discovery_push(zmq_context, ZMQ_PUSH)
                        ,zmq_sock_discovery_sub(zmq_context, ZMQ_SUB)
                        ,_this_node_ready(false)
                        ,_flush_higgs_ready(false)
                        ,_zmq_thread_ready(false)
                        ,_boost_thread_ready(false)
                        ,_dsp_thread_ready(false)
                        ,_tx_schedule_ready(false)
                        ,_demod_thread_ready(false)
                        ,dspEvents(NULL)
                        // boost_io_service()
                        // boost_resolver(boost_io_service)
                            {
    const std::string role = GET_RADIO_ROLE();
  
    if( role != "arb" ) {
        createRb();
        rb->injectFlags(GET_RINGBUS_HASH_CHECK_ENABLE(), GET_RINGBUS_HASH_CHECK_PRINT());
    }


    bool forward_to_dashboard = false;
    bool forward_to_smodem = false;


    if( role == "rx" ) {
        forward_to_dashboard = GET_DASHBOARD_DO_CONNECT();
        forward_to_smodem = false;
    }

    if( role == "tx" ) {
        forward_to_dashboard = false;
        forward_to_smodem = true;
    }

    if( role == "arb" ) {
    }

    enableThreadByRole(role);
    specialSettingsByRole(role);

    status = new StatusCode(forward_to_smodem, forward_to_dashboard, this);

    // _should_reset_experiment = false;
    // _should_reset_ber = false;
    // _should_manual_eq = false;
    // _should_manual_zero_eq = false;
    // _should_manual_reset_eq = false;
    // _should_tx_symbol_sync = false;

    accept_remote_feedback_bus_higgs = true;

    if( !GET_GNURADIO_UDP_DO_CONNECT() ) {
        cout << endl << "WARNING: net.gnuradio_udp.do_connect is false, no data will appear in Gnu Radio" << endl;
    }

}
HiggsDriver::~HiggsDriver() {
  cout << "dtons" << endl;
  // jsonServer->StopListening();
  destroyRb();
}


// Soapy uses std::strings for formats, but we need to compare against this our fetch 
// map these to an int/enum value for faster comparisoins

// SOAPY_SDR_CS16
#define HIGGS_SOAPY_SDR_CS16 (0)
// SOAPY_SDR_CF32
#define HIGGS_SOAPY_SDR_CF32 (1)


/***********************************************************************
 * Find available devices
 **********************************************************************/
SoapySDR::KwargsList findHiggs(const SoapySDR::Kwargs &args)
{
  const auto init_path = args.at("init_path");
  const auto patch_path = args.at("patch_path");

  cout << "Looking for init: " << init_path << endl;
  cout << "Looking for patch: " << patch_path << endl;

  std::vector<SoapySDR::Kwargs> results;
  // return results;

  std::cout << "findHiggs()" << std::endl;

  // read init.json off disk just for the purpose of grabbing ringbus settings
  HiggsSettings _settings(init_path, patch_path);
  HiggsSettings* settings = &_settings;

  const std::string role = GET_RADIO_ROLE();
	
  // INTEGRATION_TEST
  if( role != "arb" ) {

    HiggsRing rb( GET_HIGGS_IP(), GET_TX_CMD_PORT(), GET_RX_CMD_PORT() );

    rb.send(0, 0x10000001);

    uint32_t data, error;

    uint32_t timeout = 0;
    std::vector<uint32_t> got;

    while(timeout++ < 20) {

      rb.get(data, error);

      if( error == 0 ) {
        got.push_back(data);
      }

      cout << "timeout: " << timeout << " got " << got.size() << endl;

      if(got.size() == 10) {
        break;
      }
    }

    if(got.size() != 10) {
      cout << "Did not get correct number of packets" << endl;
      return results; // abort
    }

    for (auto& x : got) {
      cout << "x " << HEX32_STRING(x) << endl;
    }


    auto did_pass = true;
    auto j = 0;
    for(uint32_t i = 0x10000000; i < 0x1000000a; i++) {
      if(got[j] != i) {
        cout << "Incorret packet index " << j << endl;
        cout << "Expected: " << HEX32_STRING(i) << " got " << HEX32_STRING(got[j]) << endl;
        did_pass = false;
      }

      j++;
    }

    if( !did_pass ) {
      return results;
    }

    rb.send(0, 0x10000002);

    usleep(10*1000);

    rb.get(data, error);

    if( error == 0 ) {
      cout << "Eth sent a ringbus to itself, all ringbus forwarding ok" << endl;
    } else {
      return results; // bail
    }
}


  //locate the device on the system...
  //return a list of 0, 1, or more argument maps that each identify a device

  SoapySDR::Kwargs options;

  options["device"] = std::string("higgs");
  // options["blah"] = "foobar";
  options["driver"] = std::string("higgs");

  options["version"] = "4";
  options["part_id"] = "0";
  options["serial"] = "0";
  options["label"] = "higgs";
  
  // options["init_path"] = init_path;
  // options["patch_path"] = patch_path;

  results.push_back(options);

  return results;

}

/***********************************************************************
 * Make device instance
 **********************************************************************/
// naked function, not a member
SoapySDR::Device* makeHiggs(const SoapySDR::Kwargs &args)
{
  // SoapySDR::Kwargs::const_iterator it;

  // std::cout << "makeHiggs() got passed: " << endl;
  // for ( it = args.begin(); it != args.end(); it++ )
  // {
  //     std::cout << it->first  // string (key)
  //               << ':'
  //               << it->second   // string's value 
  //               << std::endl ;
  // }
  // std::cout << "------------------" << endl;

  // modem_main calls SoapySDR::Device::make() with the args
  // we pull the path of the two config files and pass to the higgs constructor
  // where they get used in making the settings object

  const auto init_path = args.at("init_path");
  const auto patch_path = args.at("patch_path");
  //create an instance of the device object given the args
  //here we will translate args into something used in the constructor
  return new HiggsDriver(init_path, patch_path);
}






/***********************************************************************
 * Our Stuff
 ***********************************************************************/

void HiggsDriver::createRb()
{
    if( rb == 0 ) {
        rb = new HiggsRing( GET_HIGGS_IP(), GET_TX_CMD_PORT(), GET_RX_CMD_PORT() );
    }
}

void HiggsDriver::destroyRb() {
    if( rb != 0 ) {
        delete(rb);
        rb = 0;
    }
}

// pass true to start stream
// pass false to stop
// void HiggsDriver::cs00ControlStream(const bool enable) {

//     uint32_t rhs;
//     if(enable) {
//         rhs = 1;
//     } else {
//         rhs = 0;
//     }

//     //////////////////////////rb->send(RING_ADDR_CS00, STREAM_CMD | (rhs)  );
//     rb->send(RING_ADDR_CS31, STREAM_CMD | (rhs)  );
// }

void HiggsDriver::cs00TurnstileAdvance(const uint32_t val) {
    //////////////////////////rb->send(RING_ADDR_CS00, TURNSTILE_CMD | (val&0xffffff)  );
    rb->send(RING_ADDR_RX_FFT, TURNSTILE_CMD | (val&0xffffff)  );
}

static uint32_t sanitizeWarnSync(const std::string& who, const uint32_t x) {
    if( (x % 5) != 0 ) {
        cout << "sending non multiple 5 value of " << x << " in " << who << "()\n";
    }

    if( x == 0 ) {
        cout << "sending zero value in " << who << "()\n";
    }

    const uint32_t amount = x&0xffffff;

    return amount;
}

void HiggsDriver::cs31TriggerSymbolSync(const uint32_t x) {

    const uint32_t amount = sanitizeWarnSync("cs31TriggerSymbolSync", x);

    //////////////////////////////rb->send(RING_ADDR_CS00, SYNCHRONIZATION_CMD | (amount)  );
    rb->send(RING_ADDR_RX_FFT, (SYNCHRONIZATION_CMD | amount)  );
}


void HiggsDriver::duplexTriggerSymbolSync(const uint32_t peer, const uint32_t x) {

    const uint32_t amount = sanitizeWarnSync("duplexTriggerSymbolSync", x);

    dspEvents->ringbusPeerLater(peer, RING_ADDR_RX_FFT, (DUPLEX_SYNCHRONIZATION_CMD | amount), 0);
}

void HiggsDriver::cs31CFOTurnoff(unsigned int data) {
  rb->send(RING_ADDR_RX_FFT, CFO_TURN_OFF_CMD|data);
}


void HiggsDriver::cs31CFOCorrection(const uint32_t val) {
    rb->send(RING_ADDR_RX_FFT, val);
}

void HiggsDriver::cs32SFOCorrection(const uint32_t val) {
  rb->send(RING_ADDR_RX_FINE_SYNC, SFO_CORRECTION_RX_CMD|val);
  cout<<"to send ------ "<<HEX32_STRING((SFO_CORRECTION_RX_CMD|val))<<endl;
}

void HiggsDriver::cs32SFOCorrectionShift(const uint32_t val) {
  rb->send(RING_ADDR_RX_FINE_SYNC, SFO_SHIFT_RX_CMD|val); 
}

void HiggsDriver::cs31CoarseSync(const uint32_t amount, const uint32_t direction) {

    const uint32_t a = SFO_PERIODIC_ADJ_CMD | amount ;
    const uint32_t b = SFO_PERIODIC_SIGN_CMD | direction ;

    rb->send(RING_ADDR_RX_FFT, a);
    usleep(500);
    rb->send(RING_ADDR_RX_FFT, b);

    cout << "sfo sent: " << HEX32_STRING(a) << "\n";
    cout << "sfo sent: " << HEX32_STRING(b) << "\n";

}

void HiggsDriver::cs32EQData(const uint32_t data)
{
  rb->send(RING_ADDR_RX_FINE_SYNC, EQ_DATA_RX_CMD|data); 
}

// This was used for rx side 
// void HiggsDriver::cs01TriggerFineSync() {
//     rb->send(RING_ADDR_CS01, SYNCHRONIZATION_CMD | (0x1)  );
// }
void HiggsDriver::requestFeedbackBusJam() {
    rb->send(RING_ADDR_RX_TAGGER, FEEDBACK_BUS_CMD | (0x0)  );
}

void HiggsDriver::cs11TriggerMagAdjust(const uint32_t val) {
    rb->send(RING_ADDR_RX_EQ, MAGADJUST_CMD | (val)  );
}

// unsure of units here
void HiggsDriver::setDSA(const uint32_t lower) {
    cout << "setDSA " << HEX32_STRING(lower) << "\n";
    rb->send(RING_ADDR_ETH, DSA_GAIN_CMD | (lower&0xffffff)  );
}


void HiggsDriver::setPhaseCorrectionMode(const uint32_t mode) {
    cout << "setPhaseCorrectionMode " << mode << "\n";
    rb->send(RING_ADDR_RX_EQ, RX_PHASE_CORRECTION_CMD | (mode&0xffffff)  );
}


// pass 0 to disable
// direction can either be add or delete samples
// 1 is deletes
// 2 is add

// sfo AND sto
void HiggsDriver::cs00SetSfoAdvance(const uint32_t _amount, const uint32_t _direction) {
    // mask to 24 bits before oring
    uint32_t amount = _amount & 0xffffff;
    uint32_t direction = _direction & 0xffffff;
    //////////////////////////rb->send(RING_ADDR_CS00, SFO_PERIODIC_ADJ_CMD | amount );
    rb->send(RING_ADDR_RX_FFT, SFO_PERIODIC_ADJ_CMD | amount );
    usleep(20);
    ///////////////////////////rb->send(RING_ADDR_CS00, SFO_PERIODIC_SIGN_CMD | direction );
    rb->send(RING_ADDR_RX_FFT, SFO_PERIODIC_SIGN_CMD | direction );
}


// void HiggsDriver::resetExperiment() {
//     _should_reset_experiment = true;
// }
// void HiggsDriver::resetBER() {
    // _should_reset_ber = true;
// }
// void HiggsDriver::manualSendChannelEq() {
    // _should_manual_eq = true;
// }
// void HiggsDriver::manualResetChannelEq() {
    // _should_manual_reset_eq = true;
// }
// void HiggsDriver::manualZeroChannelEq() {
    // _should_manual_zero_eq = true;
// }
// 
// void HiggsDriver::triggerTxSymbolSync() {
    // _should_tx_symbol_sync = true;
// }

void HiggsDriver::enterDebugMode(void) {

    cout << "S-Modem entering debug-mode\n";

    // sjs.settings.setBool('exp.skip_check_fb_datarate', true);
    // sjs.settings.setBool('exp.debug_stall', true);

    settings->vectorSet(true, (std::vector<std::string>){"exp","skip_check_fb_datarate"});
    settings->vectorSet(true, (std::vector<std::string>){"exp","debug_stall"});
}


///
/// Which transmitter are we?
/// this is reconstructed from exp.peers.t0 and exp.our.id and exp.our.role
/// returns -1 if we are the receiver
/// returns -2 if id is not in the list of transmitters
int32_t HiggsDriver::getTxNumber(void) {

    if( GET_RADIO_ROLE() != "tx" ) {
        return -1;
    }

    auto t0 = GET_PEER_T0();
    auto t1 = GET_PEER_T1();
    auto t2 = GET_PEER_T2();
    auto t3 = GET_PEER_T3();

    auto our_id = GET_OUR_ID();

    std::vector<uint32_t> allt = {t0,t1,t2,t3};

    for(unsigned i = 0; i < allt.size(); i++) {
        auto t = allt[i];

        if( t == our_id ) {
            // cout << "T found: " << t << " with id " << i << "\n";
            return i;
        }
    }

    // cout << "T0 " << t0 << "\n";
    // cout << "T1 " << t1 << "\n";
    // cout << "T2 " << t2 << "\n";
    // cout << "T3 " << t3 << "\n";

    cout << "Warning: getTxNumber() found we are in tx mode but exp.our.id was not found in transmit peers\n";

    return -2;
}

// based on tx peers, and DSP_WAIT_FOR_PEERS
// return
std::vector<uint32_t> HiggsDriver::getTxPeers(void) {
    auto t0 = GET_PEER_T0();
    auto t1 = GET_PEER_T1();
    auto t2 = GET_PEER_T2();
    auto t3 = GET_PEER_T3();

    // auto our_id = GET_OUR_ID();

    std::vector<uint32_t> allt = {t0,t1,t2,t3};

    auto number = DSP_WAIT_FOR_PEERS();

    allt.resize(number);

    return allt;
}


// pass role and tnum

uint32_t HiggsDriver::duplexEnumForRole(const std::string& _role, const int32_t tnum) {
    uint32_t role_enum = DUPLEX_ROLE_RX;
    if(_role == "tx") {
        switch(tnum) {
            default:
            case 0:
                role_enum = DUPLEX_ROLE_TX_0;
                break;
            case 1:
                role_enum = DUPLEX_ROLE_TX_1;
                break;
            case 2:
                role_enum = DUPLEX_ROLE_TX_2;
                break;
            case 3:
                role_enum = DUPLEX_ROLE_TX_3;
                break;
        }
        return role_enum;
    } else {
        return role_enum;
    }
}


template <typename T>
static void smodemSettings(T* xx) {
/*
    // int code_bad;
    // uint32_t code_length = 255;
    // uint32_t fec_length = 48;
    xx.print_settings_did_change = false;

    // TX Air Packet setup
    xx.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    xx.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);

    // code_bad = xx.set_code_type(AIRPACKET_CODE_REED_SOLOMON,
    //                                    code_length,
    //                                    fec_length);

    xx.set_interleave(0);

    // xx.enablePrint("mod");
    // xx.enablePrint("demod");
    // xx.disablePrint("all");
    // xx.enablePrint("all");
    xx.print25 = true;
    xx.print26 = true;
*/
    xx->print_settings_did_change = true;

    xx->set_duplex_mode(true);

    // qpsk 320
    xx->set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    xx->set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);


    // uint32_t code_length = 255;
    // uint32_t fec_length = 48; // 32

    // int code_bad;
    // code_bad = xx->set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
    // (void)code_bad;
    // if(!code_bad) {
    //     std::cout << "airRx->set_code_type returned " << code_bad << "\n";
    // }

    xx->set_interleave(0);

    xx->print5 = true;


    // xx->enablePrint("demod");
    // xx->disablePrint("all");
    // xx->enablePrint("all");

}





void HiggsDriver::sharedAirPacketSettings(AirPacketOutbound* const x) {
    smodemSettings(x);
}

void HiggsDriver::sharedAirPacketSettings(AirPacketInbound* const x) {
    smodemSettings(x);
}


/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerMyDevice("HiggsDriver", &findHiggs, &makeHiggs, SOAPY_SDR_ABI_VERSION);
