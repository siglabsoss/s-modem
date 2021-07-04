#include "HiggsSoapyDriver.hpp"
#include <algorithm> //min
#include <cstring> // memcpy

#include "CustomEventTypes.hpp"
#include "RadioEstimate.hpp"
#include "AttachedEstimate.hpp"
#include "HiggsFineSync.hpp"
#include "FlushHiggs.hpp"
#include "TxSchedule.hpp"
#include "EventDsp.hpp"
#include "DemodThread.hpp"
#include "EVMThread.hpp"
#include "AirPacket.hpp"
#include "common/CmdRunner.hpp"
#include "HiggsUdpOOO.hpp"
#include "HiggsSNR.hpp"


#include <fcntl.h>


using std::cout;
using std::endl;




// REALLY useful, where to put?
static std::string exec(const char* cmd) {
    std::array<char, 1024> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static int iNodeForSocket(int fd, bool print=false) {
    auto pid = ::getpid();

    if(print) {
        cout << pid << endl;
    }

    std::string stat_path = std::string("/proc/") + std::to_string(pid) + std::string("/fd/") + std::to_string(fd);

    if(print) {
        cout << "built path: " + stat_path << endl;
    }

    std::string cmd = "/usr/bin/stat "; // need trailing space
    cmd += stat_path;

    // FIXME: this exec() could be replaced by CmdRunner.hpp
    auto res = exec(cmd.c_str());

    std::istringstream stream(res);

    std::string token;

    // std::getline(stream, token, '>');
    std::getline(stream, token, '[');
    std::getline(stream, token, ']');

    // this is NOT the number after Inode:
    // instead it's the (other?) inode which /proc/net/udp uses
    int inode = std::stoi(token);

    if( print ) {
        cout << "token: " << token << endl;
        cout << "inode: " << inode << endl;
        cout << "got res " << res << endl;
    }

    return inode;
}




// This should be the "expensive" call. ie setup buffers and do large allocations here
// see activateStream
SoapySDR::Stream* HiggsDriver::setupStream(
  const int direction,
  const std::string &format,
  const std::vector<size_t> &channels,
  const SoapySDR::Kwargs &args )
{
    (void)direction;
    (void)format;
    (void)channels;
    (void)args;
    std::cout << "setupStream( direction = " << direction << " #channels = " << channels.size() << " format = " << format << " )" << std::endl;

    if(!GET_GNURADIO_UDP_DO_CONNECT()) {
        cout << "------------------------------------------------------" << endl;
        cout << "-" << endl;
        cout << "-   Gnuradio UDP will not have any samples" << endl;
        cout << "-" << endl;
        cout << "------------------------------------------------------" << endl;
    }

    // will throw if node.csv can't be found
    discoveryCheckIdentity();

    activationsPerSetup = 0; // how many times has activateStream been called since setup stream

    if (direction != SOAPY_SDR_RX)
    {
        throw std::runtime_error("Higgs Driver currently only supports RX, use SOAPY_SDR_RX");
    }


    // create_sample_lookup();
    // manually setting this because I can't figure out how to parse
    // options from osmocom

    // loop through all the formats that the driver supports, and throw
    // if we were asked for one we don't support
    std::vector<std::string> our_formats = getStreamFormats(direction, 0);
    bool format_supported = false;

    for (auto const& f : our_formats) {
        std::cout << f << std::endl;
        if( f == format ) {
            format_supported = true;
            break;
        }
    }

    if( !format_supported ) {
        throw std::runtime_error("Format \"" + format + "\" is not supported by Higgs Driver");
    }

    // convert str to int
    // this needs to be updated if getStreamFormats() is
    if( format == SOAPY_SDR_CS16 ) {
        rxFormat = HIGGS_SOAPY_SDR_CS16;
    } else if(format == SOAPY_SDR_CF32) {
        rxFormat = HIGGS_SOAPY_SDR_CF32;
    } else {
        throw std::runtime_error("Format code is not internally consistent");
    }


    // in the RTLSDR example, we return this object as the rx stream pointer
    // because that code was only written as rx, they don't need to differentiate between tx and rx
    // in hackrf code, there are sub structures that are created and returned here
    return (SoapySDR::Stream *) this;
 
}

// run multiple bash commands here for things like the tun?
void HiggsDriver::prepareEnvironment() {

    if( GET_CREATE_TUN() ) {

        // FIXME: sanity check this string, could be used to exploit
        std::string ip = GET_CREATE_TUN_IP();
        std::string user = GET_CREATE_TUN_USER();

        cout << "------------------------------------------------------" << endl;
        cout << "-" << endl;
        cout << "-   Making tun " << ip << " for user " << user << endl;
        cout << "-" << endl;
        cout << "------------------------------------------------------" << endl;

        
        std::string cmd = std::string("")
        .append("openvpn --mktun --dev tun1 --user ").append(user).append("; ")
        .append("ip link set tun1 up && ")
        .append("ip addr add ").append(ip).append(" dev tun1 ;");

        auto packed = CmdRunner::runOnceDebug(cmd);

        int retval = std::get<0>(packed);
        std::string output = std::get<1>(packed);

        if(retval != 0) {
            cout << " Command to setup tun failed with: " << endl << endl <<
            output << endl;
            assert(false);
        }

        // cout << "would run " << cmd << endl;
    }

}

void HiggsDriver::discoverSocketInodes() {
    if(true) {
        cout << "tx_sock_fd " << tx_sock_fd << endl;
        cout << "rx_sock_fd " << rx_sock_fd << endl;
    }

    
    tx_sock_inode = std::to_string(iNodeForSocket(tx_sock_fd));
    rx_sock_inode = std::to_string(iNodeForSocket(rx_sock_fd));

    if(true) {
        cout << "Found tx inode " << tx_sock_inode << endl;
        cout << "Found rx inode " << rx_sock_inode << endl;
    }
}


static void setThreadPriority(std::thread &_thread, int policy, int priority) {
    sched_param thread_priority;
    thread_priority.sched_priority = priority;
    // int thread_schedule_policy = policy;

    if(pthread_setschedparam(_thread.native_handle(), policy, &thread_priority)) {
        std::cout << "Failed to set Thread scheduling : " << std::strerror(errno) << "\n";
    }
}

void HiggsDriver::enableThreadByRole(const std::string& r) {
    if( r == "arb") {
        higgsRunBoostIO              = false;
        higgsRunThread               = false;
        higgsRunOOOThread            = false;
        higgsRunRbPrintThread        = false;
        higgsRunZmqThread            = true;
        higgsRunTunThread            = true;
        higgsRunDemodThread          = false;
        higgsRunDspThread            = true;
        higgsRunFlushHiggsThread     = false;
        higgsRunTxScheduleThread     = false;
        higgsRunHiggsFineSyncThread  = false;
    }
}


void HiggsDriver::specialSettingsByRole(const std::string& r) {

    cout << "specialSettingsByRole() for " << r << "\n";
    if( r == "arb") {
        higgsAttachedViaEth = false;

        settings->vectorSet(false, PATH_FOR(GET_GNURADIO_UDP_DO_CONNECT()));

        // will drop any zmq messages destined for our higgs (which we don't have)
        // see HiggsDriver::zmqAcceptRemoteFeedbackBus()
        accept_remote_feedback_bus_higgs = false;

    }
}


// this should be the fast/light call, (compared to setupStream)
// this may be called on/off quickly
int HiggsDriver::activateStream(
        SoapySDR::Stream *stream,
        const int flags,
        const long long timeNs,
        const size_t numElems)
{
    (void)stream;
    (void)flags;
    (void)timeNs;
    (void)numElems;
    if (flags != 0) return SOAPY_SDR_NOT_SUPPORTED;

    // call bash commands for tun etc
    prepareEnvironment();

    std::cout << "activateStream()" << std::endl;


    // cout << settings.get2("role") << endl;
    // cout << settings.get2("rolee") << endl; // throws

    // cout << settings.get3(uint32_t, "global", "") << endl;
    // cout << settings.get3<uint32_t>("global", "mole", "third") << endl;
    // cout << "pull:" << settings.get3<bool>("global", "performance", "DEBUG_PRINT_LOOPS") << endl;

    // exit(0);

    if( higgsAttachedViaEth ) {
        bindRxStreamSocket();
        connectTxStreamSocket();

        discoverSocketInodes();
    }

    if( GET_GNURADIO_UDP_DO_CONNECT() ) {
        connectGrcUdpSocket();
        connectGrcUdpSocket2();
        cout << "GET_GNURADIO_UDP_DO_CONNECT() set\n";
    } else {
        cout << "GET_GNURADIO_UDP_DO_CONNECT() NOT SET\n";
    }


    printAllRunThread();



    grc_udp_address_len = sizeof(grc_udp_address);
    grc_udp_address_len_2 = sizeof(grc_udp_address_2);
    /**
     * Set up stuff to run before threads are started
     */

    if( higgsRunEVMThread ) {
        evmThread = new EVMThread(this);
        evmThread->setThreadPriority(SCHED_BATCH, 0);
    }


    // FIXME: moving this first
    // caused us not to segfault
    // we need a global "reset" to keep threads in line
    if( higgsRunBoostIO ) {
        if (not _boost_io_thread.joinable())
        {
          std::cout << "activateStream()._boost_io_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _boost_io_thread = std::thread(&HiggsDriver::boostIOThread, this);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._boost_io_thread.joinable()";
        }
    }


    if( higgsRunThread ) {
        if (not _rx_async_thread.joinable())
        {
          std::cout << "activateStream()._rx_async_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _rx_async_thread = std::thread(&HiggsDriver::rxAsyncThread, this);
          setThreadPriority(_rx_async_thread, SCHED_RR, 30);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._rx_async_thread.joinable()";
        }
    }

    if( higgsRunOOOThread ) {
        if (not _rx_ooo_thread.joinable())
        {
          std::cout << "activateStream()._rx_ooo_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _rx_ooo_thread = std::thread(&HiggsDriver::drainOOOThread, this);
          setThreadPriority(_rx_ooo_thread, SCHED_RR, 29);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._rx_ooo_thread.joinable()";
        }
    }


    if( higgsRunRbPrintThread ) {
        if (not _rb_print_thread.joinable())
        {
          std::cout << "activateStream()._rb_print_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _rb_print_thread = std::thread(&HiggsDriver::printRbThread, this);
          setThreadPriority(_rb_print_thread, SCHED_RR, 40);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._rb_print_thread.joinable()";
        }
    }


    if( higgsRunZmqThread ) {
        if (not _zmq_rx_thread.joinable())
        {
          std::cout << "activateStream()._zmq_rx_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _zmq_rx_thread = std::thread(&HiggsDriver::zmqRxThread, this);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._zmq_rx_thread.joinable()";
        }
    }

    if( higgsRunTunThread ) {
        if (not _tun_thread.joinable())
        {
          std::cout << "activateStream()._tun_thread.joinable()" << std::endl;
          // rtlsdr_reset_buffer(dev);
          _tun_thread = std::thread(&HiggsDriver::tunThread, this);
        }
        else
        {
            std::cout << "Something went wrong with activateStream()._tun_thread.joinable()";
        }
    }

    if( higgsRunDemodThread ) {
        const auto that = this;

        // DemodThread now requires we pass a lambda
        // to setup the Air object initially
        // since this setup happens at beginning, we have to pass it to constructor
        auto setupAirCb = [&](AirPacketInbound* const xx){
            that->sharedAirPacketSettings(xx);
        };

        demodThread = new DemodThread(this, setupAirCb);

        // INTEGRATION_TEST
        // demodThread->setThreadPriority(SCHED_RR, 40);
        demodThread->setThreadPriority(SCHED_BATCH, 0);
    }


    higgsSNR = new HiggsSNR(this);

    if( higgsRunDspThread ) {
        // call constructor to fire up thread and start consuming events
        dspEvents = new EventDsp(this);
        dspEvents->setThreadPriority(SCHED_RR, 20);
    }


    if( higgsRunFlushHiggsThread ) {
        // this starts a thread behind the scenes
        flushHiggs = new FlushHiggs(this);
        flushHiggs->setThreadPriority(SCHED_RR, 30);
    }

    if( higgsRunTxScheduleThread ) {
        txSchedule = new TxSchedule(this);
    }

    if( higgsRunHiggsFineSyncThread ) {
        higgsFineSync = new HiggsFineSync(this);
    }


    // setup callbacks
    if( higgsRunDemodThread ) {
        if( higgsRunDspThread ) {

            EventDsp* _dsp = dspEvents;

            demodThread->setRetransmitCallback([_dsp](const size_t peer, const uint8_t seq, const std::vector<uint32_t>& indices) {
                // cout << "inside sendRetransmitRequestToPartner() lambda\n"; //  FIXME remove once it works
                _dsp->sendRetransmitRequestToPartner(peer, seq, indices);
            });
        } else {
            cout << "!!! Weirdly RunDemodThread is set but RunDspThread is not will not set callback\n";
        }
    }




    // auto end = std::chrono::steady_clock::now();
    // std::chrono::duration<double> diff = end-start;

    // fixme probably could be moved to setup stream
    // if bind rx stream / thread starting was also moved
    // if this is the first call to activateStream(), we should flush the output dma
    // so that packets will be aligned from eth, and there will be no junk


    if( higgsAttachedViaEth ) {
        requestFeedbackBusJam();
        // cs00ControlStream(true);
    }

    // at this point, we might want to flush / fix individual input DMA, but at the beginning this should be ok?
    // we need to turn on the stream from CS00


    activationsPerSetup++;
    return 0;
}

void HiggsDriver::printAllRunThread() const {
    std::cout << "\n";
    std::cout << "  higgsRunBoostIO " << (higgsRunBoostIO ? "true":"false") << "\n";
    std::cout << "  higgsRunThread " << (higgsRunThread ? "true":"false") << "\n";
    std::cout << "  higgsRunOOOThread " << (higgsRunOOOThread ? "true":"false") << "\n";
    std::cout << "  higgsRunRbPrintThread " << (higgsRunRbPrintThread ? "true":"false") << "\n";
    std::cout << "  higgsRunZmqThread " << (higgsRunZmqThread ? "true":"false") << "\n";
    std::cout << "  higgsRunTunThread " << (higgsRunTunThread ? "true":"false") << "\n";
    std::cout << "  higgsRunDemodThread " << (higgsRunDemodThread ? "true":"false") << "\n";
    std::cout << "  higgsRunDspThread " << (higgsRunDspThread ? "true":"false") << "\n";
    std::cout << "  higgsRunFlushHiggsThread " << (higgsRunFlushHiggsThread ? "true":"false") << "\n";
    std::cout << "  higgsRunTxScheduleThread " << (higgsRunTxScheduleThread ? "true":"false") << "\n";
    std::cout << "  higgsRunHiggsFineSyncThread " << (higgsRunHiggsFineSyncThread ? "true":"false") << "\n";
    std::cout << "\n";
}

int HiggsDriver::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    (void)stream;
    (void)flags;
    (void)timeNs;
    std::cout << "deactivateStream()" << std::endl;

    dspEvents->stopThread();

    usleep(1000000*3);

    flushHiggs->stopThread();

    // moving to rx thread?
    // cs00ControlStream(false);

    // there will be some flushing period
    std::this_thread::sleep_for(std::chrono::milliseconds(1000*1));






    if (flags != 0) return SOAPY_SDR_NOT_SUPPORTED;

    // take down ooo thread first (before rx at least)
    // if(_rx_ooo_thread.joinable()) {
    //     std::cout << "deactivateStream()._rx_ooo_thread.joinable()" << std::endl;
    //     higgsRunOOOThread = false;

    //     _rx_ooo_thread.join();
    // }



    if (_rx_async_thread.joinable())
    {
      std::cout << "deactivateStream()._rx_async_thread.joinable()" << std::endl;
      higgsRunThread = false; // will be checked by thread and it will exit
      // rtlsdr_cancel_async(dev);
      _rx_async_thread.join();
    }

    if(_rb_print_thread.joinable()) {
        std::cout << "deactivateStream()._rb_print_thread.joinable()" << std::endl;
        higgsRunRbPrintThread = false;

        _rb_print_thread.join();
    }


    if(_zmq_rx_thread.joinable()) {
        std::cout << "deactivateStream()._zmq_rx_thread.joinable()" << std::endl;
        higgsRunZmqThread = false;

        _zmq_rx_thread.join();
    }

    if(_tun_thread.joinable()) {
        std::cout << "deactivateStream()._tun_thread.joinable()" << std::endl;
        higgsRunTunThread = false;

        _tun_thread.join();
    }



    // stopLibeventThread();

    return 0;
}

