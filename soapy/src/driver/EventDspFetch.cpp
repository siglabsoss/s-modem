#include "EventDsp.hpp"
#include "driver/AttachedEstimate.hpp"

using namespace std;

// 0 illegal
// 1 double
// 2 uint32_t
// 3 pair<uint32_t,uint32_t>

void EventDsp::zmqFetch(const std::vector<uint32_t>& data_vec) {
    if( data_vec.size() < 4 ) {
        cout << "zmqFetch() called (over zmq) with too few arguments " << data_vec.size() << "\n";
        return;
    }

    const uint32_t from = data_vec[0];
    const uint32_t type = data_vec[1];
    const uint32_t index = data_vec[2];
    const uint32_t seq = data_vec[3];

    bool valid = true;

    const double* p_double = 0;
    const uint32_t* p_uint32_t = 0;
    const std::pair<uint32_t,uint32_t>* p_pair_uint32_t_uint32_t = 0;

    const double test0 = 42.42;

    std::pair<uint32_t,uint32_t> working_pair;


    switch(index) {
        case 0:
            p_double = &test0;
            break;
        case 1:
            p_uint32_t = &times_tried_attached_feedback_bus;
            break;
        case 2:
            p_uint32_t = &(soapy->this_node.id);
            break;
        case 3:
            {
                epoc_frame_t now_est;
                int error;
                if( attached ) {
                    now_est = attached->predictScheduleFrame(error, false, false);
                    working_pair = {now_est.epoc,now_est.frame};
                } else {
                    cout << "zmqFetch() attached was NULL\n";
                    return;
                }
                p_pair_uint32_t_uint32_t = &working_pair;
            }
            break;
        default:
            valid = false;
            break;
    }

    if(!valid) {
        cout << "zmqFetch() found message for non supported index " << index << "\n";
        return;
    }

    uint32_t type_for_index = 0;
    
    if( p_double != 0 ) {
        type_for_index = 1;
    } else if (p_uint32_t != 0) {
        type_for_index = 2;
    } else if (p_pair_uint32_t_uint32_t != 0) {
        type_for_index = 3;
    } else {
        // if we get here it means the switch() above set valid to true, but we didn't set any
        // pointers
        cout << "zmqFetch() did not set any pointers.  There is a bug with bool valid\n";
        return;
    }

    if( type_for_index != type ) {
        cout << "zmqFetch() got request for type " << type << " but the index " << index << " is actually type " << type_for_index << "\n";
        return;
    }

    // by here command is indeed valid

    cout << "zmqFetch(" << from << "," << type << "," << index << "," << seq << ")\n";

    // for( const auto w : data_vec ) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    switch(type_for_index) {
        case 1:
            zmqFetchReply_double(p_double, from, type_for_index, index, seq);
            break;
        case 2:
            zmqFetchReply_uint32_t(p_uint32_t, from, type_for_index, index, seq);
            break;
        case 3:
            zmqFetchReply_pair_uint32_t_uint32_t(p_pair_uint32_t_uint32_t, from, type_for_index, index, seq);
            break;
        default:
            break;
    }

}

void EventDsp::zmqFetchReply_double(const double* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq) {

    std::vector<uint32_t> payload;
    payload.push_back(soapy->this_node.id); // word 0: from
    payload.push_back(type);                // word 1: type
    payload.push_back(index);               // word 2: index
    payload.push_back(seq);                 // word 3: seq

    // union {
    //     double d;
    //     uint64_t i;
    // } conv = {*p}; // member 'd' set to value of 'number'.

    union {
        double d;
        struct {
            uint32_t w0;
            uint32_t w1;
        } words;
    } conv = {*p}; // member 'd' set

    payload.push_back(conv.words.w0);
    payload.push_back(conv.words.w1);

    sendVectorTypeToPartnerPC(from, FEEDBACK_VEC_FETCH_REPLY, payload);
}

void EventDsp::zmqFetchReply_uint32_t(const uint32_t* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq) {

    std::vector<uint32_t> payload;
    payload.push_back(soapy->this_node.id); // word 0: from
    payload.push_back(type);                // word 1: type
    payload.push_back(index);               // word 2: index
    payload.push_back(seq);                 // word 3: seq

    payload.push_back(*p);

    sendVectorTypeToPartnerPC(from, FEEDBACK_VEC_FETCH_REPLY, payload);
}

void EventDsp::zmqFetchReply_pair_uint32_t_uint32_t(const std::pair<uint32_t,uint32_t>* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq) {

    std::vector<uint32_t> payload;
    payload.push_back(soapy->this_node.id); // word 0: from
    payload.push_back(type);                // word 1: type
    payload.push_back(index);               // word 2: index
    payload.push_back(seq);                 // word 3: seq

    uint32_t w0;
    uint32_t w1;
    std::tie(w0,w1) = *p;

    payload.push_back(w0);
    payload.push_back(w1);

    sendVectorTypeToPartnerPC(from, FEEDBACK_VEC_FETCH_REPLY, payload);
}

std::tuple<uint32_t,uint64_t,std::vector<uint32_t>> EventDsp::zmqFetchFromPartner_shared(const size_t peer, const uint32_t index, const uint32_t type) {
    const uint32_t seq = fetch_seq;
    fetch_seq++;
    const auto now = std::chrono::steady_clock::now();

    const uint64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - booted
    ).count();

    std::vector<uint32_t> payload;
    payload.push_back(soapy->this_node.id); // word 0: from
    payload.push_back(type);                // word 1: type
    payload.push_back(index);               // word 2: index
    payload.push_back(seq);                 // word 3: seq

    return std::tuple<uint32_t,uint64_t,std::vector<uint32_t>>(seq, elapsed_us, payload);
}

