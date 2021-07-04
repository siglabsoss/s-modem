#include "CoarseSync.hpp"
#include <iostream>
#include <cassert>
#include "CoarseData.hpp"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

namespace BevStream {

using namespace std;

CoarseSync::CoarseSync(std::vector<uint32_t> *_coarseResults, std::mutex *m, bool print) : BevStream(true,print), coarseMutex(m), coarseResults(_coarseResults) {
  name = "CoarseSync";

  buildVectors();
}

CoarseSync::~CoarseSync() {
  if(_print) {
    cout << "~CoarseSync()" << endl;
  }
}

void CoarseSync::buildVectors() {
  zeros.reserve(sz);
  for(size_t i = 0; i < sz; i++) {
    zeros.emplace_back(0,0);
  }

  auto expLen = ARRAY_SIZE(exp_data_1280_ext_15);
  IShortToComplexMulti(expCplxWhole, exp_data_1280_ext_15, expLen);

  exp.resize(5);

  for(size_t i = 0; i < 5; i++) {
    // let bufferIndex = this.getExpIndexArg(i+1);
    auto bufferOffset = getExpOffsetArg(i+1);

    // this.exp[i] = exp_source.slice(bufferOffset, bufferOffset+this.sz);
    auto first = expCplxWhole.begin() + bufferOffset;
    auto last = expCplxWhole.begin() + bufferOffset+sz;
    std::copy(first, last, std::back_inserter(exp[i]));
  }


  // for(size_t i = 0; i < expLen; i++) {
  //   expCplxWhole.pu
  // }




}


void CoarseSync::stopThreadDerivedClass() {
  // perform any gain stream specific tasks
}

// runs from same place (thread) that constructed us
void CoarseSync::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, byte_sz, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

void CoarseSync::_resetCoarseSyncMembers() {
  // this.chunk0 = undefined;
  triggered_frame = 0; // how many frames into this trigger are we looking at
  // this.cumulative_sum = mt.complex(0,0);
}

void CoarseSync::triggerCoarse() {
  if( pending_trigger ) {
    cout << "Warning: pending_trigger was already set. dropping the coarse sync request" << endl;
  }
  pending_trigger = true;
}

void CoarseSync::triggerCoarseAt(size_t frame) {
  if( pending_trigger_at ) {
    cout << "Warning: pending_trigger_at was already set for " << pending_trigger_at_frame << ". dropping the coarse sync request for " << frame << " now its " << lifetime_frame << endl;
    return;
  }
  pending_trigger_at = true;
  pending_trigger_at_frame = frame;
}

// repeatedly called, but only if there are 1024 uint32_t values
void CoarseSync::gotDataCore(struct bufferevent *bev, struct evbuffer *buf, unsigned char* chunk1_raw, size_t lengthIn) {

  // cout << "gotDataCore" << endl;
  if( !doing_coarse && pending_trigger ) {
    doing_coarse = true;
    pending_trigger = false;
    _resetCoarseSyncMembers();
    if( print4 ) {
      cout << "Trigger began at frame " << lifetime_frame << endl;
    }
  }

  if( !doing_coarse && pending_trigger_at ) {
    if( pending_trigger_at_frame < lifetime_frame ) {
      cout << "dumping pending_trigger_at_frame of " << pending_trigger_at_frame << " which is too late for " << lifetime_frame << endl;
      pending_trigger_at = false;
    } else if(pending_trigger_at_frame == lifetime_frame) {
      pending_trigger_at = false;
      doing_coarse = true;
      _resetCoarseSyncMembers();
      if( print4 ) {
        cout << "Executing Pending Trigger at frame " << lifetime_frame << endl;
      }
    }
  }


  if( doing_coarse ) {

    if(triggered_frame == 0) {
      IShortToComplexMulti(chunk0, reinterpret_cast<uint32_t*>(chunk1_raw), sz);
      // chunk0 = convBufferComplex(chunk1_raw);

      triggered_frame++;
      lifetime_frame++;
      bufferevent_write(next->input, zeros.data(), zeros.size()*4);
      // push(zeros);
      // cb();
      return;
    }

    std::vector<std::complex<double>> chunk1;
    std::vector<std::complex<double>> ym;
    std::vector<std::complex<double>> am_inner;

    IShortToComplexMulti(chunk1, reinterpret_cast<uint32_t*>(chunk1_raw), sz);
    // let chunk1 = convBufferComplex(chunk1_raw);


    complexConjMult(ym, chunk0, chunk1);
    // let ym = conj_mult(chunk0,chunk1);

    // console.log(`ym is ${ym.length} and ${ym[0].toString()}`);

    auto expBuffer = getExpBuffer();

    // if(print2) {
    //   // mutil.printHexNl(chunk0_u32.slice(0,8), 'input_conj_multi_location_0 ' + triggered_frame);
    //   // mutil.printHexNl(chunk1_u32.slice(0,8), 'input_conj_multi_location_1 ' + triggered_frame);
    //   mutil.printHexNl(mutil.complexToIShortMulti(ym.slice(0,8)), 'output_conj_multi_location ' + triggered_frame);
    //   mutil.printHexNl(mutil.complexToIShortMulti(expBuffer.slice(0,8)), 'exp_data ' + triggered_frame);
    // }

    // this is the value inside the the sigma summation operator
    // let am_inner = mult(ym, expBuffer);
    complexMult(am_inner, ym, *expBuffer);

    // static int runonce = 0;
    // if(runonce == 0) {
    //   for(auto x : am_inner) {
    //     cout << x << endl;
    //   }
    // }
    // runonce++;


    // this represents a_m
    // let sum = mt.complex(0,0);
    auto sum = std::complex<double>(0,0);
    for(size_t i = 0; i < sz; i++) {
      // sum = mt.add(sum, am_inner[i]);
      sum += am_inner[i];
    }


    // add to the running sum
    // cumulative_sum = cumulative_sum.add(sum);
    cumulative_sum += sum;


    if(print1) {
      // console.log(`coarse: ${sum.arg()} - ${sum.toString()}`);
      cout << "coarse: " << std::arg(sum) << " - " << sum << endl;
    }

    // console.log(`${triggered_frame} had angle: ${cumulative_sum.arg()}`);
    // console.log(`${triggered_frame} runs offset: ${getExpOffset().toString(16)}`);

    if( triggered_frame == (ofdmNum-1) ) {
      if( print3 ) {
        // console.log(`finishing up, angle is ${cumulative_sum.arg()}`);
        cout << "finishing up, angle is " << std::arg(cumulative_sum) << " -> " << cumulative_sum << endl;
      }

      uint32_t riscv_angle_mag = riscvATANComplex(cumulative_sum);

      uint32_t temp_angle = riscv_angle_mag&0xffff;
      uint32_t temp_offset = ((((temp_angle+6553)&0xffff)*1280)>>16);

      // _sync_callback(temp_offset);
      callCoarseSyncCallback(temp_offset);
      // cout << "call _sync_callback " << temp_offset << endl;
      doing_coarse = false;

    }


    // chunk0 = chunk1;
    chunk0 = std::move(chunk1);

    triggered_frame++;
    lifetime_frame++;

    // push zeros
    // this was the original idea for cs00 but we scrapped it due to code complexity
    // push(zeros);
    bufferevent_write(next->input, zeros.data(), zeros.size()*4);

  } else {
      // we are not in coarse, pass through


      lifetime_frame++;

      // this.push(chunk1_raw);
      bufferevent_write(next->input, chunk1_raw, byte_sz);  // FIXME should be this
      // bufferevent_write(next->input, zeros.data(), zeros.size()*4);
      // cb();
    }
}

void CoarseSync::callCoarseSyncCallback(uint32_t val) {
  {
    std::lock_guard<std::mutex> lk(*this->coarseMutex);
    coarseResults->push_back(val);
  }
}

// runs on _thread
void CoarseSync::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  if(_print) {
    cout << name << " got data with " << lengthIn << endl;
  }