void HiggsDriver::printTxRxUnderflow(const uint32_t data) {

    uint32_t counter_value = (data&0xffff);

    // mask we feed into the switch statement
    uint32_t switch_on = ((data & 0xff0000) >> 16)&0xff;


    std::ostringstream os;

    os  << "--------------------------------------------\n"
        << " - \n"
        << " - \n"
        << " - \n"
        << " - ";

    switch(switch_on) {
        case 0:
            os << " CS01 Underflow ended ";
            break;
        case 1:
            os << " CS01 Underflow began ";
            break;
        case 3:
            os << " Underflow Counter Lower " << HEX_STRING(counter_value) << " ";
            break;
        case 4:
            os << " Underflow Counter Upper " << HEX_STRING(counter_value) << " ";
            break;
        case 5:
            os << " CS31 Overflow ended ";
            break;
        case 6:
            os << " CS31 Overflow began ";
            break;
        case 7:
            os << " Overflow Counter Lower " << HEX_STRING(counter_value) << " ";
            break;
        case 8:
            os << " Overflow Counter Upper " << HEX_STRING(counter_value) << " ";
            break;
        default:
            os << "printTxRxUnderflow() does not understand " << HEX32_STRING(data) << "\n";
            break;
    }


    os 
    << "\n - \n"
    << " - \n"
    << " - \n"
    << "--------------------------------------------\n";

    cout << os.str();
}



