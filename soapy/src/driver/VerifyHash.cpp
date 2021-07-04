#include <assert.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "cpp_utils.hpp"
#include "common/convert.hpp"
#include "driver/VerifyHash.hpp"
#include <boost/functional/hash.hpp>
#include "crc32.hpp"

using namespace std;

// AirPacket

VerifyHash::VerifyHash() :
crc(new siglabs::crc::Crc32())
{}

uint32_t VerifyHash::getLengthFromHash(const uint32_t w) {
    return (w & 0xfff);
}

// use boost hash or'd with length to get a combined (but still unique)
// identifier
uint32_t VerifyHash::getHashForChars(const std::vector<unsigned char> &v) const {
    // boost::hash<std::vector<unsigned char>> char_hasher;

    // take lower bits for length.  this will cap an individual packet length out at 4095
    const uint32_t length = v.size() & 0xfff;

    // cout << "length " << length << "\n";

    const uint32_t h2 = crc->calc(v);

    const uint32_t combined = (h2 & 0xfffff000) | length;

    return combined;
}

uint32_t VerifyHash::getHashForWords2(const std::vector<uint32_t> &v) const {
    // boost::hash<std::vector<unsigned char>> char_hasher;

    // take lower bits for length.  this will cap an individual packet length out at 4095
    const uint32_t length = (v.size()*4) & 0xfff;

    // cout << "length " << length << "\n";

    const uint32_t h2 = crc->calc(v);

    const uint32_t combined = (h2 & 0xfffff000) | length;

    return combined;
}

uint32_t VerifyHash::getHashForWords(const std::vector<uint32_t> &v) const {

    // boost::hash<std::vector<unsigned char>> char_hasher;

    std::vector<unsigned char> back_as_char = wordVectorPacketToCharVector(v);

    // truncate the hash, it's a size_t but we will just compare 32 bits of it
    const auto ipv4_hash = getHashForChars(back_as_char);
    return ipv4_hash; // fixme

    const auto ipv4_len = getLengthFromHash(ipv4_hash);
    if( ipv4_len == 0) {
        std::cout << "getHashForWords in fallback mode\n";
        return getHashForWords2(v);
    } else {
        return ipv4_hash;
    }

    
}
// received the "authoratative" version
void VerifyHash::gotHashListFromPeer(const std::vector<uint32_t> &v) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    if( do_print ) {
        cout << "VerifyHash::gotHashListFromPeer got " << v.size() << " hashes" << endl;
        for(auto x : v) {
            cout << "   " << HEX32_STRING(x) << endl;
        }
    }

    expected.insert(std::make_pair(now, v));

    rateControlledReport();
}
// received a maybe ok version
verify_hash_retransmit_t VerifyHash::gotHashListOverAir(const std::vector<uint32_t> &v, const air_header_t &header) {
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    if( do_print ) {
        cout << "VerifyHash::gotHashListOverAir got " << v.size() << " hashes" << endl;
        for(auto x : v) {
            cout << "   " << HEX32_STRING(x) << endl;
        }
    }

    std::vector<uint32_t> v2 = v;
    auto seq = std::get<1>(header);
    (void)seq;
    // cout << "!!yy!!!!!!!! seq " << (int)seq << endl;
    v2.insert(v2.begin(), std::get<1>(header) );

    received.insert(std::make_pair(now, v2));

    // this->ins.insert();

    const auto packed = rateControlledReport(now);

    const auto retransmit = std::get<1>(packed);
    return retransmit;
}

std::tuple<std::string,verify_hash_retransmit_t> VerifyHash::rateControlledReport(const std::chrono::steady_clock::time_point for_key) {
    report_after_counter++;

    // cout << "rateControlledReport " << report_after_counter << endl;

    // if( (report_after_counter % report_after_interactions) == (report_after_interactions-1) ) {
    auto packed = getStatus(for_key);
    auto msg = std::get<0>(packed);
    // cout << msg << endl;
    //     report_after_counter = 0;
    // }

    return packed;
}

