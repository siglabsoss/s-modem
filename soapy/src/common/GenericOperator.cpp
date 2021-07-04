#include "GenericOperator.hpp"
#include <iostream>
#include <iomanip>

namespace siglabs {
namespace rb {

#define HEX32_STRING(n) std::setfill('0') << std::setw(8) << std::hex << (n) << std::dec

using namespace std;

/// 
/// @param[in] operation : one of zero,add,sub,set,get
/// @param[in] _sel : Select which variable to perform an operation on
/// @param[in] _val : Argument to the operation
/// @param[in] _rb_type : Ringbus Mask.  pass 0 to add this on the outside
std::vector<uint32_t> op(const std::string operation, const uint32_t _sel, const uint32_t _val, const uint32_t _rb_type) {

    uint32_t op;
    if( operation == "zero" ) {
        op = 0;
    } else if( operation == "add" ) {
        op = 1;
    } else if( operation == "sub" ) {
        op = 2;
    } else if( operation == "set" ) {
        op = 3;
    } else if( operation == "get" ) {
        op = 4;
    } else {
        cout << "op() called with illegal operation " << operation << "\n";
        op = 4;
    }

    uint32_t val_low = _val & 0xffff;
    uint32_t val_high = (_val >> 16) & 0xffff;


    uint32_t sel = _sel & 0xf;

    uint32_t op_sel      = (op | (sel<<4)) << 16;
    uint32_t op_sel_high = op_sel | 0x80000;
    
    uint32_t r0 = op_sel      | val_low  | _rb_type;
    uint32_t r1 = op_sel_high | val_high | _rb_type;


    return {r0,r1};

}



void DispatchGeneric::gotWord(const uint32_t w) {
    if( cb == 0 ) {
        return;
    }
    const uint32_t type = w & 0xff000000;
    if( type == mask ) {
        gotMatchingWord(w);
    }
}

void DispatchGeneric::gotMatchingWord(const uint32_t data) {
    if(print) {
        cout << "Got masked type " << HEX32_STRING(data) << "\n";
    }

    uint32_t dmode = (data >> 16) & 0xff; // 8 bits
    uint32_t data_16 = data & 0xffff; // 16 bits

    if(print) {
        cout << "dmode  " << HEX32_STRING(dmode) << "\n";
        cout << "data16 " << HEX32_STRING(data_16) << "\n";
    }

    uint32_t op   = dmode&0x7;        // 3 bits
    uint32_t high = (dmode&0x8) >> 3; // 1 bit
    uint32_t sel  = (dmode&0xf0)>>4; // 4 bits

    uint32_t op_sel = dmode&(0x7|0xf0); // op and sel put together

    if(print) {
        cout << "op    " << HEX32_STRING(op) << "\n";
        cout << "high  " << HEX32_STRING(high) << "\n";
        cout << "sel   " << HEX32_STRING(sel) << "\n";
    }

    if(high == 0) {
        schedule_epoc_storage = data_16;
        schedule_epoc_storage_op = (uint32_t)op_sel;
        return;
    }


    if((uint32_t)schedule_epoc_storage_op != op_sel) {
        // got back to back non matching ops. dropping
        schedule_epoc_storage_op = -1;
        return;
    }

    if( op != 4 ) {
        cout << "Got called with op " << op << " which is illegal\n";
        schedule_epoc_storage_op = -1;
        return;
    }

    uint32_t full_value = ((data_16<<16)&0xffff0000) | (schedule_epoc_storage);

    cb(sel, full_value);
}

void DispatchGeneric::setCallback(op_get_cb_t _cb, uint32_t _mask) {
    cb = _cb;
    mask = _mask;
}


}
}