/// A thread which pulls values out of ringbus socket
/// There are multiple paths if we are rx or tx
/// For some high priority ringbus, we directly insert them into their targets
/// for the rest of them, we pass them to eventDsp via a BEV.  This turns out to be slow
/// because eventDsp is overloaded right now.  A flag `skip_bev` determines this
/// see `EventDsp::dspDispatchAttachedRb`
void HiggsDriver::printRbThread(void)
{
    // this thread relies on soapy->flushHiggs
    // so block until that thread is up
    while(!flushHiggsReady()) {
        usleep(1);
    }

    std::cout << "printRbThread()";
    std::cout << " in both mode";
    std::cout << "\n";

    bool print_this_rb;


    //higgsRunRbPrintThread
    while(higgsRunRbPrintThread) {
        uint32_t word, rb_error;
        rb->get(word, rb_error);
        if( rb_error == 0 ) {

            print_this_rb = true;
            bool skip_bev = false;
            const uint32_t cmd_type = word & 0xff000000;
            const uint32_t cmd_data = word & 0xffffff;
            // this radio is tx mode

            // common code for both tx/rx mode
            switch(cmd_type) {
                case TX_UNDERFLOW:
                    skip_bev = true;
                    printTxRxUnderflow(cmd_data);
                    break;
                case FILL_REPLY_PCCMD:
                    flushHiggs->consumeRingbusFill(word);
                    skip_bev = true;
                    print_this_rb = false;
                    break;
                case TX_PROGRESS_REPORT_PCCMD:
                case TX_EPOC_REPORT_PCCMD:
                    dspEvents->attached->consumeContinuousEpoc(word);
                    skip_bev = true;
                    print_this_rb = false;
                    break;
                // case TX_FILL_LEVEL_PCCMD:
                case TX_UD_LATENCY_PCCMD:
                    // shared for duplex and non duplex as it just copies
                    flushHiggs->consumeUserdataLatencyEarly(word);
                    print_this_rb = false;
                    break;
                case TX_USERDATA_ERROR:
                    print_this_rb = false;
                    break;
                case READBACK_HASH_PCCMD:
                    skip_bev = true;
                    print_this_rb = false;
                    rb->handleHashReply(word);
                    break;
                case GENERIC_READBACK_PCCMD:
                    print_this_rb = false;
                    break;
                case FEEDBACK_HASH_PCCMD:
                    print_this_rb = false;
                    break;
                case TIMER_RESULT_PCCMD:
                    print_this_rb = true;
                    skip_bev = true;
                    break;
                case DEBUG_18_PCCMD:
                case DEBUG_OTA_FRAME_PCCMD:
                case RX_COUNTER_SYNC_PCCMD:
                    print_this_rb = false;
                    skip_bev = true;
                    break;

                case DEBUG_COARSE_COUNTER_PCCMD:
                    cout << "Occupancy at time of coarse sync: " << (word&0xffffff) << "\n";
                    print_this_rb = false;
                    skip_bev = true;
                    break;

                case NODEJS_RCP_PCCMD:
                case UART_READOUT_PCCMD:
                case POWER_RESULT_PCCMD:
                case RX_FINE_SYNC_OVERFLOW_PCCMD:
                case EXFIL_READOUT_PCCMD:
                case LAST_USERDATA_PCCMD:
                case TX_FB_REPORT_PCCMD:      // related to cs11 and fb bus jams
                case TX_FB_REPORT2_PCCMD:     // related to cs11 and fb bus jams
                case TX_REPORT_STATUS_PCCMD:  // related to cs11 and fb bus jams
                case FINE_SYNC_RAW_PCCMD:
                    print_this_rb = false;
                    break;
                default:
                    break;
            }



            if(!skip_bev) {
                bufferevent_write(dspEvents->rb_in_bev->in, &word, sizeof(word));
            }

            if( print_this_rb ) {
                // auto futWork = std::async( std::launch::async,  [](uint32_t _word) {
                std::cout << "Ringbus: 0x" << HEX32_STRING(word) << "\n";
                // }, word );
            }

            if( ringCallback ) {
                bool found = false;
                (void) found;


                // hold this lock and iterate through
                {
                    std::lock_guard<std::mutex> lock(js_ringbus_mutex);
                    
                    for( auto x : callbackShouldCatch) {
                        if( cmd_type == x ) {
                            found = true;
                            // ringCallback(word);
                            break;
                        }
                    }
                }

                // lock is released by the exit of scope, now call our function
                // without holding the lock
                if( found ) {
                    ringCallback(word);
                }

                // if( found ) {
                //     cout << HEX32_STRING(cmd_type) << " passed rb to callback" << endl;
                // } else {
                //     cout << HEX32_STRING(cmd_type) << " skipped callback" << endl;
                // }
            } else {
                // cout << "Skipping ringCallback due to callback not being set" << endl;
            }

        }

    }

    std::cout << "printRbThread() closing\n";
}


