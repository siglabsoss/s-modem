#pragma once

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Types.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Errors.hpp>
#include <stdint.h>
#include <iostream>
#include <unistd.h> //usleep
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <set>

#include "HiggsRing.hpp"
// fixme, update paths in makefile
#include "cpp_utils.hpp"
#include <thread>

// include some riscv (.h) files
#include "ringbus.h"
//! These two .h files come from riscv include folder
//! the matching .c files are HACKED INCLUDED omg
//! See HiggsSoapyFeedbackBus.cpp
#include "feedback_bus.h"
#include "schedule.h"
// for side-control
// #include <jsonrpccpp/server/connectors/httpserver.h>

// for feedback bus
#include "common/discovery.hpp"
#include <zmq.hpp>
#include "common/constants.hpp"
#include "common/SafeQueue.hpp"

#include "driver/HiggsSettings.hpp"
#include "driver/HiggsFineSync.hpp"
#include "driver/StatusCode.hpp"

typedef std::function<void(const std::vector<uint32_t>&)> zmq_cb_t;
typedef std::function<void(uint32_t)> ringbus_cb_t;
typedef std::function<void(size_t, size_t)> event_cb_t;
typedef std::pair<std::string,std::vector<char>> internal_zmq_t;

class AttachedEstimate;
class RadioEstimate;
class HiggsEvent;
class EventDsp;
class FlushHiggs;
class TxSchedule;
class HiggsFineSync;
class StatusCode;
class DemodThread;
class EVMThread;
class HiggsSNR;
class AirPacket;
class AirPacketOutbound;
class AirPacketInbound;

template <unsigned int N, unsigned int GIVEUP> class HiggsUdpOOO;

// Soapy uses std::strings for formats, but we need to compare against this our fetch 
// map these to an int/enum value for faster comparisoins

// SOAPY_SDR_CS16
#define HIGGS_SOAPY_SDR_CS16 (0)
// SOAPY_SDR_CF32
#define HIGGS_SOAPY_SDR_CF32 (1)

#define FIXED_POINT_MAX_VAL (32768-1)



// size of udp packet (includes sequence)
#define UDP_RX_BYTES (1472u)
#define UDP_OOO_TOLERANCE (40u)

// called by gnuradio? to know what to call us with
#define PACKET_DATA_MTU_WORDS (4096)


// boost::asio::io_service io;

/***********************************************************************
 * Device interface
 **********************************************************************/
class HiggsDriver : public SoapySDR::Device
{
public:
    uint32_t junk;
    HiggsDriver(const std::string config_path, const std::string patch_path);
  
    ~HiggsDriver();

    /*******************************************************************
     * Settings API
     ******************************************************************/


    /*******************************************************************
     * Unused stubs:
     * Soapy has many stubbs we can implement in the future if we want
     * for now we include this .h file in the midde of our class (hacked)
     ******************************************************************/
#include "UnusedStubs.hpp"
  
    SoapySDR::Stream *setupStream(
      const int direction,
      const std::string &format,
      const std::vector<size_t> &channels = std::vector<size_t>(),
      const SoapySDR::Kwargs &args = SoapySDR::Kwargs() );
  

    void closeStream( SoapySDR::Stream *stream );
  
