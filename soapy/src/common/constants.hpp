#pragma once
// Constants etc
//
// Things that are constants, but are not settings, or cannot be changed
//
// #define PI 3.14159265358979323846

#define ZMQ_TAG_RING "?"
#define ZMQ_TAG_ALL "@"
#define ZMQ_TAG_PEER_START 'A'
#define ZMQ_TAG_STATUS "~"


// used when not specified by env/cmdln
#define DEFAULT_JSON_PATH "../init.json"
#define DEFAULT_JSON_PATH_ARBITER "../arb.json"


#define THE_TUN_NAME "tun1"


// #define PRINT_LIBEV_EVENT_TIMES

//
// Things we NEVER change
//

// in uint32_t words
#define SFO_CFO_PACK_SIZE (5+2)


// in uint32_t words
#define ALL_SC_BUFFER_SIZE (1+1024)


#define SEND_TO_HIGGS_PACKET_SIZE (364*4)

#define GNURADIO_UDP_SC_WIDTH (64)
#define GNURADIO_UDP_PACKET_COUNT (128)
#define GNURADIO_UDP_SIZE ((GNURADIO_UDP_SC_WIDTH*4)*GNURADIO_UDP_PACKET_COUNT)

// in bytes, driver will assert if gets smaller packets
#define UDP_PACKET_DATA_SIZE (1472)

#define FS 32768000

#define SFO_ADJ (200/2)

#define CFO_ADJ 4074.36654315252

// it is ILLEGAL to have math in a macro without ()
// do not ever have a naked function such as 7+1, always put (7+1)
#define STO_ADJ_SHIFT (8)

#define DATA_TONE_NUM 32

// index into the data array
#define DATA_SUBCARRIER_INDEX (31)


// subcarrierForBufferIndex(31) is 1007
// 1024 - 1007 = 17
// #define DATA_SUBCARRIER_TX_ARRAY_INDEX (17)

// used for BEV size as well as argument to RadioDemodTDMA
#define SUBCARRIER_CHUNK (1024)

#define COARSE_TONE_INDEX 63

// how many "full" tones are we sending from mover/mapper on 21
#define NUMBER_FULL_TONES 64


#define DEMOD_BEV_SIZE_WORDS 65


#define EVM_TONE_NUM (32)


// FIXME, pull from AirPacket or something
// 8  for qpsk,128
// 16 for qam16,128
// 40 for qam16,320
// 20 for qpsk,320
// 40 for qpsk,640lin
// 80 for qam16,640lin
// 32 for qpsk,512lin
#define SLICED_DATA_WORD_SIZE (20)


// how many bytes do we take from the feedback bus into memory
#define SLICED_DATA_BEV_WRITE_SIZE_WORDS (SLICED_DATA_WORD_SIZE+1)
// how many of the previous do we read at a time
#define SLICED_DATA_CHUNK (128)


#define RINGBUS_DISPATCH_BATCH_LIMIT (4)

// #define OVERWRITE_GNU_WITH_CFO

// #define OVERWRITE_FINE_CFO_WITH_STREAM
// meaning less if OVERWRITE_FINE_CFO_WITH_STREAM is not set
// #define OFCWF_SUBCARRIER (62)

//
// Things we change faster
//


// Schedule , epoc, timeslot related stuff
// __SPS_R1 = (31.25E6 / (1024+256))
// __SPS_R1 = (512.0*50.0)
// calculated using (__SPS_R1/__SPS_R1)
#define SCHEDULES_PER_SECOND (1.048576)
#define SCHEDULE_FRAMES_PER_SECOND (24414.0625)

// used together with GET_RUNTIME_SLOWDOWN()
// #define USE_DYNAMIC_TIMERS






// #define DEBUG_PRINT_LOOPS
// #define DEBUG_PRINT_TIE_TICKLE
// #define DEBUT_PRINT_JSON_OUTPUT
// #define DEBUG_PRINT_DASH_UPDATES
// #define DEBUG_PRINT_THREAD_INFO