// returns handle
// size_t HiggsDriver::setupPerformance() {
//     return 0;
// }
// void HiggsDriver::logPerformance(size_t handle) {

// }



// returns true if should run again
bool HiggsDriver::rxAsyncGotPacket(HiggsUdpOOO<UDP_RX_BYTES,UDP_OOO_TOLERANCE>* const memory) {
    unsigned char* reordered_pkt;

    bool run_again = false;

    // cout << "rxAsyncGotPacket" << "\n";

    reordered_pkt = memory->getNext();
    if( reordered_pkt != 0 ) {

        bufferevent_write(this->dspEvents->udp_payload_bev->in, reordered_pkt+4, (UDP_RX_BYTES-4));
        memory->release(reordered_pkt);
        run_again = true;

        // get a pointer to a re-ordered packet

// #ifdef DEBUG_PRINT_LOOPS
//     static int calls2 = 0;
//     calls2++;
//     if( calls2 % 50000 == 0) {
//         cout << "while( reordered_pkt != 0 ) {" << endl;
//     }
// #endif

            // now = std::chrono::steady_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
            // // cout << now-then << endl;
            // if( duration > 300  ){
            //     cout << duration  << endl;
            // }
            // then = now;



#ifdef DEBUG_PRINT_LOOPS
    static int calls3 = 0;
    calls3++;
    if( calls3 % 50000 == 0) {
        struct evbuffer *input_ = bufferevent_get_input(dspEvents->udp_payload_bev->out);
        auto len = evbuffer_get_length(input_);

        // input);
        cout << "----------if( dspEvents && && ) " << len << endl;

#ifdef DEBUG_PRINT_THREAD_INFO
    pthread_getcpuclockid(pthread_self(), &threadClockId);
//! Using thread clock Id get the clock time
    clock_gettime(threadClockId, &currTime);
    cout << "----------sec: " << currTime.tv_sec << " fraction: " << (double)currTime.tv_nsec/1E9 << endl;

    time_t now = 0;
    time(&now);
    printf("----------time_t %jd\n", (intmax_t) now);
    // or 
    printf("----------time_t %lld\n", (long long) now);

#endif
    }
#endif


    } 

    return run_again;
}