void EventDsp::zmqFetchFromPartner_double(const size_t peer, const uint32_t index, const zmq_fetch_double_t cb) {
    uint32_t seq;
    uint64_t elapsed_us;
    std::vector<uint32_t> payload;
    std::tie(seq, elapsed_us, payload) = zmqFetchFromPartner_shared(peer, index, 1);

    pending_fetch_double.emplace_back(seq, elapsed_us, cb);

    sendVectorTypeToPartnerPC(peer, FEEDBACK_VEC_FETCH_REQUEST, payload);
}

void EventDsp::zmqFetchFromPartner_uint32_t(const size_t peer, const uint32_t index, const zmq_fetch_uint32_t_t cb) {
    uint32_t seq;
    uint64_t elapsed_us;
    std::vector<uint32_t> payload;
    std::tie(seq, elapsed_us, payload) = zmqFetchFromPartner_shared(peer, index, 2);

    pending_fetch_uint32_t.emplace_back(seq, elapsed_us, cb);

    sendVectorTypeToPartnerPC(peer, FEEDBACK_VEC_FETCH_REQUEST, payload);
}

void EventDsp::zmqFetchFromPartner_pair_uint32_t_uint32_t(const size_t peer, const uint32_t index, const zmq_fetch_pair_uint32_t_uint32_t_t cb) {
    uint32_t seq;
    uint64_t elapsed_us;
    std::vector<uint32_t> payload;
    std::tie(seq, elapsed_us, payload) = zmqFetchFromPartner_shared(peer, index, 3);

    pending_fetch_pair_uint32_t_uint32_t.emplace_back(seq, elapsed_us, cb);

    sendVectorTypeToPartnerPC(peer, FEEDBACK_VEC_FETCH_REQUEST, payload);
}


template<typename T>
static void _dispatch(const uint32_t seq, const T& val, std::vector<std::tuple<uint32_t,uint64_t,std::function<void(const bool, const T)>>>& vec) {

    for(auto it = vec.begin(); it != vec.end(); ++it) {
        uint32_t vec_seq;
        uint64_t vec_elapsed_us;
        std::function<void(const bool, const T)> vec_cb;
        std::tie(vec_seq, vec_elapsed_us, vec_cb) = *it;

        if( vec_seq == seq ) {
            // erase first
            vec.erase(it);

            // call callback
            vec_cb(true, val);
            break;
        }
    }
}


void EventDsp::zmqFetchDispatchReply(const std::vector<uint32_t>& data_vec) {
    // cout << "zmqFetchDispatchReply()\n";

    // for( const auto w : data_vec ) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    if( data_vec.size() < 4 ) {
        cout << "zmqFetchDispatchReply() called (over zmq) with too few arguments " << data_vec.size() << "\n";
        return;
    }

    const uint32_t from = data_vec[0];
    const uint32_t type = data_vec[1];
    const uint32_t index = data_vec[2];
    const uint32_t seq = data_vec[3];

    (void)from;
    (void)index;

    if( type == 1 ) {
        if( data_vec.size() < 6 ) {
            cout << "zmqFetchDispatchReply() called for type " << type << " but only had length " << data_vec.size() << "\n";
            return;
        }

        union {
            double d;
            struct {
                uint32_t w0;
                uint32_t w1;
            } words;
        } conv;

        conv.words.w0 = data_vec[4];
        conv.words.w1 = data_vec[5];

        double val = conv.d;

        _dispatch<double>(seq, val, pending_fetch_double);
        
    } else if( type == 2 ) {
        if( data_vec.size() < 5 ) {
            cout << "zmqFetchDispatchReply() called for type " << type << " but only had length " << data_vec.size() << "\n";
            return;
        }

        _dispatch<uint32_t>(seq, data_vec[4], pending_fetch_uint32_t);
    } else if( type == 3 ) {
        if( data_vec.size() < 6 ) {
            cout << "zmqFetchDispatchReply() called for type " << type << " but only had length " << data_vec.size() << "\n";
            return;
        }

        std::pair<uint32_t,uint32_t> pair = {data_vec[4], data_vec[5]};

        _dispatch<std::pair<uint32_t,uint32_t>>(seq, pair, pending_fetch_pair_uint32_t_uint32_t);
    }

}








// zmqFetchFromPartner_double(1, 0, [](const bool ok, const double val){
//     if( ok ) {
//         cout << "fetch 0 got " << val << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });

// zmqFetchFromPartner_double(1, 0, [](const bool ok, const double val){
//     if( ok ) {
//         cout << "fetch 1 got " << val << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });

// zmqFetchFromPartner_double(2, 0, [](const bool ok, const double val){
//     if( ok ) {
//         cout << "fetch 2 got " << val << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });

// zmqFetchFromPartner_uint32_t(1, 2, [](const bool ok, const uint32_t val){
//     if( ok ) {
//         cout << "fetch 3 got " << val << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });

// zmqFetchFromPartner_uint32_t(2, 2, [](const bool ok, const uint32_t val){
//     if( ok ) {
//         cout << "fetch 4 got " << val << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });


// zmqFetchFromPartner_pair_uint32_t_uint32_t(1, 3, [](const bool ok, const std::pair<uint32_t,uint32_t> val){
//     if( ok ) {
//         uint32_t a,b;
//         std::tie(a,b) = val;
//         cout << "fetch 5 got " << a << ", " << b << "\n";
//     } else {
//         cout << "failed\n";
//     }
// });
