var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
// const mapmov = require('../mock/mapmov.js');
// const menv = require('../mock/mockenvironment.js');
const mt = require('mathjs');
const Stream = require("stream");
const streamers = require('../mock/streamers.js');
const range = require('pythonic').range;
const nativeWrap = require('../mock/wrapnativestream.js');

const coarseChain = require('../build/Release/CoarseChain.node');

// const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');

var assert = require('assert');

const argmax = require( 'compute-argmax' );

// var gnuplot = require('gnu-plot');




describe("Native Coarse Chain", function () {

  let raw_mod;
  let raw_mod_chunk;

  before(()=>{
    raw_mod = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk_mod.hex');
    raw_mod_chunk = raw_mod.slice(0,83*1024);
  });



it('Native Coarse passthrough', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;

    let sArray;
    let coarse;
    let cap;

    let arrayChunkDone = function() {
    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024}, arrayChunkDone);


    // load values into streamer, this should be done before pipe
    sArray.load(raw_mod_chunk);

    let afterPipe = function(cls) {
      if(print) {
        console.log('afterPipe');
      }
      // cls.advance(initialAdvance);
    }

    

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {


    let afterShutdown = function() {
        // console.log('gotShutdown');

        let convBack = mutil.streamableToIShort(cap.capture);

        // console.log(convBack.length);

        assert(_.isEqual(convBack, raw_mod_chunk));

        resolve(true);
      }

      // step 1 make object
      coarse = new nativeWrap.WrapCoarseChain({},{print:print}, coarseChain, afterPipe, afterShutdown);


      cap = new streamers.ByteCaptureStream({},{print:print,disable:false});

      // pipe, which will start the streaming
      sArray.pipe(coarse).pipe(cap);

      setTimeout(() => {try{
        coarse.stop();
      }catch(e){reject(e)}},100);

    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 10};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('Native Coarse Sync', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;

    let sArray;
    let coarse;
    let cap;


    let coarseResultsArray = []

    let coarseCallback = function(val) {
      // console.log('got coarse callback with 0x' + val.toString(16));
      coarseResultsArray.push(val);
    }

    let chunksPushed = 0;

    let arrayChunkDone = function() {
      // console.log('a chunk was done');

      if(chunksPushed === 0) {
      //   console.log('triggering coarse')
        coarse.triggerCoarseAt(6);
      }

      // seems like there is a timing bug in this test
      // could be fixed if triggerCoarseAt were a queue and not just one number
      if(chunksPushed === 26) {
      //   console.log('triggering coarse')
        coarse.triggerCoarseAt(31);
      }

      chunksPushed++;
    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024}, arrayChunkDone);


    // load values into streamer, this should be done before pipe
    sArray.load(raw_mod_chunk);

    let afterPipe = function(cls) {
      if(print) {
        console.log('afterPipe');
      }
      // cls.advance(initialAdvance);
    }

    

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {


    let afterShutdown = function() {
        // console.log('gotShutdown');

        let convBack = mutil.streamableToIShort(cap.capture);

        // console.log(convBack.length);

        assert.deepEqual([902,902], coarseResultsArray);

        // assert.equal(2, coarseResults, "coarse did not run twice or issue with callback")

        let vector = [];

        for( let i = 0; i < cap.capture.length; i+=4 ) {
          let ishort = mutil.arrayToIShort(cap.capture.slice(i,i+4));
          vector.push(ishort);
          // console.log(ishort);
        }

        let zeros = _.flatten(_.times(1024, _.constant([0])));

        let zeroRows = 0;

        for( let i = 0; i < vector.length; i+=1024) {
          // console.log(vector.slice(0,6));
          // mutil.printHexPad(vector.slice(i+0,i+6));

          let frame = vector.slice(i+0,i+1024);

          if(_.isEqual(frame, zeros)) {
            zeroRows++;
          }
        }

        assert.equal(zeroRows, 40, "Incorrect number of zero frames in output");

        resolve(true);
      }



      // step 1 make object
      coarse = new nativeWrap.WrapCoarseChain({},{print:print}, coarseChain, afterPipe, afterShutdown, coarseCallback);


      cap = new streamers.ByteCaptureStream({},{print:print,disable:false});

      // pipe, which will start the streaming
      sArray.pipe(coarse).pipe(cap);

      setTimeout(() => {try{
        coarse.stop();
      }catch(e){reject(e)}},100);

    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 10};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it




}); // describe