void HiggsDriver::drainOOOThread(void) {
    // cout << "drainOOO\n";
    while(higgsRunThread) {
        bool ready = false;
        if( dspEvents && dspEvents->udp_payload_bev && dspEvents->udp_payload_bev->in && dspThreadReady() ) {
            ready = true;
        }
        if(ready) {
            break;
        }
        usleep(100);
    }

    bool run_again = true;

    while(higgsRunThread) {

        do {
            run_again = rxAsyncGotPacket(_ooo_memory_p);
        } while(run_again);

        usleep(500);
    }
}


#include <fstream>


void HiggsDriver::rxAsyncThread()
{
    std::cout << "rxAsyncThread() " << std::endl;
    // rb->zeroTimeout(true);
    //printf("rx_async_thread\n");
    // rtlsdr_read_async(dev, &_rx_callback, this, asyncBuffs, bufferLength);
    //printf("rx_async_thread done!\n");
    
    // uint32_t data[1024];
    // unsigned char *data_char = (unsigned char *)data;
    
    // uint32_t mod = 1000000;
    // uint32_t counter = 0;
    // uint32_t bump = 0;
    // uint32_t debug_out = 0;
    #define VLEN 10
    // #define TIMEOUT 1
    int retval;
    struct mmsghdr msgs[VLEN];
    struct iovec iovecs[VLEN];
    // struct timespec timeout;
    std::chrono::steady_clock::time_point then = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    (void)then;
    (void)now;

    int sock = rx_sock_fd;

    while(higgsRunThread) {
        bool ready = false;
        if( dspEvents && dspEvents->udp_payload_bev && dspEvents->udp_payload_bev->in && dspThreadReady() ) {
            ready = true;
        }
        if(ready) {
            break;
        }
        usleep(100);
    }
    // usleep(1000);

    std::cout << "rxAsyncThread() ready" << std::endl;

    uint32_t _prev_drops = 0;

    for (std::size_t i = 0; i < VLEN; i++) {
        iovecs[i].iov_base         = _ooo_memory_p->get();
        iovecs[i].iov_len          = UDP_PACKET_DATA_SIZE;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
    }
    // timeout.tv_sec = TIMEOUT;
    // timeout.tv_nsec = 0;
    while(higgsRunThread) {
// #ifdef DEBUG_PRINT_LOOPS
//     static int calls = 0;
//     calls++;
//     if( calls % 50000 == 0) {
//         cout << "while(higgsRunThread) {" << endl;
//     }
// #endif



        static bool _file_open = false;
        static std::ofstream _ofile;
        std::string _fname = "seq_nums.txt";

        if( false ) {
            if(!_file_open) {
                _file_open = true;
                _ofile.open(_fname);
                cout << "OPENED " << _fname << "\n";
            }

            
        }


        // this comment may be old:
        // passing 0 as flags (instead of MSG_DONTWAIT) to recv
        // means we will wait for rx_sock_timeout_us for a packet
        // setting this number higher means we have less time for
        // other stuff on this thread, but right now we aren't doing anything else
        // rx_val = recv(rx_sock_fd, pkt, UDP_PACKET_DATA_SIZE, 0);
        // MSG_WAITFORONE
        // MSG_DONTWAIT
        retval = recvmmsg(sock, msgs, VLEN, MSG_WAITFORONE, 0);
        if( retval == -1) {
            std::cout << "ERROR: recvmmsg()\n";
        } else {
            // for (std::size_t i = 0; i < retval; i++) {
            //     std::cout << "Message " << i << " length: " << msgs[i].msg_len << "\n";
            // } 
            // if( retval != UDP_PACKET_DATA_SIZE ) {
                // cout << "ERROR wrong sized packet " << rx_val << endl;
            // }

            // tell our class there is memory here
            for (ssize_t i = 0; i < retval; i++) {
                _ooo_memory_p->markFull((unsigned char*) iovecs[i].iov_base);

                uint32_t *d = (uint32_t*)iovecs[i].iov_base;
                _ofile << d[0] << "\n";
            }

            for (ssize_t i = 0; i < retval; i++) {
                iovecs[i].iov_base = _ooo_memory_p->get();
            }
            // pkt = memory.get();
        }

        // if( retval > 1 ) {
        //     cout << "retval: " << retval << "\n";
        // }

        if( _prev_drops != _ooo_memory_p->num_drops ) {
             dspEvents->sendEvent(MAKE_EVENT(UDP_SEQUENCE_DROP_EV,_ooo_memory_p->num_drops));
            _prev_drops = _ooo_memory_p->num_drops;
        }


        // usleep(40); // this simulates putting "load" on the cpu
    } // while (higgsRunThread)

    cout << "rx_async_thread is turning off cs00 stream" << endl;
    // cs00ControlStream(false);
    usleep(100);
    cout << "rx_async_thread() thread closing" << endl;
}