/// See HiggsSettingsDuckType
///
#define PATH_FOR(xxx) \
([]() { \
    HiggsSettingsDuckType duck; \
    HiggsSettingsDuckType* settings = &duck; \
    auto ret = xxx ; \
    return ret; \
})()



///
/// There are live settings, a little bit costly to call, but should be ok
///
/// Here you can put a chain of arguments, but the end user only sees a global define
/// hoever with each call, the global settings will be crawled
/// these TAKE A LOCK
///
/// Note due to hacked circumstance, it's better to NOT have outside parens
///   1. they add nothing assuming these are all uniform below
///   2. they prevent another thread or anything without this from accessing
///       ie:    soapy->(settings->get.....)    is illegal
///

#define GET_BLOCK_FSM() (settings->get<bool>("dsp", "block_fsm"))

#define GET_PEER_T0() (settings->get<uint32_t>("exp", "peers", "t0"))
#define GET_PEER_T1() (settings->get<uint32_t>("exp", "peers", "t1"))
#define GET_PEER_T2() (settings->get<uint32_t>("exp", "peers", "t2"))
#define GET_PEER_T3() (settings->get<uint32_t>("exp", "peers", "t3"))

#define GET_PEER_0() GET_PEER_T0()
#define GET_PEER_1() GET_PEER_T1()
#define GET_PEER_2() GET_PEER_T2()
#define GET_PEER_3() GET_PEER_T3()


#define GET_PEER_RX() (settings->get<uint32_t>("exp", "rx_master_peer"))
#define GET_OUR_ID() (settings->get<uint32_t>("exp", "our", "id"))
#define GET_PEER_ARBITER() (settings->get<uint32_t>("exp", "peers", "arb"))

#define GET_RADIO_ROLE() settings->get<std::string>("exp", "our", "role")

#define GET_DISABLE_OPEN_ONCE()  (settings->get<bool>("demod", "disable_open_once"))

// higgs_udp
#define GET_HIGGS_IP() (settings->get<std::string>("net", "higgs_udp", "higgs_ip"))
#define GET_PC_IP() (settings->get<std::string>("net", "higgs_udp", "pc_ip"))
#define GET_RX_CMD_PORT() (settings->get<uint32_t>("net", "higgs_udp", "rx_cmd"))
#define GET_TX_CMD_PORT() (settings->get<uint32_t>("net", "higgs_udp", "tx_cmd"))
#define GET_TX_DATA_PORT() (settings->get<uint32_t>("net", "higgs_udp", "tx_data"))
#define GET_RX_DATA_PORT() (settings->get<uint32_t>("net", "higgs_udp", "rx_data"))
#define GET_ZMQ_LISTEN_PORT() (settings->get<uint32_t>("net", "zmq", "listen"))
#define GET_ZMQ_PUBLISH_INBOUND() (settings->getDefault<bool>(false, "net", "zmq", "publish", "inbound"))
#define GET_ZMQ_PUBLISH_PORT() (settings->getDefault<uint32_t>(10004, "net", "zmq", "publish", "port"))


// string ip
#define GET_DASHBOARD_IP() (settings->get<std::string>("net", "dashboard", "ip"))
#define GET_DISCOVERY_PORT0() (settings->get<uint32_t>("net", "network", "discovery_port0"))
#define GET_DISCOVERY_PORT1() (settings->get<uint32_t>("net", "network", "discovery_port1"))
#define GET_DISCOVERY_IP() (settings->get<std::string>("net", "network", "discovery_ip"))
#define GET_DISCOVERY_PC_IP() (settings->get<std::string>("net", "network", "pc_ip"))
#define GET_PATCH_DISCOVERY_FOR_NAT() (settings->getDefault<bool>(false, "net", "network", "patch_discovery_for_nat"))
// string port (not int)
#define GET_DASHBOARD_PORT() (settings->get<std::string>("net", "dashboard", "port"))
#define GET_DASHBOARD_DO_CONNECT() (settings->get<bool>("net", "dashboard", "do_connect"))

