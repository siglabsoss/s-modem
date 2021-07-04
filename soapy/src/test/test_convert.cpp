#include <rapidcheck.h>
#include "driver/AirPacket.hpp"

// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "common/convert.hpp"
#include <boost/functional/hash.hpp>

// instead of including data
std::vector<std::vector<uint32_t>> get_data();
std::vector<uint32_t> get_demodpacket0();
std::vector<uint32_t> get_demodpacket1();

using namespace std;

#define __ENABLED



unsigned bit_set(uint32_t v) {
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; v; c++)
    {
      v &= v - 1; // clear the least significant bit set
    }

    return c;
}



void set_p0(unsigned char* buf, int &nread) {
    int c = 0;
buf[c] = 69; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 84; c++;
buf[c] = 57; c++;
buf[c] = 165; c++;
buf[c] = 64; c++;
buf[c] = 0; c++;
buf[c] = 64; c++;
buf[c] = 1; c++;
buf[c] = 228; c++;
buf[c] = 253; c++;
buf[c] = 10; c++;
buf[c] = 2; c++;
buf[c] = 4; c++;
buf[c] = 2; c++;
buf[c] = 10; c++;
buf[c] = 2; c++;
buf[c] = 4; c++;
buf[c] = 1; c++;
buf[c] = 8; c++;
buf[c] = 0; c++;
buf[c] = 64; c++;
buf[c] = 12; c++;
buf[c] = 1; c++;
buf[c] = 68; c++;
buf[c] = 0; c++;
buf[c] = 2; c++;
buf[c] = 177; c++;
buf[c] = 6; c++;
buf[c] = 180; c++;
buf[c] = 91; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 134; c++;
buf[c] = 120; c++;
buf[c] = 12; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 16; c++;
buf[c] = 17; c++;
buf[c] = 18; c++;
buf[c] = 19; c++;
buf[c] = 20; c++;
buf[c] = 21; c++;
buf[c] = 22; c++;
buf[c] = 23; c++;
buf[c] = 24; c++;
buf[c] = 25; c++;
buf[c] = 26; c++;
buf[c] = 27; c++;
buf[c] = 28; c++;
buf[c] = 29; c++;
buf[c] = 30; c++;
buf[c] = 31; c++;
buf[c] = 32; c++;
buf[c] = 33; c++;
buf[c] = 34; c++;
buf[c] = 35; c++;
buf[c] = 36; c++;
buf[c] = 37; c++;
buf[c] = 38; c++;
buf[c] = 39; c++;
buf[c] = 40; c++;
buf[c] = 41; c++;
buf[c] = 42; c++;
buf[c] = 43; c++;
buf[c] = 44; c++;
buf[c] = 45; c++;
buf[c] = 46; c++;
buf[c] = 47; c++;
buf[c] = 48; c++;
buf[c] = 49; c++;
buf[c] = 50; c++;
buf[c] = 51; c++;
buf[c] = 52; c++;
buf[c] = 53; c++;
buf[c] = 54; c++;
buf[c] = 55; c++;

    nread = c;

}