void HiggsDriver::closeStream( SoapySDR::Stream *stream )
{
  this->deactivateStream(stream, 0, 0);
}


// creats and binds the socket
int32_t HiggsDriver::bindRxStreamSocket()
{
    memset((char *)&rx_address, 0, sizeof(rx_address));

    // tx_address.sin_family = AF_INET;
    // tx_address.sin_port = htons(tx_port);
    // tx_address.sin_addr.s_addr = inet_addr(ip);

    rx_address.sin_family = AF_INET;
    rx_address.sin_port = htons(GET_RX_DATA_PORT());
    rx_address.sin_addr.s_addr = inet_addr(GET_PC_IP().c_str());

    rx_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(rx_sock_fd < 0){
        std::cout << "ERROR: Socket file descriptor was not allocated" << std::endl;
        return -1;
    }
    else{
        std::cout << "Socket construction complete" << std::endl;
        // return 0;
    }

    // set socket options BEFORE bind


    // Set timeout for rx data socket
    // struct timeval tv;
    // tv.tv_sec = 0;
    // tv.tv_usec = rx_sock_timeout_us;
    // setsockopt(rx_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int buf_size = 99999999;
    if (setsockopt(rx_sock_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(int)) == -1) {
        std::cout << "Error setting socket opts: " << strerror(errno) << "\n";
    }

    int bind_successful = -1;
    bind_successful = bind(rx_sock_fd, (struct sockaddr*)&rx_address, sizeof(rx_address));


    if(bind_successful < 0){
        std::cout << "ERROR: Unable to bind to Data receive socket: " << GET_RX_DATA_PORT() << std::endl;
        exit(1);

        return -2;
    }
    
    std::cout << "Socket bind successful" << std::endl;
    
    return 0;

}
void HiggsDriver::releaseRxStreamSocket()
{
}