#define GET_GNURADIO_UDP_IP()         settings->get<std::string>("net", "gnuradio_udp", "ip")
#define GET_GNURADIO_UDP_PORT()       settings->get<std::string>("net", "gnuradio_udp", "port")
#define GET_GNURADIO_UDP_PORT2()      settings->get<std::string>("net", "gnuradio_udp", "port_2")
#define GET_GNURADIO_UDP_SEND_SINGLE()     settings->getDefault<bool>(false, "net", "gnuradio_udp", "send_single_sc")
#define GET_GNURADIO_UDP_DO_CONNECT() settings->get<bool>("net", "gnuradio_udp", "do_connect")
#define GET_GNURADIO_UDP_DO_CONNECT2() settings->getDefault<bool>(false, "net", "gnuradio_udp", "do_connect_2")
#define GET_GNURADIO_UDP_DOWNSAMPLE() settings->getDefault<unsigned>(0, "net", "gnuradio_udp", "downsample")

#define GET_CREATE_TUN() settings->getWithDefault<bool>(false, "net", "tun", "create_up_and_ip")
#define GET_CREATE_TUN_IP() settings->get<std::string>("net", "tun", "ip")
#define GET_CREATE_TUN_USER() settings->get<std::string>("net", "tun", "user")

#define GET_PRINT_PEER_RB() (settings->get<bool>("global", "print", "PRINT_PEER_RB"))
#define GET_PRINT_TDMA_SC() settings->get<uint32_t>("global", "print", "PRINT_TDMA_SC")
#define GET_PRINT_SFO_CFO_ESTIMATES() settings->get<bool>("global", "print", "PRINT_SFO_CFO_ESTIMATES")
#define GET_PRINT_TX_BUFFER_FILL() settings->get<bool>("global", "print", "PRINT_TX_BUFFER_FILL")
#define GET_PRINT_RECEIVED_ZMQ() settings->getDefault<bool>(false, "global", "print", "PRINT_RECEIVED_ZMQ")
#define GET_PRINT_OUTGOING_ZMQ() settings->getDefault<bool>(false, "global", "print", "PRINT_OUTGOING_ZMQ")
#define GET_PRINT_TX_FLUSH_LOW_PRI() settings->getDefault<bool>(false, "global", "print", "PRINT_TX_FLUSH_LOW_PRI")
#define GET_PRINT_EVBUF_SOCKET_BURST() settings->getDefault<bool>(false, "global", "print", "PRINT_EVBUF_SOCKET_BURST")



#define GET_ZMQ_NAME() (settings->get<std::string>("exp", "our", "zmq_name"))
#define GET_POLL_FOR_CPU_LOAD() settings->get<bool>("exp", "poll_rx_for_cpu_load")

#define GET_FSM_SKIP_R1() settings->get<bool>("exp", "fsm_skip_r1")

#define DSP_WAIT_FOR_PEERS() (settings->get<uint32_t>("exp", "DSP_WAIT_FOR_PEERS"))

#define GET_SFO_STATE_THRESH() settings->get<double>("dsp", "sfo", "state_thresh")
#define GET_SFO_ESTIMATE_WAIT() settings->get<unsigned>("dsp", "sfo", "sfo_estimate_wait")