void set_p1(unsigned char* buf, int &nread) {
    int c = 0;
buf[c] = 69; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 129; c++;
buf[c] = 179; c++;
buf[c] = 187; c++;
buf[c] = 64; c++;
buf[c] = 0; c++;
buf[c] = 64; c++;
buf[c] = 1; c++;
buf[c] = 106; c++;
buf[c] = 186; c++;
buf[c] = 10; c++;
buf[c] = 2; c++;
buf[c] = 4; c++;
buf[c] = 2; c++;
buf[c] = 10; c++;
buf[c] = 2; c++;
buf[c] = 4; c++;
buf[c] = 1; c++;
buf[c] = 8; c++;
buf[c] = 0; c++;
buf[c] = 232; c++;
buf[c] = 5; c++;
buf[c] = 84; c++;
buf[c] = 212; c++;
buf[c] = 0; c++;
buf[c] = 2; c++;
buf[c] = 136; c++;
buf[c] = 7; c++;
buf[c] = 180; c++;
buf[c] = 91; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 175; c++;
buf[c] = 50; c++;
buf[c] = 8; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 0; c++;
buf[c] = 16; c++;
buf[c] = 17; c++;
buf[c] = 18; c++;
buf[c] = 19; c++;
buf[c] = 20; c++;
buf[c] = 21; c++;
buf[c] = 22; c++;
buf[c] = 23; c++;
buf[c] = 24; c++;
buf[c] = 25; c++;
buf[c] = 26; c++;
buf[c] = 27; c++;
buf[c] = 28; c++;
buf[c] = 29; c++;
buf[c] = 30; c++;
buf[c] = 31; c++;
buf[c] = 32; c++;
buf[c] = 33; c++;
buf[c] = 34; c++;
buf[c] = 35; c++;
buf[c] = 36; c++;
buf[c] = 37; c++;
buf[c] = 38; c++;
buf[c] = 39; c++;
buf[c] = 40; c++;
buf[c] = 41; c++;
buf[c] = 42; c++;
buf[c] = 43; c++;
buf[c] = 44; c++;
buf[c] = 45; c++;
buf[c] = 46; c++;
buf[c] = 47; c++;
buf[c] = 48; c++;
buf[c] = 49; c++;
buf[c] = 50; c++;
buf[c] = 51; c++;
buf[c] = 52; c++;
buf[c] = 53; c++;
buf[c] = 54; c++;
buf[c] = 55; c++;
buf[c] = 56; c++;
buf[c] = 57; c++;
buf[c] = 58; c++;
buf[c] = 59; c++;
buf[c] = 60; c++;
buf[c] = 61; c++;
buf[c] = 62; c++;
buf[c] = 63; c++;
buf[c] = 64; c++;
buf[c] = 65; c++;
buf[c] = 66; c++;
buf[c] = 67; c++;
buf[c] = 68; c++;
buf[c] = 69; c++;
buf[c] = 70; c++;
buf[c] = 71; c++;
buf[c] = 72; c++;
buf[c] = 73; c++;
buf[c] = 74; c++;
buf[c] = 75; c++;
buf[c] = 76; c++;
buf[c] = 77; c++;
buf[c] = 78; c++;
buf[c] = 79; c++;
buf[c] = 80; c++;
buf[c] = 81; c++;
buf[c] = 82; c++;
buf[c] = 83; c++;
buf[c] = 84; c++;
buf[c] = 85; c++;
buf[c] = 86; c++;
buf[c] = 87; c++;
buf[c] = 88; c++;
buf[c] = 89; c++;
buf[c] = 90; c++;
buf[c] = 91; c++;
buf[c] = 92; c++;
buf[c] = 93; c++;
buf[c] = 94; c++;
buf[c] = 95; c++;
buf[c] = 96; c++;
buf[c] = 97; c++;
buf[c] = 98; c++;
buf[c] = 99; c++;
buf[c] = 100; c++;

    nread = c;
}

std::vector<uint32_t> get_word_p0() {
    std::vector<uint32_t> v = 
    {0x54000045,0x00409a93,0x088b0140,0x0104020a,0x0204020a,0x32ec0008,0x2f00f842,0x5bb442e1,0x00000000,0x000f3465,0x00000000,0x13121110,0x17161514,0x1b1a1918,0x1f1e1d1c,0x23222120,0x27262524,0x2b2a2928,0x2f2e2d2c,0x33323130,0x37363534,0x54000045,0x00409c93,0x00406b94,0x378a0140,0x0104020a,0x0204020a,0xc7850008,0x20009b60,0x5bb442e3,0x00000000,0x00010bba,0x00000000,0x13121110,0x17161514,0x1b1a1918,0x1f1e1d1c,0x23222120,0x27262524,0x2b2a2928,0x2f2e2d2c,0x33323130,0x37363534,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000};
    return v;
}

// this is EXTREMELY inefficient
static void old_unwrap_phase_inplace(std::vector<double>&cfo_angle) {
    const unsigned wanted = cfo_angle.size();
    for(unsigned index = 1; index < wanted; index++)
    {
        if(cfo_angle[index]-cfo_angle[index-1]>=M_PI)
        {
            for(unsigned index1 = index; index1 < wanted; index1++)
            {
                cfo_angle[index1] = cfo_angle[index1] - 2.0*M_PI;
            }
        }
        else if(cfo_angle[index]-cfo_angle[index-1]<M_PI*(-1.0))
        {
            for(unsigned index1 = index; index1 < wanted; index1++)
            {
                cfo_angle[index1] = cfo_angle[index1] + 2.0*M_PI;
            }
        }
    }
}


#define BUFSIZE 2000

