#include <rapidcheck.h>
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <boost/pool/simple_segregated_storage.hpp>
#include <vector>
#include <cstddef>
#include <chrono>
#include "driver/HiggsUdpOOO.hpp"


#define UDP_RX_BYTES 32u

// #include "common/utilities.hpp"
// #include "gen/sidecontroljsonclient.h"

using namespace std;
using namespace std::chrono;

void sseq(unsigned char* pkt, unsigned int v) {
    unsigned int* word = (unsigned int*)pkt;
    word[0] = v;
}

void tag(unsigned char* pkt, unsigned int v, unsigned char tag='a') {
    unsigned int* word = (unsigned int*)pkt;
    word[0] = v;
    word[1] = (int)tag;
}

void pprint(unsigned char* pkt) {
    unsigned int* word = (unsigned int*)pkt;
    cout << "seq: " << word[0] << " tag: " << (char)word[1] << endl;
    // word[0] = v;
    // word[1] = (int)tag;
}

void pprintint(unsigned char* pkt) {
    unsigned int* word = (unsigned int*)pkt;
    cout << "seq: " << word[0] << " tag: " << (int)word[1] << endl;
    // word[0] = v;
    // word[1] = (int)tag;
}

#define TEST_SEQ (4671680)

int rrecv(unsigned char* pkt, int skipAt=4671690) {
    static int calls = 0;
    static int seq = TEST_SEQ;

    tag(pkt, seq, 'a'+calls);

    if( seq == skipAt ) {
        seq+=10;
    }

    calls++;
    seq++;
    return 2;
}

void main9()
{

    boost::simple_segregated_storage<std::size_t> storage;
    std::vector<char> v(1024);
    storage.add_block(&v.front(), v.size(), 256);

    int *i = static_cast<int*>(storage.malloc());
    *i = 1;

    int *j = static_cast<int*>(storage.malloc_n(1, 512));
    j[10] = 2;

    storage.free(i);
    storage.free_n(j, 1, 512);
}

void check1(void) {
    // rc::check("get release", []() {
        unsigned char* pkt;//[UDP_RX_BYTES];
        unsigned char* reordered_pkt;
        int rx_val;
        {
            HiggsUdpOOO<UDP_RX_BYTES> memory;

            int totalRuns = 20000;

            for(auto i = 0; i < totalRuns; i++) {

                pkt = memory.get();

                // memory.foo(pkt);

                // memory.pool.free(pkt);
                // memory.markFull(pkt);
                memory.release(pkt);
            }
            cout << " ahh  motherland " << endl;
        }
            cout << " ahh  motherland " << endl;


    // });
}


bool check2(void) {

    return rc::check("get release one drop", []() {
        unsigned char* pkt;
        unsigned char* reordered_pkt;
        int rx_val;

        {
            HiggsUdpOOO<UDP_RX_BYTES,3u> memory(false,false);

            pkt = memory.get();

            int runs = 0;
            const int totalRuns = *rc::gen::inRange(200, 1000);

            const int dropAt = *rc::gen::inRange(10, 750);


            // high_resolution_clock::time_point t1 = high_resolution_clock::now();


            while(runs < totalRuns) {
                // cout << "run:                              " << runs << endl;

                rx_val = rrecv(pkt,TEST_SEQ+dropAt);
                // cout << (reinterpret_cast<size_t>(pkt)&0xfff) << endl;

                if( rx_val > 1) {
                    // tell our class there is memory here
                    memory.markFull(pkt);

                    // allocate another memory and overwrite
                    pkt = memory.get();
                }

                reordered_pkt = memory.getNext();
                while( reordered_pkt != 0 ) {
                    // cout << "x" << endl;
                    // pprint(reordered_pkt);
                    memory.release(reordered_pkt);
                    reordered_pkt = memory.getNext();
                }

                runs++;
            } // while

            // memory.releaseIndex(0);
        }
        // cout << "dealoc" << endl;

    }); // rapid check
}

bool check3(void) {

    return rc::check("reorder", []() {
        unsigned char* pkt;
        unsigned char* reordered_pkt;
        int rx_val;

        {
            HiggsUdpOOO<UDP_RX_BYTES,10u> memory(true,false,false);

            pkt = memory.get();

            // RC_ASSERT(0);

            int runs = 0;
            const int totalRuns = *rc::gen::inRange(2,254);

            // const int dropAt = *rc::gen::inRange(, 750);
            // int dropAt = 10;

            int seq = 0;

            rx_val = 1; // hardwire

            bool requestReorder = false;
            bool doingReorder = false;
            bool prevWasReorder = false;
            int reorderSeq;
            int reorderDelay = 2;
            int reorderFinish;

            unsigned int checkOrder = 0;

            int timesReordered = 0;

            while(runs < totalRuns) {
                // cout << "run:                              " << runs << endl;
                
                // are we allowed to reorder this loop?
                bool canReorder = (runs > 1) && (runs < totalRuns-10) && !doingReorder && !prevWasReorder;

                if( canReorder ) {
                    // only make random pulls if we are allowed
                    const int pull = *rc::gen::inRange(0,100);
                    bool shouldReoder = pull > 30 && pull <= 60;  // 30 % chance
                    if(shouldReoder) {
                        requestReorder = true;
                        reorderDelay = *rc::gen::inRange(2,9);
                        timesReordered++;
                    }
                }

                if( requestReorder ) {
                    requestReorder = false;
                    doingReorder = true;
                    reorderSeq = seq;
                    reorderFinish = seq + reorderDelay;

                    seq++;
                    // cout << "doing reorder at " << runs << endl;
                }

                tag(pkt, 0, (char)0); // reset memory for fun

                if(doingReorder && reorderFinish == seq) {
                    // put the old packet in
                    tag(pkt, reorderSeq, (unsigned char)reorderSeq);
                    // dont bump seq
                    doingReorder = false;
                } else {
                    tag(pkt, seq, (unsigned char)seq);
                    seq++;
                }


                // tell our class there is memory here
                memory.markFull(pkt);

                // allocate another memory and overwrite
                pkt = memory.get();


                reordered_pkt = memory.getNext();
                while( reordered_pkt != 0 ) {
                    // pprintint(reordered_pkt);

                    unsigned int* word = (unsigned int*)reordered_pkt;

                    // in this test seq and first word are always the same
                    RC_ASSERT(word[0] == word[1]);

                    RC_ASSERT(word[0] == checkOrder);
                    checkOrder++;

                    memory.release(reordered_pkt);
                    reordered_pkt = memory.getNext();
                }

                prevWasReorder = doingReorder;
                runs++;
            } // while

            // std::stringstream buf;
            // buf << timesReordered << " Reorders";
            // RC_TAG(buf.str());

            RC_ASSERT(totalRuns == checkOrder);

            // cout << "Total Runs " << totalRuns <<  " Check order " << checkOrder << endl;

        }
        // cout << "dealoc" << endl;

    }); // rapid check
        exit(0);

}

