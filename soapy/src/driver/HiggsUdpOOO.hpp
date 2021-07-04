#pragma once

#include <map>
#include <iterator>
#include <vector>
#include <mutex>
#include <boost/pool/simple_segregated_storage.hpp>
#include <vector>
#include <cstddef>

typedef std::map<uint32_t, unsigned char*> udp_map_t;

#define MEM_STORAGE_EXTRA (10000)
#define MEM_STORAGE (GIVEUP+MEM_STORAGE_EXTRA) // only valid inside class

// user story:
// user calls get() this returns a block ready to be written to
// user calls markFull() this means that the block we just got is "Full" and has a seq
// user is advised to call get1,markFull1,get2,markfull2
// user calls getNext() and the next sequence number is returned, or 0
// user calls release on the result of getNext when done

template <unsigned int N, unsigned int GIVEUP=40>
class HiggsUdpOOO {
public:
    // uint32_t seq_number;
    bool seq_ok;
    uint32_t initial_seq_number;
    uint32_t errors_found;
    uint32_t num_drops;

    // boost pool
    boost::simple_segregated_storage<std::size_t> pool;
    // boost pool memory
    std::vector<unsigned char> pool_memory;

    size_t memstart; // for debug

    // full
    udp_map_t full;

    uint32_t a_seq;

    mutable std::mutex m;
    bool printAlloc;
    bool printDrop;
    bool printFull;

    HiggsUdpOOO(bool _printD=true, bool _printA=false, bool _printF=false) :
     seq_ok(false), 
     initial_seq_number(0), 
     errors_found(0), 
     num_drops(0), 
     m(), 
     printAlloc(_printA),
     printDrop(_printD),
     printFull(_printF)
    {
        pool_memory.resize(N*MEM_STORAGE);
        // give the vector's memory to the pool, the 3rd argument is how big each chunk should be
        pool.add_block(&pool_memory.front(), pool_memory.size(), N);

        memstart = reinterpret_cast<size_t>(&pool_memory.front());
    };

    unsigned char* get(void) {
        unsigned char* mem;
        // std::cout << "made mem " << full.size() << std::endl;

        if( printAlloc ) {
            std::cout << "get : ";
        }

        std::lock_guard<std::mutex> lock(m);

        bool out_of_space = pool.empty();

        if( out_of_space ) {
            std::cout << std::endl << "HiggsUdpOOO ran out of space during get() and will crash" << std::endl;
            usleep(100);
        }

        mem = static_cast<unsigned char*>( pool.malloc() );
        
        if( printAlloc ) {
            size_t got = reinterpret_cast<size_t>(mem);

            std::cout << (got - memstart) / N << std::endl;
        }

        return mem;
    }

    void markFull(unsigned char* data) {
        std::lock_guard<std::mutex> lock(m);
        unsigned int seq_number = (reinterpret_cast<uint32_t*>(data)[0]);
        a_seq = seq_number; // FIXME delme
        // std::cout << "mF() " << HEX32_STRING(seq_number) << std::endl;

        // if( (seq_number < initial_seq_number) || (seq_number > (initial_seq_number+80)) ) {
        //     std::cout << "Wild seq: " << seq_number << "\n";
        // }


        full.insert(std::make_pair(seq_number, data));

        if(printAlloc) {
            size_t got = reinterpret_cast<size_t>(data);

            std::cout << "ful :    " << (got - memstart) / N << std::endl;
        }

        if(!seq_ok) {
            seq_ok = true;
            initial_seq_number = seq_number;
        }
    }


    void release(unsigned char* old) {
        std::lock_guard<std::mutex> lock(m);

        if(printAlloc) {
            size_t got = reinterpret_cast<size_t>(old);

            std::cout << "free:       " << (got - memstart) / N << std::endl;
        }
        this->pool.free(old);
    }

    // void releaseIndex(size_t idx) {
    //     auto memstart2 = reinterpret_cast<unsigned char*>(memstart);
    //     memstart2 += (idx*N);
    //     this->pool.free(memstart2);
    // }

    void debug_printAlloc_load() {
        static int counter = 0;
        if(counter % 15000 == 0) {
            std::cout << "recent seq: " << a_seq << std::endl;
            std::cout << "carrying load " << full.size() << " [";

            for(auto it = this->full.begin(); it != this->full.end(); ++it) {

                std::cout << it->first << ", ";
            }
        std::cout << std::endl;

        }
        counter++;
    }

    // user is asking for next valid sequence numbered packet
    // this function does a search through all known packets for the next index
    // if no valid packet is found, 0 is returned.
    // if more than 40 packets are held and no valid is found, we assume a network glitch
    // we drop from map / free everything except the packet with the largest sequence number and resume
    // this could be more data friendly by doing a more expensive look for gaps (a gap will always have ~39
    // packets droped after it that we could have saved)
    unsigned char* getNext() {
        std::lock_guard<std::mutex> lock(m);


        if(full.size() == 0) {
            return 0; // no error, nothing to find
        }

        if( printFull ) {
            std::cout << "Full # " << full.size() << std::endl;
        }
        
        auto it2 = full.find(initial_seq_number);
        if (it2 != full.end()) {
            unsigned char *ptr = it2->second;
            full.erase(it2);
            initial_seq_number++;
            if(printAlloc) {
                size_t got = reinterpret_cast<size_t>(ptr);
                std::cout << "next:          " << (got - memstart) / N << std::endl;
            }
            return ptr;
        }

        if(full.size() > GIVEUP) {
            if( printDrop ) {
                std::cout << "Not found " << initial_seq_number << " mem size is: " << full.size() << std::endl;
            }

            uint32_t new_max = 0;
            unsigned char* data_max = 0;


            // loop through one time to find the largest packet we've seen
            if(printAlloc)  { std::cout << "erasing: "; }
            for(auto it = this->full.begin(); it != this->full.end(); ++it) {
                if(it->first > new_max) {
                    new_max = it->first;
                    data_max = it->second;
                    // arg_of_max = it;
                }
                // new_max = std::max(it->first,new_max);
                if(printAlloc) {
                    size_t got = reinterpret_cast<size_t>(it->second);
                    std::cout << (got - memstart) / N << " ";
                }
            }
            if(printAlloc) {std::cout << std::endl;}

            // loop through again to free() all the memory we are not going to keep
            for(auto it = this->full.begin(); it != this->full.end(); ++it) {
                // for all packets we are dropping
                if(data_max != it->second) {
                    this->pool.free(it->second);
                }
            }

            // empty the map
            full.erase(full.begin(), full.end());

            // put 1 element back in map, the largest thing we have in the buffer (the packet we are keeping)
            full.insert(std::make_pair(new_max, data_max));
            initial_seq_number = new_max;

            if(printAlloc) {
                size_t got = reinterpret_cast<size_t>(data_max);
                std::cout << "put back: " << (got - memstart) / N << std::endl;
            }
            
            num_drops++;

            if( printDrop ) {
                std::cout << std::endl << std::endl << "!!!!!!!!SSetting initial sequence number to: "
                          << initial_seq_number << std::endl
                          << std::endl
                          << std::endl
                          << std::endl;
            }
            return 0;

        } // full size over thresh

        // std::cout << "carrying load " << full.size() << std::endl;
        return 0;

    } // getNext
}; // HigsUdpOOO


#undef MEM_STORAGE
#undef MEM_STORAGE_EXTRA