#define GET_DEBUG_LOOPBACK_FINE_SYNC_COUNTER() settings->getWithDefault<bool>(false, "exp", "debug_loopback_fine_sync_counter")
#define GET_DEBUG_COARSE_SYNC_ON_RX() settings->getDefault<bool>(false, "exp", "debug_coarse_sync_on_rx")
#define GET_DEBUG_EQ_BEFORE_SCHEDULE() settings->get<bool>("exp", "debug_eq_before_schedule") 
#define GET_DEBUG_TX_EPOC() settings->get<bool>("exp", "debug_tx_epoc")
#define GET_DEBUG_TX_EPOC_AND_ZMQ() settings->get<bool>("exp", "debug_tx_epoc_and_zmq")
#define GET_DEBUG_TX_LOCAL_EPOC() settings->get<bool>("exp", "debug_tx_local_epoc") 
#define GET_DEBUG_TX_FILL_LEVEL() settings->get<bool>("exp", "debug_tx_fill_level") 
#define GET_DEBUG_RESET_FB_ONLY() settings->get<bool>("exp", "debug_tx_reset_fb_only") 
#define GET_FLOOD_PING_TEST() settings->get<bool>("exp", "flood_ping_test") 
#define GET_FLOOD_RB_TEST() settings->get<bool>("exp", "flood_rb_test") 
#define GET_SKIP_CHECK_ATTACHED_FB() settings->get<bool>("exp", "skip_check_attached_fb") 
#define GET_DASHDIE_INTO_MOCK() settings->getDefault<bool>(false, "exp", "dashtie_into_mock") 
#define GET_AGGREGATE_ATTACHED_RB() settings->getDefault<bool>(false, "exp", "aggregate_attached_rb") 
#define GET_SKIP_CHECK_FB_DATARATE() settings->getDefault<bool>(false, "exp", "skip_check_fb_datarate") 

//! use with debug_eq_before_schedule. makes radio stay in sending first row of canned data
#define GET_TDMA_FORCE_FIRST_ROW() settings->get<bool>("exp", "tdma_force_first_row")

#define GET_INTEGRATION_TEST_RUN_TIME() settings->get<uint32_t>("exp", "integration_test", "run_time")
#define GET_INTEGRATION_TEST_ENABLED() settings->get<bool>("exp", "integration_test", "test_enabled")
#define GET_RETRY_RESET_FB_BUS_AFTER_DOWN() settings->get<bool>("exp", "retry_reset_fb_bus_after_down")
#define GET_ALLOW_CS20_FB_CRASH() settings->getDefault<bool>(false, "exp", "allow_cs20_fb_crash")

#define GET_SFO_FITTING_THRESHOLD() settings->get<int>("dsp", "sfo", "FITTING_THRESHOLD")

#define GET_SFO_BG_UPPER() settings->get<double>("dsp", "sfo", "background_tolerance", "upper")
#define GET_SFO_BG_LOWER() settings->get<double>("dsp", "sfo", "background_tolerance", "lower")

#define GET_CFO_BG_UPPER() settings->get<double>("dsp", "cfo", "background_tolerance", "upper")
#define GET_CFO_BG_LOWER() settings->get<double>("dsp", "cfo", "background_tolerance", "lower")

#define GET_SFO_MAX_STEP() settings->getDefault<double>(0.1, "dsp", "sfo", "max_step")

#define GET_SFO_RESTIMATE_TOL() settings->get<int>("dsp", "sfo", "restimate_tolerance")

#define GET_CFO_STATE_THRESHOLD() settings->get<double>("dsp", "cfo", "state_threshold")

#define GET_CFO_SYMBOL_PULL_RATIO() settings->get<unsigned>("dsp", "cfo", "pull_ratio")

#define GET_DATA_SEND_PACKET_WHEN_IDLE() settings->get<bool>("data", "send_packet_when_idle")

#define GET_CFO_SYMBOL_NUM() settings->get<unsigned>("dsp", "cfo", "symbol_num")
#define GET_SFO_SYMBOL_NUM() settings->get<unsigned>("dsp", "sfo", "symbol_num")

#define GET_EQ_USE_FILTER() settings->get<bool>("dsp", "eq", "use_filter")
#define GET_EQ_UPDATE_SECONDS() settings->get<double>("dsp", "eq", "update_seconds")
#define GET_EQ_MAGNITUDE_DIVISOR() settings->getDefault<double>(1.0, "dsp", "eq", "magnitude_divisor")
#define GET_EQ_IIR_GAIN() settings->getDefault<int32_t>(65536, "dsp", "eq", "iir_gain")



#define GET_COARSE_SYNC_TWEAK() settings->get<int>("dsp", "coarse", "tweak")
#define GET_DSP_LONG_RANGE() settings->getDefault<bool>(false, "dsp", "long_range", "enable")