int time1(void) {

    unsigned char* pkt;//[UDP_RX_BYTES];
    unsigned char* reordered_pkt;
    int rx_val;

    HiggsUdpOOO<UDP_RX_BYTES> memory;

    pkt = memory.get();

    int runs = 0;
    int totalRuns = 29000000;


    high_resolution_clock::time_point t1 = high_resolution_clock::now();


    while(runs < totalRuns) {

        rx_val = rrecv(pkt, 0);

        if( rx_val > 1) {
            // tell our class there is memory here
            memory.markFull(pkt);

            // allocate another memory and overwrite
            pkt = memory.get();
        }

        reordered_pkt = memory.getNext();
        while( reordered_pkt != 0 ) {
            // cout << "x" << endl;
            // pprint(reordered_pkt);
            memory.release(reordered_pkt);
            reordered_pkt = memory.getNext();
        }

        runs++;
    } // while

    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>( t2 - t1 ).count();

    cout << "ran " << totalRuns << " in " << duration  << " (" << duration / 1E6 << ")" << endl;

    cout << "about to destruct" << endl;

}

int main() {
    bool c3 = check3();
    bool c2 = check2();
    return !(c3 && c2);
}


void main4(void) {

    unsigned char* pkt;//[UDP_RX_BYTES];
    unsigned char* reordered_pkt;
    int rx_val;

    HiggsUdpOOO<UDP_RX_BYTES> memory;

    pkt = memory.get();

    int runs = 0;

    while(runs<1) {

        rx_val = rrecv(pkt);

        if( rx_val > 1) {
            // tell our class there is memory here
            memory.markFull(pkt);

            // allocate another memory and overwrite
            pkt = memory.get();
        }

        reordered_pkt = memory.getNext();
        while( reordered_pkt != 0 ) {
            // cout << "x" << endl;
            pprint(reordered_pkt);
            memory.release(reordered_pkt);
            reordered_pkt = memory.getNext();
        }

        runs++;
    }


}


void main3(void) {
    cout << "Test boot" << endl;

    unsigned char* pkt, *pkt2;//[UDP_RX_BYTES];
    unsigned char* reordered_pkt;

    HiggsUdpOOO<UDP_RX_BYTES> memory;



    pkt = memory.get();
    tag(pkt, 4, 'a');
    memory.markFull(pkt);
    pkt2 = memory.get();
    reordered_pkt = memory.getNext();
    pprint(reordered_pkt);
    memory.release(reordered_pkt);

    // inside the while
    reordered_pkt = memory.getNext();
    if(reordered_pkt) {
        pprint(reordered_pkt);
        memory.release(reordered_pkt);
    } 


    // pkt = memory.get();
    tag(pkt2, 5, 'b');
    memory.markFull(pkt2);
    reordered_pkt = memory.getNext();
    if(reordered_pkt) {
        pprint(reordered_pkt);
        memory.release(reordered_pkt);
    } else {
        cout << "skipping" << endl;
    }

}



void main2(void) {
    cout << "Test boot" << endl;

    unsigned char* pkt;//[UDP_RX_BYTES];
    unsigned char* reordered_pkt;

    HiggsUdpOOO<UDP_RX_BYTES> memory;



    pkt = memory.get();
    tag(pkt, 4, 'a');
    memory.markFull(pkt);
    reordered_pkt = memory.getNext();
    pprint(reordered_pkt);
    memory.release(reordered_pkt);


    pkt = memory.get();
    tag(pkt, 6, 'c');
    memory.markFull(pkt);
    reordered_pkt = memory.getNext();
    if(reordered_pkt) {
        pprint(reordered_pkt);
        memory.release(reordered_pkt);
    } else {
        cout << "skipping" << endl;
    }

    pkt = memory.get();
    tag(pkt, 5, 'b');
    memory.markFull(pkt);
    reordered_pkt = memory.getNext();
    if(reordered_pkt) {
        pprint(reordered_pkt);
        memory.release(reordered_pkt);
    } else {
        cout << "skipping" << endl;
    }



    reordered_pkt = memory.getNext();
    if(reordered_pkt) {
        pprint(reordered_pkt);
        memory.release(reordered_pkt);
    } else {
        cout << "skipping" << endl;
    }



    cout << endl;
    cout << endl;




}