///
/// returns 
///   first: is this considered a match
///   second: number correct
///   third : total number
///   fourth: bytes correct
///   fifth : total bytes
/// a is found
/// b is ideal (for byte calculation purposes)
std::tuple<bool, unsigned, unsigned, unsigned, unsigned, std::vector<uint32_t>> VerifyHash::compare(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b) {
    unsigned greater_size = std::max(a.size(), b.size());
    unsigned lesser_size = std::min(a.size(), b.size());
    unsigned same = 0;
    unsigned bytes_same = 0;
    unsigned bytes_total = 0;

    const auto invalid_return = std::make_tuple(false, 0, 0, 0, 0, std::vector<uint32_t>());

    std::vector<uint32_t> invalid_idx;
    for(unsigned i = 0; i < lesser_size; i++) {
        if( a[i] == b[i] ) {
            same++;
            if( i != 0 ) {
                bytes_same += VerifyHash::getLengthFromHash(a[i]);
                bytes_total += VerifyHash::getLengthFromHash(b[i]);
            }
        } else {
            if( i != 0 ) {
                bytes_total += VerifyHash::getLengthFromHash(b[i]);
                invalid_idx.push_back(i-1);
            }
        }
    }

    if( lesser_size < 5 ) {
        // strict comparisons for small packets
        if( greater_size == lesser_size && same == lesser_size ) {
            return std::make_tuple(true, lesser_size, lesser_size, bytes_same, bytes_total, invalid_idx);
        } else {
            return invalid_return;
        }
    } else {
        // loose comparisons for large packets

        double less_d = lesser_size;
        double tol = less_d * 0.7;

        if( same > tol ) {
            return std::make_tuple(true, same, greater_size,       bytes_same, bytes_total, invalid_idx);
        } else {
            return invalid_return;
        }
    }


    // return std::make_tuple(default_error_state, "", default_error_timestamp, false, true);


        // auto status = std::get<0>(it1->second);
        // auto message = std::get<1>(it1->second);
        // auto timeout = it2->second;
        // auto remote = std::get<3>(it1->second);
        // auto auto_print = std::get<4>(it1->second);

    // bool match = false;
    // unsigned scorejjlk = 0;
    // // return std::pair(match, scorejjlk);
    // return std::pair<bool, unsigned>(match, scorejjlk);
}


// The iterate and erase algorithm is here
// https://stackoverflow.com/questions/4600567/how-can-i-delete-elements-of-a-stdmap-with-an-iterator
// iterates over one of the maps, either the got over air, or the sent from tx map
// when we find things that have expired, we add to headers
void VerifyHash::pruneOld(verify_hash_internal_t &m, const std::chrono::steady_clock::time_point now, unsigned &bump_counter, unsigned &bump_counter2, unsigned &bump_bytes) {

    // cout << "pruneOld for size " << m.size() << endl;

    // for loop does not have a 3rd statement
    for(verify_hash_internal_t::iterator it=m.begin(); it!=m.end();) {
        unsigned age = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - it->first
        ).count();

        if( age > delete_after_us ) {
            // cout << "delete route" << endl;
            bump_counter += it->second.size();
            bump_counter2 += it->second.size();
            it = m.erase(it); // special way of ++ in delete case
        } else {
            // cout << "keep route" << endl;
            ++it;
        }

    }
}

void VerifyHash::resetCounters() {
    count_sent = 0;
    count_got = 0;
    bytes_sent = 0;
    bytes_got = 0;
    count_stale_expected = 0;
    count_stale_received = 0;
    percent_correct = 0.0;
    report_after_counter = 0;
    expected.clear();
    received.clear();
}