#define GET_STO_MARGIN()  settings->get<int>("dsp", "sto", "margin")
#define GET_STO_DISABLE_POSITIVE()  settings->getDefault<bool>(false, "dsp", "sto", "disable_positive_adjustment")

#define GET_ADC_DSA_GAIN() settings->get<int>("adc", "gain", "dsa")
#define GET_ADC_DSA_GAIN_DB() settings->get<int>("adc", "gain", "dsa_db")

#define GET_TEST_COARSE_MODE() settings->get<bool>("exp", "test_coarse_mode")
#define GET_TEST_COARSE_MODE_AND_ADJUST() settings->get<bool>("exp", "test_coarse_mode_and_adjust")
#define GET_PRINT_COOKED_BER() settings->get<bool>("global", "print", "PRINT_COOKED_BER")
#define GET_PRINT_RE_NEW_MESSAGE() settings->get<bool>("global", "print", "PRINT_RE_NEW_MESSAGE")
#define GET_PRINT_RE_FINE_SCHEDULE() settings->get<bool>("global", "print", "PRINT_RE_FINE_SCHEDULE")
#define GET_PRINT_VERIFY_HASH() settings->get<bool>("global", "print", "PRINT_VERIFY_HASH")
#define GET_PRINT_DISPATCH_MOCK_RPC() settings->getDefault<bool>(false, "global", "print", "PRINT_DISPATCH_MOCK_RPC")


#define GET_ONLY_UPDATE_CHANNEL_ANGLE_R0() settings->get<bool>("dashboard", "only_update_channel_angle_r0")
#define GET_ONLY_UPDATE_RATE_DIVISOR() settings->get<int>("dashboard", "channel_angle_rate_divisor")


#define GET_SFO_INIT_T0() settings->getDefault<double>(0.0, "dsp", "sfo", "peers", "t0")
#define GET_SFO_INIT_T1() settings->getDefault<double>(0.0, "dsp", "sfo", "peers", "t1")
#define GET_SFO_INIT_ATTACHED() settings->getDefault<double>(0.0, "dsp", "sfo", "peers", "attached")
#define GET_CFO_INIT_T0() settings->getDefault<double>(0.0, "dsp", "cfo", "peers", "t0")
#define GET_CFO_INIT_T1() settings->getDefault<double>(0.0, "dsp", "cfo", "peers", "t1")
#define GET_CFO_INIT_ATTACHED() settings->getDefault<double>(0.0, "dsp", "cfo", "peers", "attached")

#define GET_CFO_RESET_ZERO() settings->getDefault<bool>(true, "dsp", "cfo", "reset_zero")
#define GET_SFO_RESET_ZERO() settings->getDefault<bool>(true, "dsp", "cfo", "reset_zero")
#define GET_CFO_APPLY_INITIAL() settings->get<bool>("dsp", "cfo", "apply_initial")
#define GET_SFO_APPLY_INITIAL() settings->get<bool>("dsp", "sfo", "apply_initial")
#define GET_CFO_ADJUST_USING_RESIDUE() settings->getDefault<bool>(false, "dsp", "cfo", "adjust_using_residue")
#define GET_STO_SKIP() settings->getDefault<bool>(false, "dsp", "sto", "skip")

#define GET_FILE_DUMP_SLICED_DATA_ENABLED() settings->get<bool>("global", "file_dump", "sliced_data", "enabled")
#define GET_FILE_DUMP_SLICED_DATA_PATH() settings->get<std::string>("global", "file_dump", "sliced_data", "path")

#define GET_DATA_HASH_VALIDATE_ALL() settings->get<bool>("data", "hash", "validate_all")

#define GET_RESIDUE_DUMP_ENABLED() settings->getDefault<bool>(false, "dsp", "residue", "dump", "enabled")
#define GET_RESIDUE_DUMP_FILENAME() settings->getDefault<std::string>("save_residue.txt", "dsp", "residue", "dump", "filename")
#define GET_RESIDUE_RATE_DIVISOR() settings->getDefault<uint32_t>(2, "dsp", "residue", "rate_divisor")

