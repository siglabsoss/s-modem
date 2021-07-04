'use strict';
var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const CoarseSync = require('../mock/coarsesync.js');
const streamers = require('../mock/streamers.js');
const csv = require('csv-parser');
const fs = require('fs');
const enumerate = require('pythonic').enumerate;
const hc = require('../mock/hconst.js');

const FBParse = require("../mock/fbparse.js");
const FBTag = require("../mock/fbtag.js");

var assert = require('assert');

let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}



let raw;
let frame0;
let frame0c;
let rmover_input;
let rmover_expected;

describe("feedback bus tagger", function () {

before(() => {
  raw = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk.hex');


  // input
  rmover_input = mutil.readHexFile('test/data/reverse_mover_input.hex');

  rmover_expected = [];
  for(let i = 0; i < 32; i++) {
    let val = (((i*2598234492)^(i*20423)) + 0x42)&0xffffffff;
    val >>>= 0;
    rmover_expected.push(val);
    // console.log(val.toString(16) + " <-- ");
  }


  // frame0 = raw.slice(256,1280);

  // let off = 1280*2;
  // raw = mutil.readHexFile('test/data/mapmov_cs10.hex');
  // frame0 = raw.slice(off+256,off+1280);


  // frame0c = mutil.IShortToComplexMulti(frame0);
});

// testing what the rx chain will look like
    // assert(iFloat[0] == 0.00006408886989957563);
    // assert(iFloat[1] == 0.00007324442274239673);
    // assert(iFloat[21] == -0.000013546574318279256);
it('fb stream', function(done) {
  this.timeout(4000);
  let t = jsc.forall(jsc.constant(0), function () {

    let sArray;
    let coarse;
    let cap;

    let coarseResults = 0;

    let callback = function(val) {
      // console.log('got coarse callback with 0x' + val.toString(16));
      coarseResults++;
    }

    let chunksPushed = 0;

    let arrayChunkDone = function() {
      // console.log('a chunk was done');

      if(chunksPushed === 10) {
      //   console.log('triggering coarse')
        // coarse.triggerCoarse();
      }

      chunksPushed++;
    }

    // source -> turnstile -> coarse -> Bundle to 1280 and remove cp -> ishorttocf64 -> fft-> tag

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024}, arrayChunkDone);

    // load values into streamer, this should be done before pipe
    sArray.load(raw.slice(0,2*1280)); // goes to 66

    let turnstile = new streamers.ByteTurnstileStream({},{chunk:1280*4});

    // turnstile.advance((1280-256)*4);

    coarse = new CoarseSync.CoarseSync({},{ofdmNum:20,print2:false,print1:false,print3:false,print4:false}, callback);

    let removeCp = new streamers.RemoveCpStream({},{chunk:(1024+256)*4,cp:256*4});

    let toFloat = new streamers.IShortToCF64({}, {});

    let fft = new streamers.CF64StreamFFT({}, {n:1024,cp:0,inverse:true});

    let gain = new streamers.F64GainStream({}, {gain:1/10});

    let tagger = new FBTag.Tagger({},{});

    cap = new streamers.ByteCaptureStream({});

    let fcap = new streamers.F64CaptureStream({});

    // pipe, which will start the streaming
    // sArray.pipe(turnstile).pipe(coarse).pipe(removeCp).pipe(toFloat).pipe(fft).pipe(fcap);

    sArray.pipe(turnstile).pipe(coarse).pipe(removeCp).pipe(toFloat).pipe(fft).pipe(gain).pipe(tagger);


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // console.log(`cap got ${cap.capture.length/4/1024} frames`);

        // let f0_fft = fcap.toComplex().slice(0,1024);

        // let f0_fft_mag = f0_fft.map((e,i)=>mt.abs(e));

        // let f0_fft_arg = f0_fft.map((e,i)=>e.arg());

        let savePlot = false;


        if( savePlot ) {
          let plotDelay = 0;
          // setTimeout(() => {
          //   np.plot(f0_re, "rx_re.png");
          // }, plotDelay);
          // plotDelay += 1000;

          setTimeout(() => {
            np.plot(f0_fft_mag, "rx_fft_mag.png");
          }, plotDelay);
          plotDelay += 1000;

          setTimeout(() => {
            np.plot(f0_fft_arg, "rx_fft_arg.png");
          }, plotDelay);
          plotDelay += 1000;

        }

        // console.log(fcap.toComplex());
        // for( x of fcap.toComplex().slice(0,1024)) {
        //   console.log(x);
        // }


        resolve(true);
      }catch(e){reject(e)}}, 1000);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it.skip('ffffb stream', (done) => {

  done();
}); // it



it('Reverse Mover Slicer', function(done) {
  this.timeout(4000);
  let t = jsc.forall(jsc.constant(0), function () {


    let sArray;
    let coarse;
    let cap;
    let conv;
    let tagger;

    let totalMapMovOut = [];

    let onMapmovData = function(d) {
      // console.log('on mapmov' + d);
      totalMapMovOut = totalMapMovOut.concat(d);
    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024});

    // load values into streamer, this should be done before pipe
    sArray.load(rmover_input); // goes to 66

    conv = new streamers.IShortToCF64();

    tagger = new FBTag.Tagger({},{printSlicer:false, onMapmovData:onMapmovData});

    cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(conv).pipe(tagger).pipe(cap);


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        let finalOut = mutil.airpacketUnmangle(totalMapMovOut);

        let keepLast = 32;
        finalOut = finalOut.slice(finalOut.length-keepLast); // silce till the end

        // for(let x of finalOut) {
        //   console.log(x.toString(16) + "  final");
        // }
        assert.deepEqual(rmover_expected, finalOut);

        resolve(true);
      }catch(e){reject(e)}}, 300);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



it('Reverse Mover Slicer Tagged', function(done) {
  this.timeout(2000);
  let t = jsc.forall(jsc.constant(0), function () {


    let sArray;
    let coarse;
    let cap;
    let conv;
    let tagger;
    let fbparse;

    let totalMapMovOut = [];

    let onMapMovTaggedData = function(header,body) {
      // console.log(header);
      // console.log(body);
        totalMapMovOut = totalMapMovOut.concat(body);

    }



    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024});

    // load values into streamer, this should be done before pipe
    sArray.load(rmover_input); // goes to 66

    conv = new streamers.IShortToCF64();

    tagger = new FBTag.Tagger({},{printSlicer:false});



    // NOT A STREAM
    fbparse = new FBParse.Parser({printUnhandled:false});
    fbparse.reset();

    fbparse.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_DEMOD_DATA,
      onMapMovTaggedData );

    cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(conv).pipe(tagger).pipe(cap);


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{

        // console.log(cap.capture);

        // because this is non pipe mode, we sleep and then read the entire output at once
        let asBuf = Buffer.from(cap.capture);

        console.log(asBuf);
        // then feed into this non stream
        fbparse.gotPacket(asBuf);

        setTimeout(() => {

          // this varuable was built from callbacks from the non piped fbparse
          let finalOut = mutil.airpacketUnmangle(totalMapMovOut);

          let keepLast = 32;
          finalOut = finalOut.slice(finalOut.length-keepLast); // silce till the end

          // for(let x of finalOut) {
          //   console.log(x.toString(16) + "  final");
          // }
          assert.deepEqual(rmover_expected, finalOut);
          resolve(true);
        }, 300);

      }catch(e){reject(e)}}, 300);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