std::tuple<std::string,verify_hash_retransmit_t> VerifyHash::getStatus(const std::chrono::steady_clock::time_point for_key) {
    std::stringstream ss;

    const verify_hash_retransmit_t invalid_return = std::make_tuple(false, 0, std::vector<uint32_t>());

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    unsigned junk = 0;
    unsigned junk2 = 0;

    // cout << "getStatus() " << endl;
    // if something in the expected map expires, it means we sent it
    // but never found it
    pruneOld(expected, now, count_stale_expected, count_sent, bytes_sent);

    // if something in the received map expires, it means we didn't match it against anything
    // so it was badly corrupted, but we pass junk, becuase this already got counted
    // by the expected entry going stale
    pruneOld(received, now, count_stale_received, junk, junk2);

    bool key_of_interest = false;
    bool need_retransmit = false;
    verify_hash_retransmit_t retransmit_return;
    // cout << "done prune " << endl;

    // outer loop does not have 3rd statement in for, see pruneOld() for a simple example
    for(verify_hash_internal_t::iterator found=received.begin(); found!=received.end();) {

        // unsigned received_age = std::chrono::duration_cast<std::chrono::microseconds>( 
        // now - found->first
        // ).count();

        // if( received_age < young_cutoff_us ) {
        //     ++found; // normal ++
        //     continue;
        // }

        if( found->first == for_key ) {
            key_of_interest = true;
            // cout << "!!!!!!!!!!!!! WORKED getStatus() " << endl;
        } else {
            key_of_interest = false;
        }

        std::tuple<bool, unsigned, unsigned, unsigned, unsigned, std::vector<uint32_t>> match_results(false, 0, 0, 0, 0, std::vector<uint32_t>());
        verify_hash_internal_t::const_iterator ideal;
        for(ideal=expected.begin(); ideal!=expected.end(); ++ideal) {
            // unsigned ideal_age = std::chrono::duration_cast<std::chrono::microseconds>( 
            // now - ideal->first
            // ).count();

            // if( ideal_age < young_cutoff_us  ) {
            //     continue;
            // }

            // first is found
            // second is ideal
            match_results = compare(found->second, ideal->second);

            if( std::get<0>(match_results) ) {
                break;
            }
        }

        auto did_match = std::get<0>(match_results); // did we match?
        auto match_correct = std::get<1>(match_results); // how many words were correct
        auto match_total = std::get<2>(match_results); // how many total words were there
        auto match_bytes_correct = std::get<3>(match_results); // how many words were correct
        auto match_bytes_total = std::get<4>(match_results); // how many total words were there
        auto missing_indices = std::get<5>(match_results); // which packet indices are we missing



        // cout << "match? " << (int)did_match << endl;
        need_retransmit = false;
        if( did_match ) {
            // cout << " v " << match_correct << ", " << match_total << endl;
            if( match_total != match_correct ) {
                if( do_print_partial ) {
                    cout << " PARTIAL MATCH!!! " << endl;
                    cout << "Found: " << endl;
                    for( auto x : found->second ) {
                        cout << HEX32_STRING(x) << endl;
                    }
                    cout << "Expected: " << endl;
                    for( auto x : ideal->second ) {
                        cout << HEX32_STRING(x) << endl;
                    }
                }

                need_retransmit = true;
            }

            data_rate.insert(std::make_pair(now, match_bytes_correct));

        }


        if( did_match ) {
            count_got += match_correct;
            count_sent += match_total;
            bytes_got += match_bytes_correct;
            bytes_sent += match_bytes_total;
            

            // cout << "did_match " << (int) did_match << " ideal->second " << ideal->second[0] << endl;
            auto match_sequence = ideal->second[0];


            retransmit_return = std::make_tuple(true, match_sequence, missing_indices);

            found = received.erase(found); // special way of ++ in delete case
            expected.erase(ideal); // don't need to worry about saving result, loop will set again

            if( key_of_interest ) {
                break; // new behavior, guarentee we only match 1 packet
            }

        } else {
             ++found; // normal ++
        }
    }

    updateDataRate(now);

    if( count_sent != 0 ) {
        percent_correct = ((double)count_got / (double)count_sent) * 100.0;
    }

    ss << "VerifyHash status:" << endl;
    ss << "  Correct : " << count_got << endl;
    ss << "  Total   : " << count_sent << endl;
    ss << "  %Correct: " << percent_correct << endl;
    ss << "  Rate    : " << bytes_per_second << endl;
    ss << endl;
    ss << "  Stale expected: " << count_stale_expected << endl;
    ss << "  Stale received: " << count_stale_received << endl;

    if( key_of_interest && need_retransmit ) {
        return std::make_tuple(ss.str(), retransmit_return);
    } else {
        return std::make_tuple(ss.str(), invalid_return);
    }
}


void VerifyHash::updateDataRate(const std::chrono::steady_clock::time_point now) {

    // prune data rate,
    // keep data received for 2 seconds
    const unsigned delete_after_us2 = (1E6 * data_rate_average_seconds);
    for(verify_hash_rate_t::iterator it=data_rate.begin(); it!=data_rate.end();) {
        unsigned age = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - it->first
            ).count();

        if( age > delete_after_us2 ) {
            // cout << "delete route" << endl;
            // bump_counter += it->second.size();
            // bump_counter2 += it->second.size();
            it = data_rate.erase(it); // special way of ++ in delete case
        } else {
        // cout << "keep route" << endl;
            ++it;
        }
    }

    double local_rate = 0;
    for(verify_hash_rate_t::iterator it=data_rate.begin(); it!=data_rate.end();++it) {
        local_rate += it->second;
    }

    local_rate /= data_rate_average_seconds; // because we are keeping data for 2 seconds

    bytes_per_second = local_rate;


}