#define GET_RESIDUE_UPSTREAM_DUMP_ENABLED() settings->getDefault<bool>(                         false, "dsp", "residue", "dump_upstream", "enabled")
#define GET_RESIDUE_UPSTREAM_DUMP_FILENAME() settings->getDefault<std::string>("residue_upstream.txt", "dsp", "residue", "dump_upstream", "filename")


#define GET_RUNTIME_SKIP_TDMA_R0() settings->getDefault<bool>(false, "runtime", "r0", "skip_tdma")
#define GET_STALL_BEFORE_R0_FINE_SYNC() settings->getDefault<bool>(false, "exp", "stall_before_r0_fine_sync")

#define GET_RUNTIME_DATA_TO_JS() settings->getDefault<bool>(false, "runtime", "data", "to_js")

#define GET_RUNTIME_SLOWDOWN() settings->getDefault<double>(1.0, "runtime", "slowdown") // 1.0 means realtime 2.0 means two times slower

#define GET_RE_FSM_TYPE()  settings->getDefault<std::string>("c", "fsm", "radio_estimate", "type")

#define GET_RETRY_COARSE() settings->getDefault<bool>(false, "exp", "retry", "coarse", "enable")
#define GET_RETRY_COARSE_RAND() settings->getDefault<bool>(false, "exp", "retry", "coarse", "randomize")

#define GET_COARSE_SYNC_NUM() settings->getDefault<uint32_t>(50, "dsp", "coarse", "coarse_sync_ofdm_num")

#define GET_RINGBUS_HASH_CHECK_ENABLE() settings->getDefault<bool>(false, "ringbus", "hash_check", "enable")
#define GET_RINGBUS_HASH_CHECK_PRINT() settings->getDefault<bool>(false, "ringbus", "hash_check", "print")

#define GET_SFO_MAX_STEP() settings->getDefault<double>(0.1, "dsp", "sfo", "max_step")


#define GET_STO_ADJUST_USING_EQ() settings->getDefault<bool>(false, "dsp", "sto", "adjust_using_eq", "enable")
#define GET_STO_ADJUST_USING_EQ_THRESHOLD() settings->getDefault<int>(1, "dsp", "sto", "adjust_using_eq", "threshold")
#define GET_STO_ADJUST_USING_EQ_HALF_SC() settings->getDefault<int>(320, "dsp", "sto", "adjust_using_eq", "half_sc")

#define GET_RX_TUN_TO_ARB() settings->getDefault<bool>(false, "exp", "rx_tun_to_arb")


#define GET_SFO_ADJUST_USING_EQ() settings->getDefault<bool>(false, "dsp", "sfo", "adjust_using_eq", "enable")
#define GET_SFO_ADJUST_USING_EQ_SECONDS() settings->getDefault<double>(40.0, "dsp", "sfo", "adjust_using_eq", "seconds")
#define GET_SFO_ADJUST_USING_EQ_TREND() settings->getDefault<unsigned>(2, "dsp", "sfo", "adjust_using_eq", "trend")
#define GET_SFO_ADJUST_USING_EQ_FACTOR() settings->getDefault<double>(0.9, "dsp", "sfo", "adjust_using_eq", "factor")

#define GET_FSM_ARBITER_DATA() settings->getDefault<bool>(true, "exp", "fsm_arbiter_data")

#define GET_FEC_LENGTH() settings->getDefault<uint32_t>(48, "data","coding","fec_length")
#define GET_INTERLEAVER() settings->getDefault<uint32_t>(0, "data", "coding","interleaver")

#define GET_DEBUG_STALL() settings->getDefault<bool>(false, "exp", "debug_stall")

#define GET_BARRELSHIFT_TX_SET() settings->getDefault<bool>(false, "dsp", "barrelshift", "tx", "set")

#define GET_RUNTIME_SELF_SYNC_DONE() settings->getDefault<bool>(false, "runtime", "self_sync", "done")
