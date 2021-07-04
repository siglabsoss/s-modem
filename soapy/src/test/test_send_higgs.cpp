#include <rapidcheck.h>
#include "common/CounterHelper.hpp"
#include "common/constants.hpp"


#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

std::vector<uint8_t> _sendto_data;
std::vector<unsigned char> pending_for_higgs;

// std::lock_guard<std::mutex> lock(tx_sock_mutex);

// reset any globals before each run
void reset_sendto() {
    _sendto_data.resize(0);
    pending_for_higgs.resize(0);
}

int mock_sendto(int tx_sock_fd, unsigned char* start, size_t this_len, int flags, void *tx_address, size_t tx_address_len) {
    for(unsigned int i = 0; i < this_len; i++) {
        _sendto_data.push_back(start[i]);
    }
}


int mock_sendToHiggs(unsigned char* data, size_t len) {
    int sendto_ret = -1;
    {
        // grab the lock
        // std::lock_guard<std::mutex> lock(tx_sock_mutex);
        
        // max packet size
        int const chunk = 367*4;

        size_t sent = 0;
        // floor of int divide
        size_t floor_chunks = len/chunk;

        // loop that +1
        for(size_t i = 0; i < floor_chunks+1; i++ ) {
            unsigned char* start = data + sent;

            // calculate remaining bytes
            int remaining = (int)len - (int)sent;
            // this chunk is the lesser of:
            int this_len = std::min(chunk, remaining);

            // if exact even division, bail
            if(this_len <= 0) {
                break;
            }

            // for( size_t j = 0; j < this_len; j++) {
            //     std::cout << HEX_STRING((int)start[j]) << ",";
            // }
            // std::cout << std::endl;

            // run with values
            auto tx_sock_fd = 0;
            sendto_ret = mock_sendto(tx_sock_fd, start, this_len, 0, 0, 0);
            // usleep(10);
            sent += this_len;
        }


    }
    return sendto_ret;
}














int mock_petSendToHiggsSub(unsigned int allow_repeat) {

    int sendto_ret;

    // max packet size
    int const chunk = SEND_TO_HIGGS_PACKET_SIZE;

    size_t len = pending_for_higgs.size();
    unsigned char *data = pending_for_higgs.data();

    size_t sent = 0;
    // floor of int divide
    size_t floor_chunks = len/chunk;

    // loop that +1
    for(size_t i = 0; i < floor_chunks+1; i++ ) {
        unsigned char* start = data;

        // calculate remaining bytes
        int remaining = (int)len - (int)sent;
        // this chunk is the lesser of:
        int this_len = std::min(chunk, remaining); // in bytes

        // if exact even division, bail
        if(this_len <= 0) {
            break;
        }

        //  for( size_t j = 0; j < this_len; j++) {
        //    std::cout << HEX_STRING((int)start[j]) << ",";
        // }
        // std::cout << std::endl;

        // run with values
        //! FIXME: we don't catch errors here?
        int tx_sock_fd = 0;
        sendto_ret = mock_sendto(tx_sock_fd, start, this_len, 0, 0, 0);
        cout << ".";

        sent += this_len;
        if( allow_repeat <= 0 ) {
            break;
        }

        // usleep(1);
        allow_repeat--;
    }

    if( sent > 0 ) {
        // cout << "erasing " << sent << endl;
        // pass 2 pointers ?
        pending_for_higgs.erase( pending_for_higgs.begin(),  pending_for_higgs.begin()+sent );
    }

    return (int)sent;
}

uint32_t mock_petSendToHiggs();

int mock_sendToHiggs_new(unsigned char* data, size_t len) {
    if(len != 0) {
        cout << "sendToHiggs() got size " << len << endl;
    }
    int ret = 0;
    {
        // grab the lock
        // std::lock_guard<std::mutex> lock(tx_sock_mutex);

        for(size_t i = 0; i < len; i++ ) {
            pending_for_higgs.push_back(data[i]);
        }
    }

    mock_petSendToHiggs();
    return ret;
}




