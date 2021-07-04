"use strict";

const _ = require('lodash');
const mutil = require('./mockutils.js');
const util = require('util');
const Bank = require('../mock/typebank.js');
const Stream = require("stream");
// var assert = require('assert');
const mt = require('mathjs');
const estimators = require('../mock/estimators.js');

const hc = require('./hconst.js');

const FBParse = require("../mock/fbparse.js");

// see test_mover_8/cs21
const DMA_IN_EXTRA = (16);
const DMA_IN_EXTRA_USED = (4);


const MAPMOV_MODE_128_CENTERED_LINEAR_REVERSE_MOVER = [
1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31,
33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63,
65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95,
97, 99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127,
897, 899, 901, 903, 905, 907, 909, 911, 913, 915, 917, 919, 921, 923, 925, 927,
929, 931, 933, 935, 937, 939, 941, 943, 945, 947, 949, 951, 953, 955, 957, 959,
961, 963, 965, 967, 969, 971, 973, 975, 977, 979, 981, 983, 985, 987, 989, 991,
993, 995, 997, 999, 1001, 1003, 1005, 1007, 1009, 1011, 1013, 1015, 1017, 1019, 1021, 1023
];






class Tagger extends Stream.Transform {
  constructor(options, options2){
    super(options)

    const { 
       printSlicer = false
      ,reverseMoverConfig = MAPMOV_MODE_128_CENTERED_LINEAR_REVERSE_MOVER
      ,onMapmovData = null
      ,onFrameIShort = null
    } = options2;

    this.printSlicer = printSlicer;
    this.reverseMoverConfig = new Set(reverseMoverConfig); // convert to a set for faster lookups
    this.onMapmovData = onMapmovData;
    this.onFrameIShort = onFrameIShort;
    // Object.assign(this, options2);

    this.typeBytes = 8;
    this.sz = 1024;
    this.byte_sz = 1024 * this.typeBytes;

    this.uint = new Bank.TypeBankUint32().access();

    this.uint.frame_track_counter = 0;
    this.frame_num_all_output_period = 1024;
    this.frame_num_all_output_counter = 0;

    this.moverSettings();

    this.setupFeedbackBusHeaders();

  }

  setupFeedbackBusHeaders() {
    this.stream_default = new FBParse.FeedbackStream( hc.FEEDBACK_PEER_SELF, false, true, hc.FEEDBACK_STREAM_DEFAULT );
    this.stream_all_sc = new FBParse.FeedbackStream( hc.FEEDBACK_PEER_SELF, false, true, hc.FEEDBACK_STREAM_ALL_SC );

    this.vec_fine_sync = new FBParse.FeedbackVector( hc.FEEDBACK_PEER_SELF, false, true, hc.FEEDBACK_VEC_FINE_SYNC );
    this.vec_demod_data = new FBParse.FeedbackVector( hc.FEEDBACK_PEER_SELF, false, true, hc.FEEDBACK_VEC_DEMOD_DATA );

    this.vec_fine_sync.setLength(DMA_IN_EXTRA_USED)

// #ifdef DONT_SLICE_DATA
    // set_feedback_vector_length(&vec_demod_data, 128);
// #else
    // set_feedback_vector_length(&vec_demod_data, 128/16); // 64
    this.vec_demod_data.setLength(128/16);
// #endif

    //void set_stream_length(unsigned int len) {

    this.stream_default.setLength(this.enabled_subcarriers);
    this.stream_all_sc.setLength(1024);

    // set_feedback_stream_length(&stream_default, this.enabled_subcarriers);
    // set_feedback_stream_length(&stream_all_sc, 1024); // length is all subcarriers

  
  }

  moverSettings() {
    this.enabled_subcarriers = 64;
    this.number_active_schedules = 4;
  }

  adjustLifetimeCounter(v) {
    this.frame_track_counter += v;
  }
  

  handleFineSync(iFloat,iShort,asComplex) {

    // work of cs01
    let temp_angle = estimators.calculateSfoUser0(asComplex);
    let temp_angle_1 = estimators.calculateSfoUser0(asComplex);

    // work of cs11 (simply extract sc)
    let one_pilot_observe = iShort[2];
    let second_pilot_observe = iShort[4];

    // every frame
    // build vec fine sync
    let fine_sync_body = [
      temp_angle,
      one_pilot_observe,
      temp_angle_1,
      second_pilot_observe
    ];

    this.vec_fine_sync.seq = this.uint.frame_track_counter;
    this.push(mutil.IShortToStreamable(this.vec_fine_sync));
    this.push(mutil.IShortToStreamable(fine_sync_body));

  }

  handleAllSubcarriers(iFloat,iShort,asComplex) {
    let runThisTime = false;

    if( this.frame_num_all_output_counter === this.frame_num_all_output_period ) {
        runThisTime = true;
        this.frame_num_all_output_counter = 0;
    }
    
    this.frame_num_all_output_counter++;
    // console.log(this.frame_num_all_output_counter);

    if( runThisTime ) {
      this.stream_all_sc.seq = this.uint.frame_track_counter;
      this.push(mutil.IShortToStreamable(this.stream_all_sc));
      this.push(mutil.IShortToStreamable(iShort));
    }
  }