// creates and connects the tx socket
int32_t HiggsDriver::connectTxStreamSocket()
{
    tx_address.sin_family = AF_INET;
    tx_address.sin_port = htons(GET_TX_DATA_PORT());
    tx_address.sin_addr.s_addr = inet_addr(GET_HIGGS_IP().c_str());

    std::cout << "connectTxStreamSocket() " << GET_TX_DATA_PORT() << " " << GET_HIGGS_IP() << std::endl;

    tx_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(tx_sock_fd < 0){
        std::cout << "ERROR: Socket file descriptor was not allocated" << std::endl;
        return -1;
    }
   
    tx_address_len = sizeof(tx_address);

    return 0;
}

int32_t HiggsDriver::connectGrcUdpSocket()
{
    grc_udp_address.sin_family = AF_INET;
    grc_udp_address.sin_port = htons(atoi(GET_GNURADIO_UDP_PORT().c_str()));
    grc_udp_address.sin_addr.s_addr = inet_addr(GET_GNURADIO_UDP_IP().c_str());

    grc_udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    auto flags = fcntl(grc_udp_sock_fd, F_GETFL);
    if (flags < 0) {
        cout << "udp socket F_GETFL failed" << endl;
        return flags;
    }

    flags |= O_NONBLOCK;

    if (fcntl(grc_udp_sock_fd, F_SETFL, flags) < 0) {
        cout << "udp socket F_SETFL failed" << endl;
        return -1;
    }

    grc_udp_address_len = sizeof(grc_udp_address);
    return 0;
}

int32_t HiggsDriver::connectGrcUdpSocket2()
{
    grc_udp_address_2.sin_family = AF_INET;
    grc_udp_address_2.sin_port = htons(atoi(GET_GNURADIO_UDP_PORT2().c_str()));
    grc_udp_address_2.sin_addr.s_addr = inet_addr(GET_GNURADIO_UDP_IP().c_str());

    grc_udp_sock_fd_2 = socket(AF_INET, SOCK_DGRAM, 0);

    auto flags = fcntl(grc_udp_sock_fd_2, F_GETFL);
    if (flags < 0) {
        cout << "udp socket F_GETFL failed" << endl;
        return flags;
    }

    flags |= O_NONBLOCK;

    if (fcntl(grc_udp_sock_fd_2, F_SETFL, flags) < 0) {
        cout << "udp socket F_SETFL failed" << endl;
        return -1;
    }

    grc_udp_address_len_2 = sizeof(grc_udp_address_2);
    return 0;
}