int main() {


    rc::check("gain_ishort is correct", [](void) {
        uint32_t sample = 0x04000800;
        uint32_t sout;
        bool saturation;
        gain_ishort(0.75, sample, sout, saturation);

        // cout << "Gained to " << HEX32_STRING(sout) << "\n";

        RC_ASSERT(saturation == false);
        RC_ASSERT(sout == 0x03000600);

    });

    rc::check("gain_ishort_vector is correct", [](void) {
        
        std::vector<uint32_t> inn = {0x04000300, 0x04000800, 0x0f000f00, 0xf000f000};


        unsigned saturation;
        auto out = gain_ishort_vector(30.0, inn, saturation);

        // for(auto w : out) {
        //     cout << "Gained to " << HEX32_STRING(w) << "\n";
        // }
        // cout << saturation << " samples saturated\n";

        RC_ASSERT(saturation == 3);

    });

    rc::check("ComplexToIShortMulti", [](uint32_t a_word) {

        std::vector<uint32_t> out;

        std::vector<std::complex<double>> ain;


        ain.emplace_back(0,0);
        ain.emplace_back(0.5,0.5);
        ain.emplace_back(0.5,-0.5);
        ain.emplace_back(-0.5,0.5);
        ain.emplace_back(-0.5,-0.5);

        complexToIShortMulti(out, ain.data(), ain.size());
        complexToIShortMulti(out, (unsigned char*)ain.data(), ain.size());
        complexToIShortMulti(out, ain);

        for(auto w : out) {
            cout << HEX32_STRING(w) << endl;
        }

        // exit(0);
    });

    // usleep(1000*5);
    std::vector<uint32_t> atan_results = _file_read_hex("../js/test/data/atan_in_out_0.hex");

    rc::check("riscv ATAN", [atan_results](uint32_t a_word) {

    auto len = atan_results.size();

        for(size_t i = 0; i < len; i+=2) {
          auto input = atan_results[i];
          auto expected_output = atan_results[i+1];
          (void) expected_output;

          auto node_output = riscvATAN(input);
          (void) node_output;


          // console.log('------------');
          // console.log(expected_output.toString(16));
          // console.log(node_output.toString(16));
          // console.log('');

          // cout << "------------" << endl;
          // cout << HEX32_STRING(expected_output) << endl;
          // cout << HEX32_STRING(node_output) << endl << endl;


        }



        // exit(0);
    });

    rc::check("riscv ATAN negative", [atan_results](uint32_t a_word) {

         auto len = atan_results.size();
         (void)len;

        // for(size_t i = 0; i < len; i+=2) {
          auto input = std::complex<double>(-34.9227,-27.1151);
          // auto expected_output = atan_results[i+1];

          auto node_output = riscvATANComplex(input);


          // console.log('------------');
          // console.log(expected_output.toString(16));
          // console.log(node_output.toString(16));
          // console.log('');

          cout << "------------" << endl;
          // cout << HEX32_STRING(expected_output) << endl;
          cout << HEX32_STRING(node_output) << endl << endl;
          cout << (node_output&0xffff) << endl;


        // }



    });

    rc::check("complexConjMult", [](uint32_t a_word) {

        std::vector<std::complex<double>> a;
        std::vector<std::complex<double>> b;

        std::vector<uint32_t> araw;

        araw.push_back(0x00001000);
        araw.push_back(0x00002000);
        araw.push_back(0x00003000);
        araw.push_back(0x00004000);
        araw.push_back(0x00005000);


        std::vector<uint32_t> braw;

        braw.push_back(0x90007000);
        braw.push_back(0x90007000);
        braw.push_back(0x90007000);
        braw.push_back(0x90007000);
        braw.push_back(0x90007000);

        IShortToComplexMulti(a, araw.data(), araw.size());
        IShortToComplexMulti(b, braw.data(), braw.size());

        std::vector<std::complex<double>> res;

        complexConjMult(res, a, b);


        // for(auto w : a) {
        //     cout << w << endl;
        // }
        // for(auto w : b) {
        //     cout << w << endl;
        // }
        // for(auto w : res) {
        //     cout << w << endl;
        // }

        // exit(0);
    });



    // RC_ASSERT(false);
    rc::check("IShortToComplexMulti", [](uint32_t a_word) {

        std::vector<std::complex<double>> out;

        std::vector<uint32_t> ain;

        ain.push_back(0x00000001);
        ain.push_back(0x7fff0000);
        ain.push_back(0x7fff00ff);

        IShortToComplexMulti(out, ain.data(), ain.size());

        // for(auto w : out) {
        //     cout << w << endl;
        // }

        // exit(0);
    });


    rc::check("erase and demod", [](uint32_t a_word) {
        auto p = get_word_p0();

        int erase = 0;
        int loops = 0;

        while(erase != -1) {

            // cout << "loops: " << loops << endl;
            // cout << HEX32_STRING(p[0]) << endl;
            // cout << HEX32_STRING(p[1]) << endl;
            // cout << HEX32_STRING(p[2]) << endl;
            // cout << HEX32_STRING(p[3]) << endl;
            // cout << HEX32_STRING(p[4]) << endl;
            // cout << endl;

            auto o = packetToBytes(p, erase);

            // cout << "sz " << o.size() << endl;

            // cout << "erase " << erase << endl;

            if( erase > 0 ) {
                p.erase(p.begin(), p.begin()+erase);
            }

            loops++;

            // cout << endl << endl << endl;
        }

        // exit(0);

    });

    rc::check("vec inout p0", [](uint32_t a_word) {
        unsigned char buffer[BUFSIZE];

        std::vector<unsigned char> char_vec;

        int nread;
        set_p0(buffer, nread);

        // RC_ASSERT(nread == 129);

        char_vec.resize(nread);

        for(int i = 0; i < nread; i++)
        {
            char_vec[i] = buffer[i];
            // cout << (int) buffer[i] << endl;
        }


        auto conv_vec = cBufferPacketToCharVector(buffer, BUFSIZE, nread);

        RC_ASSERT(conv_vec == char_vec);

        // convert into words
        std::vector<uint32_t> conv2 = charVectorPacketToWordVector(conv_vec);

        // convert back to bytes
        std::vector<unsigned char> conv3 = wordVectorPacketToCharVector(conv2);

        RC_ASSERT( conv3 == conv_vec );


        // for(int i = 0; i < conv3.size(); i++)
        // {
        //     // char_vec[i] = buffer[i];
        //     cout << (int) conv3[i] << endl;
        // }

    });



    rc::check("vec inout", [](uint32_t a_word) {
        unsigned char buffer[BUFSIZE];

        std::vector<unsigned char> char_vec;

        // boost::hash<std::vector<unsigned char>> char_hasher;
        // boost::hash<std::vector<uint32_t>> word_hasher;

        int nread;
        set_p1(buffer, nread);

        RC_ASSERT(nread == 129);

        char_vec.resize(nread);

        for(int i = 0; i < nread; i++)
        {
            char_vec[i] = buffer[i];
            // cout << (int) buffer[i] << endl;
        }


        auto conv_vec = cBufferPacketToCharVector(buffer, BUFSIZE, nread);

        RC_ASSERT(conv_vec == char_vec);

        // convert into words
        std::vector<uint32_t> conv2 = charVectorPacketToWordVector(conv_vec);

        // convert back to bytes
        std::vector<unsigned char> conv3 = wordVectorPacketToCharVector(conv2);

        RC_ASSERT( conv3 == conv_vec );


        // for(int i = 0; i < conv3.size(); i++)
        // {
        //     // char_vec[i] = buffer[i];
        //     cout << (int) conv3[i] << endl;
        // }

    });

    rc::check("check phase unwrap", [](uint32_t a_word) {
        std::vector<double> input = {1.0, 2.0, 3.0, -3.0, -2.0, -1.0, 0.0};


        const auto countadd = *rc::gen::inRange(10u, 10000u);

        double value = 0;
        for(int i = 0; i < countadd; i++) {
            input.push_back(value);
            double add;
            add = (*rc::gen::inRange(-1000, 1000)) / 500.0;
            value += add;
        }


        std::vector<double> ta = input;
        std::vector<double> tb = input;

        old_unwrap_phase_inplace(ta);

        // for( auto x : ta ) {
        //     cout << x << "\n";
        // }
        // cout << "\n\n";

        unwrap_phase_inplace(tb);


        RC_ASSERT(ta.size() == tb.size());

        // for( auto x : tb ) {
        //     cout << x << "\n";
        // }
        // cout << "\n\n";

        double delta;
        for(int i = 0; i < ta.size(); i++) {
            delta = abs(ta[i]-tb[i]);
            // cout << "delta " << delta << "\n";
            RC_ASSERT(delta < 0.0001);
        }

        // exit(0);

    });


  return 0;
}