// finally got it working as a stream, now It can go back to back in more extensive tests
it('Reverse Mover Slicer Tagged Stream', function(done) {
  this.timeout(2000);
  let t = jsc.forall(jsc.constant(0), function () {


    let sArray;
    let coarse;
    let cap;
    let conv;
    let tagger;
    let fbparsestream;

    let totalMapMovOut = [];

    let onMapMovTaggedData = function(header,body) {
      // console.log(header);
      // console.log(body);
        totalMapMovOut = totalMapMovOut.concat(body);

    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024});

    // load values into streamer, this should be done before pipe
    sArray.load(rmover_input); // goes to 66

    conv = new streamers.IShortToCF64();

    tagger = new FBTag.Tagger({},{printSlicer:false});


    // IS A STREAM
    fbparsestream = new FBParse.ParserStream({},{printUnhandled:false});

    fbparsestream.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_DEMOD_DATA,
      onMapMovTaggedData );

    // pipe, which will start the streaming
    sArray.pipe(conv).pipe(tagger).pipe(fbparsestream);


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // this varuable was built from callbacks from the non piped fbparse
        let finalOut = mutil.airpacketUnmangle(totalMapMovOut);

        let keepLast = 32;
        finalOut = finalOut.slice(finalOut.length-keepLast); // silce till the end

        // for(let x of finalOut) {
        //   console.log(x.toString(16) + "  final");
        // }
        assert.deepEqual(rmover_expected, finalOut);
        resolve(true);
      }catch(e){reject(e)}}, 300);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it




});