uint32_t mock_petSendToHiggs() {

    // if should_pause
    // return;

    int did_send = 0;

    int allow_extra = 0;


    int allowed_to_send = *rc::gen::inRange(-10000, 1024*7*4);

    if( allowed_to_send > SEND_TO_HIGGS_PACKET_SIZE*8 ) {
        // one time boost, in the beginning
        allow_extra = 8;
    }




    did_send = mock_petSendToHiggsSub(allow_extra);
 

    // std::cout << "sent: " << did_send << " had_slept " << us_run << " since_sent " << us_sent << std::endl;
    
    // if( us_run > 9000 || did_send > 0) {
    // }
    // if( did_send > 0 ) {
    //     cout << "sent " << did_send << endl;
    // }
    



    // click over previous
    // time_prev_pet_send_higgs = now;



    if( did_send > 0 ) {

        // time_prev_sent_pet_higgs = now;

        // wake up in 10 us, because we sent data
        // return (struct timeval){ .tv_sec = 0, .tv_usec = 10 };
        return 10;
    } else {
        // out of data, send in a tiny bit
        // return (struct timeval){ .tv_sec = 0, .tv_usec = 8000 };
        return 8000;
    }

}














int main() {
    rc::check("foo", []() {

        // const auto maybeSmallInt = *rc::gen::maybe(rc::gen::inRange(0, 100));



        // auto j = Maybe<int>();

        // cout << Maybe<int>() << "";

        // auto val = *rc::gen::Maybe();


    });

    // exit(0);

    rc::check("Check original approach", []() {

        CounterHelper c1;

        reset_sendto();

        // how many times to call it
        const auto function_calls = *rc::gen::inRange(0, 1000);

        auto total_added = 0;

        for(auto i = 0; i < function_calls; i++) {

            // const bool check_this_clock = *rc::gen::inRange(0, 100) < 40;
            const auto bytes = *rc::gen::inRange(0, 1500);

            std::vector<unsigned char> tmp;
            for(auto j = 0; j < bytes; j++) {
                tmp.push_back( c1.inc() );
            }

            mock_sendToHiggs(tmp.data(), bytes);

            total_added += bytes;
        }

         RC_ASSERT( _sendto_data.size() == total_added );

         CounterHelper expected;


        for(auto s : _sendto_data) {
            // cout << s << ",";
            uint8_t expected_val = (uint8_t)expected.inc();

            RC_ASSERT(s == expected_val);
        }

    });


    rc::check("Check new way", []() {

        CounterHelper c1;

        reset_sendto();

        // how many times to call it
        const auto function_calls = *rc::gen::inRange(0, 1000);

        auto total_added = 0;

        for(auto i = 0; i < function_calls; i++) {

            // const bool check_this_clock = *rc::gen::inRange(0, 100) < 40;
            const auto bytes = *rc::gen::inRange(0, 1024*8);

            const bool add_chance = *rc::gen::inRange(0, 100) < 40;
            const bool pull_chance = *rc::gen::inRange(0, 100) < 90;


            // without doing threads, this is hard
            // in this case we just optionally run this
            if( add_chance ) {
                std::vector<unsigned char> tmp;
                for(auto j = 0; j < bytes; j++) {
                    tmp.push_back( c1.inc() );
                }
                mock_sendToHiggs_new(tmp.data(), bytes);
                total_added += bytes;
            }

            // optional run that
            if( pull_chance ) {
                mock_petSendToHiggs();
            }

        }


        // flush internal buffer, if this number 1000, doesn't work, not sure
        for(auto i = 0; i < 10; i++) {
            mock_petSendToHiggs();
        }

        // IF this doesn't flush, the test did not finish or other major error
        RC_ASSERT( pending_for_higgs.size() == 0 );


         RC_ASSERT( _sendto_data.size() == total_added );

        CounterHelper expected;

        for(auto s : _sendto_data) {
            // cout << s << ",";
            uint8_t expected_val = (uint8_t)expected.inc();

            RC_ASSERT(s == expected_val);
        }


        cout << endl << endl;

    });



  return 0;
}