  handleDefaultStream(iFloat,iShort,asComplex) {
    // stream_default.seq = frame_track_counter;
    // dma_block_send(VMEM_DMA_ADDRESS(&stream_default), FEEDBACK_HEADER_WORDS);
    // dma_block_send(DST_ROW_REV*NSLICES, enabled_subcarriers*FRAME_MOVE_CHUNK);

    // 64 contiguous subcarriers
    let start = 0;
    let end = this.enabled_subcarriers;

    this.stream_default.seq = this.uint.frame_track_counter;
    this.push(mutil.IShortToStreamable(this.stream_default));
    this.push(mutil.IShortToStreamable(iShort.slice(start,end)));
  }

  // this funciton performs the work of reverse mapmov (vmem version) as well as the slicer
  // we also have a "tap" which is onMapmovData which skipps the output stream and fbbus tags
  handleDemodData(iFloat,iShort,asComplex) {
    // if( this.printSlicer ) {
    //   console.log('enter demodData ' + iShort.length + ' ' + asComplex.length);
    // }

    // this is 
    // output of reverse mover, named reverse_mover in AirPacket.cpp
    let moverOut = [];

    for( let i = 0; i < iShort.length; i++ ) {
      let mod = i % 1024;

      let found = this.reverseMoverConfig.has(mod);

      if( found ) {
        moverOut.push(iShort[i]);
      }

      // console.log("mod " + mod + " has " + found);

      // auto it = find (reverse_mover.begin(), reverse_mover.end(), mod);
      // if (it != reverse_mover.end()) {
      //     vmem_mover_out.push_back(rx_out[i]);
      // }
    }

    if( this.printSlicer ) {
      // for( let x of moverOut ) {
      //   console.log(x.toString(16));
      // }
    }

    let sliced_out = [];

    let word = 0;
    for(let i = 0; i < moverOut.length; i++) {
        let j = i % 16;
        let input = moverOut[i];

        let bits = (( ( (input >>> 30) & 0x2 ) >>> 0 ) | (( (input >>> 15) & 0x1 )>>>0))>>>0;
        word = (((word >>> 2)) | ((bits<<30)))>>>0;

        // console.log(`j: ${j} ${word.toString(16)} ${input.toString(16)}`);

        if( j == 15 ) {
            sliced_out.push(word);
            // std::cout << HEX32_STRING(word) << std::endl;
            word = 0;
        }
    }

    // for(let x of sliced_out) {
    //   // console.log(x.toString(16));
    //   console.log(mutil.bitMangleSliced(x).toString(16));
    // }

    if( this.onMapmovData !== null ) {
      this.onMapmovData(sliced_out);
    }

    if( this.printSlicer ) {
      for(let i = 0; i < sliced_out.length; i++) {
        let j = mutil.transformWords128(i);
        console.log(mutil.bitMangleSliced(sliced_out[j]).toString(16) + "   fbtag");
      }
    }

    // increment header seq, send header and body
    this.vec_demod_data.seq = this.uint.frame_track_counter;
    this.push(mutil.IShortToStreamable(this.vec_demod_data));
    this.push(mutil.IShortToStreamable(sliced_out));
  }

  // chunk is C64 which means
  // [real, imag, real, imag]   as floats
  //   8      8     8     8     as bytes
  _transform(chunk,encoding,cb) {

    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    let iFloat = Array(this.sz*2);
    let asComplex = Array(this.sz);


    // for(let i = 0; i < this.sz*2; i+=1) {
    //   iFloat[i] = dataView.getFloat64(i*8, true);
    // }

    for(let i = 0; i < this.sz*2; i+=2) {
      let re = dataView.getFloat64(i*8, true);
      let im = dataView.getFloat64((i*8)+8, true);

      asComplex[i/2] = mt.complex(re,im);

      iFloat[i] = re;
      iFloat[i+1] = im;

      // if( i < 16 ) {
      //   console.log(re);
      // }

    }

    let iShort = mutil.IFloatToIShortMulti(iFloat);

    if( this.onFrameIShort !== null ) {
      this.onFrameIShort(iShort);
    }

    // console.log("iFloat length " + iFloat.length);
    // console.log(iFloat.slice(0,16));

    // console.log("iShort length " + iShort.length);
    // console.log(iShort.slice(0,16));


    this.handleDefaultStream(iFloat,iShort,asComplex);
    this.handleFineSync(iFloat,iShort,asComplex);
    this.handleAllSubcarriers(iFloat,iShort,asComplex);
    this.handleDemodData(iFloat,iShort,asComplex);


    // for(x of iFloat) {
    //   console.log(x);
    // }


    this.uint.frame_track_counter++;
    cb();
  }
}

// cs00 outputs 1024.  fpga's past that add on a 16 fixed length section
// note this is NOT feedback bus

// cs01 adds on sfo angles like this:
// 0000e000     temp_angle
// 0000fffe     temp_angle_1
// 00000000
// 00000000
// 00000000
// 00000000


// cs11 copies two specific ofdm frames and outputs like this
// 0000e000     temp_angle
// 04900923     one_pilot_observe
// 0000fffe     temp_angle_1
// 030afbd8     second_pilot_observe
// 00000000
// 00000000
// 00000000


module.exports = {
    Tagger
};



