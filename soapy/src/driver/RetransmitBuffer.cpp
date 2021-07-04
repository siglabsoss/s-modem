#include "driver/RetransmitBuffer.hpp"

#include <iostream>

using namespace std;

RetransmitBuffer::RetransmitBuffer() {}


void RetransmitBuffer::sentOverAir(const uint8_t seq, const std::vector<std::vector<uint32_t>> bunch) {
    std::unique_lock<std::mutex> lock(m);

    // retransmit_buffer_map_t
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // cout << "sentOverAir() " << " bunch.size()"  << endl;
    // for( x : bunch ) {
    //     cout << " bunch inner size (" << x.size() << ")";
    // }
    // cout << endl;

    const auto rhs = std::make_tuple(seq, bunch);

    buffer.insert(std::make_pair(now, rhs));

    // printTable();
}


void RetransmitBuffer::scheduleRetransmit(const uint8_t seq, const std::vector<uint32_t>& indices) {
    std::unique_lock<std::mutex> lock(m);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // printTable();

    bool allmissing = false;

    if( indices.size() == 1 && indices[0] == 0xffffffff ) {
        allmissing = true;
    }

    cout << "!!!!!!!!!!!!!!!!!!!!seq " << (int) seq << " idxs: ";
    for( auto x : indices ) {
        cout << x << ",";
    }
    cout << endl;

    bool found = false;
    for(retransmit_buffer_map_t::iterator it=buffer.begin(); it!=buffer.end();) {

        // if()
        auto buffer_seq = std::get<0>(it->second);

        // if we found the data we want to retransmit
        if( buffer_seq == seq ) {

            // packets.size() // would be how many packets were went at once
            // packets[0] // would be the first packet
            const auto packets = std::get<1>(it->second);


            if( allmissing ) {
                cout << "scheduleRetransmit matched ALL MISSING" << endl;

                for( const auto& packet : packets ) {
                    need_retransmit.push_back(packet);
                }

            } else {

                // cout << "scheduleRetransmit matched" << endl;

                bool do_break = false;
                for(const auto x : indices) {
                    if( x >= packets.size() ) {
                        // cout << "scheduleRetransmit() got illegal packet index " << x << " " << packets.size() << endl;;
                        do_break = true;
                    }
                }
                if( do_break ) {
                    break;
                }
                for(const auto x : indices) {
                    // cout << "Queued index " << x << endl;
                    // cout << "packets.size() " << packets.size() << endl;
                    if( packets.size() > 1919293 ) {
                        // cout << "packets.size() is corrupted" << endl;
                        break;
                    }
                    // cout << "packets[x].size() " << packets[x].size() << endl;

                    need_retransmit.push_back(packets[x]);
                }
            }



            // cout << "delete route" << endl;
            // bump_counter += it->second.size();
            // bump_counter2 += it->second.size();
            it = buffer.erase(it); // special way of ++ in delete case
            found = true;
            break;
        } else {
            // cout << "keep route" << endl;
            ++it;
        }
    }

    if( !found ) {
        cout << "Did not find scheduleRetransmit() for seq " << (int) seq << endl;
    }

    pruneOld(buffer, now);

}

// only call if you already have the lock
void RetransmitBuffer::pruneOld(retransmit_buffer_map_t &mm, std::chrono::steady_clock::time_point now) const {

    // cout << "pruneOld for size " << m.size() << endl;

    // for loop does not have a 3rd statement
    for(retransmit_buffer_map_t::iterator it=mm.begin(); it!=mm.end();) {
        unsigned age = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - it->first
        ).count();

        if( age > delete_after_us ) {
            // cout << "delete route" << endl;
            // bump_counter += it->second.size();
            // bump_counter2 += it->second.size();
            it = mm.erase(it); // special way of ++ in delete case
        } else {
            // cout << "keep route" << endl;
            ++it;
        }

    }
}


unsigned RetransmitBuffer::firstLength() const {
    std::unique_lock<std::mutex> lock(m);
    if( need_retransmit.size() == 0 ) {
        return 0;
    }
    return need_retransmit[0].size();
}
unsigned RetransmitBuffer::packetsReady() const {
    std::unique_lock<std::mutex> lock(m);

    return need_retransmit.size();
}

std::vector<uint32_t> RetransmitBuffer::getPacket() {
    std::unique_lock<std::mutex> lock(m);

    std::vector<uint32_t> v;
    v = need_retransmit[0];
    need_retransmit.erase(need_retransmit.begin());
    return v;
}


void RetransmitBuffer::printTable() const {

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    unsigned i = 0;
    for(retransmit_buffer_map_t::const_iterator it=buffer.begin(); it!=buffer.end();) {

        unsigned age = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - it->first
        ).count();

        cout << "Age " << ((double)age)/1E6 << ", " << (int) std::get<0>(it->second) << endl;


        auto packets = std::get<1>(it->second);
        // cout << "scheduleRetransmit matched" << endl;

        // bool do_break = false;
        // for(auto x : indices) {
        //     if( x > packets.size() ) {
        //         cout << "scheduleRetransmit() got illegal packet index " << x << " " << packets.size() << endl;;
        //         // do_break = true;
        //     }
        // }
        // if( do_break ) {
        //     // break;
        // }
        // for(auto x : indices) {
            // cout << "Queued index " << x << endl;
            cout << "packets.size() " << packets.size() << endl;
        // }

        // it = m.erase(it); // special way of ++ in delete case
        // found = true;
        // break;
        ++i;
        ++it;
    // } else {
    //     ++i;
    //     ++it;
    }
}