  if(!next) {
    evbuffer_drain(buf, lengthIn);
    return;
  }

  // round down
  size_t willEatFrames = lengthIn / byte_sz;

  // cout << "will eat " << willEatFrames << " frames" << endl;

  size_t this_read = willEatFrames*byte_sz;

  // cout << "this_read " << this_read << " len " << lengthIn << endl;

  unsigned char* temp_read = evbuffer_pullup(buf, this_read);

  size_t consumed = 0;
  while((consumed+byte_sz) <= lengthIn) {
    auto read_at = temp_read+consumed;
    // cout << (void*)(read_at) << endl;
    gotDataCore(bev, buf, read_at, lengthIn);
    consumed += byte_sz;
  }

  evbuffer_drain(buf, this_read);
}



// returns index of buffer
size_t CoarseSync::getExpIndexArg(int arg) {
  auto key = std::max(arg-1,0);
  auto offset = (5-(key%5));
  return offset;
}

// returns offset in to exp_data_1280_ext_15
size_t CoarseSync::getExpOffsetArg(int arg) {
  auto offset = getExpIndexArg(arg);
  auto ret = offset * 0x100;
  return ret;
  // expected results, starting at index 1
  // 0x500
  // 0x400
  // 0x300
  // 0x200
  // 0x100
  // 0x500
  // 0x400
}

// how far into the exponential should we offset?
// returns offset into exp_data_1280_ext_15 based on this.triggered_frame
// calling this with triggered_frame == 0 is illegal, so we return the value of triggered_frame = 1 in that case
size_t CoarseSync::getExpOffset() {
  return getExpOffsetArg(triggered_frame);
}

std::vector<std::complex<double>>* CoarseSync::getExpBuffer() {
  size_t index = (triggered_frame-1) % 5;
  // console.log(`${this.triggered_frame} maps to ${index}`);
  return &(exp[index]);
}




} // namespace