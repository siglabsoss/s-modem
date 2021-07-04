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
const nativeWrap = require('../mock/wrapnativestream.js');
const rxChain1 = require('../build/Release/RxChain1.node');

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

describe("Native Rx Chain", function () {

before(() => {
  raw = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk.hex');
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
it('Native Rx Chain1', function(done) {
  this.timeout(4000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;
    let printcap = false;

    let sArray;
    let coarse;
    let rx1;
    let cap;

    let coarseResults = 0;

    let callback = function(val) {
      // console.log('got coarse callback with ' + val.toString(16));
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
    sArray.load(raw.slice(0,50*1280)); // goes to 66

    


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {

      let afterPipe = function(cls) {
        if(print) {
          console.log('afterPipe');
        }
        // cls.advance(initialAdvance);
      }


      let afterShutdown = function() {
        // console.log('gotShutdown');

        // let convBack = mutil.streamableToIShort(cap.capture);

        // // console.log(convBack.length);

        // assert(_.isEqual(convBack, raw_mod_chunk));

        resolve(true);
      }

      let addOutputCp = 0;
      let removeInputCp = 256;


      rx1 = new nativeWrap.WrapRxChain1({},{print:print,args:[removeInputCp,addOutputCp]}, rxChain1, afterPipe, afterShutdown);

      let tagger = new FBTag.Tagger({},{});

      cap = new streamers.ByteCaptureStream({});

      let fcap = new streamers.F64CaptureStream({},{print:printcap,disable:false});

      // pipe, which will start the streaming
      // sArray.pipe(turnstile).pipe(coarse).pipe(removeCp).pipe(toFloat).pipe(fft).pipe(fcap);

      sArray.pipe(rx1).pipe(tagger).pipe(fcap);


      setTimeout(() => {try{
        rx1.stop();
      }catch(e){reject(e)}},1000);

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


        // resolve(true);
      }catch(e){reject(e)}}, 2000);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



}); // describe