// thread safe way to send packets to higgs data socket
// will chunk and do small sleeps after packets

void HiggsDriver::sendToHiggs(const unsigned char* const data, const size_t len, const uint32_t later) {
    if( print_send_to_higgs ) {
        cout << "sendToHiggs() got size " << len << "\n";
    }

    bufferevent_write(flushHiggs->for_higgs_bev->in, data, len);

    // int ret = 0;
    // return ret;
}

void HiggsDriver::sendToHiggsLowPriority(const unsigned char* const data, const size_t len) {
    if( print_send_to_higgs ) {
        cout << "sendToHiggsLow() got size " << len << "\n";
    }

    // consider this chagne of zeros
    constexpr bool flag = false;// = !GET_ALLOW_CS20_FB_CRASH();

    std::vector<uint32_t> zrs;
    unsigned char* start;
    if(flag) {
        zrs.resize(32,0);
        start = (unsigned char*)zrs.data();
        bufferevent_write(flushHiggs->for_higgs_via_zmq_bev->in, start, zrs.size()*4);
    }

    bufferevent_write(flushHiggs->for_higgs_via_zmq_bev->in, data, len);

    if(flag) {
        bufferevent_write(flushHiggs->for_higgs_via_zmq_bev->in, start, zrs.size()*4);
    }
}

// skips the bef and goes straight for the socket
// int HiggsDriver::sendToHiggs2(unsigned char* data, size_t len) {
//     int sendto_ret = -1;
//     {
//         // grab the lock
//         std::lock_guard<std::mutex> lock(tx_sock_mutex);
        
//         // max packet size
//         int const chunk = 367*4;

//         size_t sent = 0;
//         // floor of int divide
//         size_t floor_chunks = len/chunk;

//         // loop that +1
//         for(size_t i = 0; i < floor_chunks+1; i++ ) {
//             unsigned char* start = data + sent;

//             // calculate remaining bytes
//             int remaining = (int)len - (int)sent;
//             // this chunk is the lesser of:
//             int this_len = std::min(chunk, remaining);

//             // if exact even division, bail
//             if(this_len <= 0) {
//                 break;
//             }

//             // for( size_t j = 0; j < this_len; j++) {
//             //     std::cout << HEX_STRING((int)start[j]) << ",";
//             // }
//             // std::cout << std::endl;

//             // run with values
//             sendto_ret = sendto(tx_sock_fd, start, this_len, 0, (struct sockaddr *)&tx_address, tx_address_len);
//             usleep(10);
//             sent += this_len;
//         }


//     }
//     return sendto_ret;
// }

// automatically chunks into packets and does a small sleep
void HiggsDriver::sendToHiggs(const std::vector<uint32_t> &vec) {
    const unsigned char* const raw_data = (const unsigned char* const)vec.data();
    const size_t len4x = vec.size() * 4;
    sendToHiggs(raw_data, len4x);
}




/**
 * Creates a standard vector of size 2^16 where
 * vector[index] = (float) ((int16_t) index)/(2^15 - 1). This vector serves as
 * a lookup table when converting a signed 16 bit integer to float
 * representation scaled by a constant.
 *
 */
// void HiggsDriver::create_sample_lookup() {
//     float converted_value;
//     for (unsigned int i = 0; i < 0x10000; i++) {
//         converted_value = (float) ((int16_t) i)/FIXED_POINT_MAX_VAL;
//         _sample_value.push_back(converted_value);
//     }
// }

bool HiggsDriver::flushHiggsReady() const {
    return _flush_higgs_ready;
}

bool HiggsDriver::dspThreadReady() const {
    return _dsp_thread_ready && dspEvents != NULL;
}

bool HiggsDriver::txScheduleReady() const {
    return _tx_schedule_ready;// && txSchedule != NULL;
}

bool HiggsDriver::demodThreadReady() const {
    return _demod_thread_ready;
}


void HiggsDriver::registerRingbusCallback(const ringbus_cb_t f) {
    ringCallback = f;
}

// pass a mask with only the upper byte set aka 0xff000000
void HiggsDriver::ringbusCallbackCatchType(const uint32_t mask) {
    // lock this entire function
    // could release lock before printing to be extra good boy
    std::lock_guard<std::mutex> lock(js_ringbus_mutex);

    const auto found = std::find(callbackShouldCatch.begin(), callbackShouldCatch.end(), mask);
    if( found == callbackShouldCatch.end() ) {
        callbackShouldCatch.push_back(mask);
    } else {
        cout << "ringbusCallbackCatchType() ignoring duplicate mask: " << HEX32_STRING(mask) << "\n";
    }
}

void HiggsDriver::registerEventCallback(const event_cb_t f) {
    eventCallback = f;
}