    int activateStream(
          SoapySDR::Stream *stream,
          const int flags = 0,
          const long long timeNs = 0,
          const size_t numElems = 0);
  
    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);

  
    //async api usage
    std::thread _rx_async_thread;
    std::thread _rb_print_thread;
    void drainOOOThread(void);
    void rxAsyncThread(void);
    bool rxAsyncGotPacket(HiggsUdpOOO<UDP_RX_BYTES,UDP_OOO_TOLERANCE>* const);
    void printRbThread(void);
  
    // higgs
    std::atomic<bool> higgsRunBoostIO;
    std::atomic<bool> higgsRunThread;
    std::atomic<bool> higgsRunOOOThread;
    std::atomic<bool> higgsRunRbPrintThread;
    std::atomic<bool> higgsRunZmqThread;
    std::atomic<bool> higgsRunTunThread;
    std::atomic<bool> higgsRunDemodThread;
    std::atomic<bool> higgsRunDspThread;
    std::atomic<bool> higgsRunFlushHiggsThread;
    std::atomic<bool> higgsRunTxScheduleThread;
    std::atomic<bool> higgsRunHiggsFineSyncThread;
    std::atomic<bool> higgsRunEVMThread;

    std::atomic<bool> higgsAttachedViaEth;

    HiggsUdpOOO<UDP_RX_BYTES,UDP_OOO_TOLERANCE>* _ooo_memory_p;
    // void rx_callback(unsigned char *buf, uint32_t len);
  

    void prepareEnvironment();
    void enableThreadByRole(const std::string&);
    void specialSettingsByRole(const std::string&);

  
    void discoverSocketInodes();

    // RX stuff
    int32_t bindRxStreamSocket();
    void releaseRxStreamSocket();
    struct sockaddr_in rx_address;
    const char *rx_ip;

    void printAllRunThread() const;


    // int rx_port;
    int rx_sock_fd;
    uint32_t rx_sock_timeout_us = 1000*1000;

    int grc_udp_sock_fd;
    struct sockaddr_in grc_udp_address;
    socklen_t grc_udp_address_len;

    int grc_udp_sock_fd_2;
    struct sockaddr_in grc_udp_address_2;
    socklen_t grc_udp_address_len_2;

    uint32_t rxFormat;

    // TX stuff
    int tx_sock_fd;
    struct sockaddr_in tx_address;
    socklen_t tx_address_len;
    int32_t connectTxStreamSocket();
    int32_t connectGrcUdpSocket();
    int32_t connectGrcUdpSocket2();
    std::mutex tx_sock_mutex;

    std::string tx_sock_inode;
    std::string rx_sock_inode;


    std::vector<unsigned char> pending_for_higgs;
    std::chrono::steady_clock::time_point time_prev_pet_send_higgs;
    std::chrono::steady_clock::time_point time_prev_sent_pet_higgs;

    // int sendToHiggs2(unsigned char* data, size_t len);
    void sendToHiggs(const unsigned char* const data, const size_t len, const uint32_t us_later = 0);
    void sendToHiggsLowPriority(const unsigned char* const data, const size_t len);
    void sendToHiggs(const std::vector<uint32_t> &vec);
    // void debugFeedbackBus();

    void createRb();
    void destroyRb();



    // Settings
    HiggsSettings _settings;
    HiggsSettings* settings = NULL;

    StatusCode* status;

    HiggsRing *rb;

    uint32_t activationsPerSetup;

    // Section of "wrapped" ringbus commands
    // void cs00ControlStream(const bool enable);
    void cs00TurnstileAdvance(const uint32_t val);
    void cs31TriggerSymbolSync(const uint32_t x = 50);
    void duplexTriggerSymbolSync(const uint32_t peer, const uint32_t x = 50);
    void cs31CoarseSync(const uint32_t amount, const uint32_t direction);
    void cs31CFOTurnoff(unsigned int data); 
    void cs31CFOCorrection(const uint32_t val);
    void cs32SFOCorrection(const uint32_t val);
    void cs32SFOCorrectionShift(const uint32_t val);
    void cs32EQData(const uint32_t data);
    // void cs01TriggerFineSync();
    void cs11TriggerMagAdjust(const uint32_t val);
    void cs00SetSfoAdvance(const uint32_t amount, const uint32_t direction);
    void __attribute__((deprecated)) setDSA(const uint32_t lower);
    void setPhaseCorrectionMode(const uint32_t mode);


    // trying a rx udp out of order thread
    std::thread _rx_ooo_thread;



    // zmq networking
    // void isZmq();
    std::chrono::steady_clock::time_point zmq_thread_start;
    std::chrono::steady_clock::time_point discovery_server_down_message;
    size_t discovery_server_message_count = 0;
    bool zmq_should_print_received = true;
    static constexpr bool zmq_should_listen = true;
    bool print_send_to_higgs = false;
    
    std::thread _zmq_rx_thread;
    void zmqRxThread(void);

    std::mutex internal_zmq_mutex;
    std::vector<internal_zmq_t> internal_zmq;
    // void zmqPublishInternal(const std::string& header, const char* const message, const size_t length);
    std::string zmqGetPeerTag(const size_t peer);
    void zmqPublishInternal(const std::string& header, const std::vector<uint32_t>& vec);


    zmq::context_t       zmq_context;
    zmq::socket_t        zmq_sock_pub; // bind and listen
    zmq::socket_t        zmq_sock_sub; // connects out

    void zmqHandleMessage(const std::string tag, const unsigned char* const data, const size_t length);
    
    void zmqHandleRemoteFeedbackBus(const std::string& tag, const unsigned char* const data, const size_t length);
    void zmqAcceptRemoteFeedbackBus(const std::string& tag, const unsigned char* const data, const size_t length);



    // Feedback Bus
    // most of these get called on _rx_async_thread
    void requestFeedbackBusJam();
    void zmqSubcribes(void);
    
    void boostIOThread(void);
    std::thread _boost_io_thread;
    // kept on the boost thread even though we are a member
    void updateDashboard(const std::string& msg);
    
    // zmq on the tx side puts into the buffer, and then the tun thread pulls out and puts into tun
    SafeQueue<std::vector<uint32_t>> demodRx2TunFifo;

    // tx side pulls out of tun, and puts into this, EventDspTx pulls out of this and sends over air
    SafeQueue<std::vector<uint32_t>> txTun2EventDsp;

    size_t txTunBufferLength();

    int tun_fd = 0;
    fd_set set_fdTun;


    // Feedback bus zmq
    std::vector<size_t> active_zmq;
    void zmqUpdateConnections(void);
    void patchDiscoveryTableForNat(void);
    void zmqPublish(const std::string& header, const char* const message, const size_t length);
    void zmqPublish(const std::string& header, const std::vector<uint32_t>& vec);
    void zmqPublishOrInternal(const size_t peer, const std::vector<uint32_t>& vec);


    // void zmqDebugPub(void);
    void zmqRingbusPeer(const size_t peer, const char* const message, const size_t length);
    void zmqSendFeedbackBusForPcBevOffThread(const unsigned char* const data, const size_t length); // in HiggsZmq.cpp
    static std::string zmqRbPeerTag(const size_t peer);
    std::mutex zmq_pub_mutex;

    // Higgs Discovery
    zmq::socket_t zmq_sock_discovery_push;
    zmq::socket_t zmq_sock_discovery_sub;
    discovery_t this_node;
    discovery_t node_on_disk;
    std::vector<discovery_t> discovery_nodes;
    void zmqSetupDiscovery(const uint32_t, const uint32_t);
    bool zmqPetDiscovery();
    void discoveryCheckIdentity();
    std::atomic<size_t> connected_node_count;
    std::atomic<bool> _this_node_ready;
    bool thisNodeReady() const;
    std::atomic<bool> _flush_higgs_ready;
    bool flushHiggsReady() const;
    std::atomic<bool> _zmq_thread_ready;
    bool zmqThreadReady() const;
    std::atomic<bool> _boost_thread_ready;
    bool boostThreadReady() const;
    std::atomic<bool> _dsp_thread_ready; // includes buffers
    bool dspThreadReady() const;
    std::atomic<bool> _tx_schedule_ready;
    bool txScheduleReady() const;
    std::atomic<bool> _demod_thread_ready;
    bool demodThreadReady() const;

    // Dsp Event Base
    EventDsp* dspEvents = 0;

    // Event class that is just for Higgs
    FlushHiggs* flushHiggs = 0;
    TxSchedule* txSchedule = 0;
    DemodThread* demodThread = 0;

    HiggsFineSync* higgsFineSync = 0;

    EVMThread* evmThread = 0;

    bool accept_remote_feedback_bus_higgs;

    HiggsSNR* higgsSNR = 0;
    // performance
    // size_t setupPerformance();
    // void logPerformance(size_t handle);


    // Tun Thread
    // int tunAlloc(char *dev, int flags, bool setTimeout = false);
    void tunThread(void);
    void tunThreadAsRx();
    void tunThreadAsTx();
    // void tunThreadAsArbiter();
    std::thread _tun_thread;

    // for node js
    // pass std function by VALUE not by reference, this prevents segfaults
    void registerRingbusCallback(const ringbus_cb_t);
    void ringbusCallbackCatchType(const uint32_t);
    ringbus_cb_t ringCallback=0;
    mutable std::mutex js_ringbus_mutex; // just protects this vector, not the callback
    std::vector<uint32_t> callbackShouldCatch;

    void registerEventCallback(const event_cb_t);
    event_cb_t eventCallback=0;

    mutable std::mutex _dash_sock_mutex;


    void enterDebugMode(void);

    int32_t getTxNumber(void);
    std::vector<uint32_t> getTxPeers(void);

    static uint32_t duplexEnumForRole(const std::string& _role, const int32_t tnum);

    void printTxRxUnderflow(const uint32_t data);

    void sharedAirPacketSettings(AirPacketOutbound* const x);
    void sharedAirPacketSettings(AirPacketInbound* const x);

private:
    // std::vector<float> _sample_value;
    // void create_sample_lookup();

};

SoapySDR::KwargsList findHiggs(const SoapySDR::Kwargs &args);
SoapySDR::Device* makeHiggs(const SoapySDR::Kwargs &args);

#include "HiggsStructTypes.hpp"