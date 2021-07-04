#include "ParseReveal.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <stdint.h>

using namespace std;

namespace siglabs {
namespace reveal {

static void _parse(std::vector<char> v) {

    for(auto c : v) {
        cout << c;
    }
}

static void _tok_line_num(ifstream& myfile, const unsigned val) {
    std::string j1;

    // cout << "foo:" << endl;
    auto s = std::to_string(val);

    for( auto c : s ) {
        // cout << "looking for " << c << endl;
        std::getline(myfile, j1, c);
        // cout << "got: " << j1 << endl;
    }
}

// these can be removed if _val is reversed
// https://stackoverflow.com/questions/9144800/c-reverse-bits-in-unsigned-integer
static uint8_t reverse_u8(uint8_t x)
{
   const unsigned char rev[] = "\x0\x8\x4\xC\x2\xA\x6\xE\x1\x9\x5\xD\x3\xB\x7\xF";
   return rev[(x & 0xF0) >> 4] | (rev[x & 0x0F] << 4);
}
static uint16_t reverse_u16(uint16_t x)
{
   return reverse_u8(x >> 8) | (reverse_u8(x & 0xFF) << 8);
}
static uint32_t reverse_u32(uint32_t x)
{
   return reverse_u16(x >> 16) | (reverse_u16(x & 0xFFFF) << 16);
}


static uint32_t _val(const std::string& val) {
    const size_t width = val.size();
    // cout << "width: " << width << " val: " << val << " (" << val.size() << ")" << endl;

    uint32_t out = 0;

    for(int i = 0; i < width; i++) {
        out <<= 1;
        // bool last == i+1 == width;
        // char c = 
        // cout << val[i] << endl;

        switch(val[i]) {
            case '0':
                break;
            case '1':
                out |= 1;
                break;
            default:
                cout << "Illegal character found in bits '" << val[i] << "' (" << (int) val[i] << ")" << endl;
                break;
        }

    }

    uint32_t out2 = reverse_u32(out);;
    if( width == 1 ) {
        out2 = out2 ? 1 : 0;
    }

    return out2;
}


std::vector<std::vector<uint32_t>> parse_reveal_dump(std::string filename, const unsigned header_lines, const size_t signals, const bool print) {
    ifstream myfile;
    myfile.open(filename, ios::binary);
    // std::vector<char> out;

    std::string j1;
    std::string j2;
    std::string line1;
    std::string val1;

    bool eq_found = false;
    int count_eq = 0;

    for(int i = 0; i < 1024; i++) {
        std::getline(myfile, j1, '=');
        if( j1.size() > 1000 ) {
            eq_found = true;
            break;
        }
        count_eq++;
        // cout << j1.size() << endl;
    }

    if( !eq_found ) {
        cout << "something went wrong finding end of ==\n";
        return {};
    } else {
        // cout << "count_eq was " << count_eq << endl;
    }

    myfile.close();
    myfile.open(filename, ios::binary);

    for(int i = 0; i < count_eq; i++) {
        std::getline(myfile, j1, '=');
    }

    // while(){
    //     // cout << "junk\n";
    //     cout << j1;
    // }

    std::getline(myfile, j2, '\n');
    for(int i = 0; i < header_lines; i++) {
        std::getline(myfile, j2, '\n');
    }

    // std::getline(myfile, line1, '\n');

    // std::vector<size_t> widths = {1,32,1,32,1,1,1,32};
    // size_t signals = 8;
    unsigned buffer_size = 1024;

    // cout << std::hex;
    std::vector<std::vector<uint32_t>> res;
    res.resize(buffer_size);

    for(int line = 0; line < buffer_size; line++) {
        _tok_line_num(myfile, line);

        std::getline(myfile, j1, '+');
        std::getline(myfile, j1, '0');
        // cout << j1 << endl;

        for( int i = 0; i < signals; i++ ) {
            std::getline(myfile, val1, ' ');
            std::getline(myfile, val1, ' ');
            // cout << val1 << endl;
            auto parsed = _val(val1);
            if( print ) {
                cout << parsed << ",";
            }

            res[line].push_back(parsed);

            // std::getline(myfile, val1, ' ');
            // std::getline(myfile, val1, ' ');
            // cout << val1 << endl;


            // std::getline(myfile, val1, ' ');
            // std::getline(myfile, val1, ' ');
            // cout << val1 << endl;
        }
        if(print) {
            cout << endl;
        }
        std::getline(myfile, j1, '\n');
    }
    // cout << line1 << endl;


    return res;

}


} // namespace
} // namespace
