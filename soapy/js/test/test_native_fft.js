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

const fftChain = require('../build/Release/FFTChain.node');

var assert = require('assert');

const argmax = require( 'compute-argmax' );

// var gnuplot = require('gnu-plot');


let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}




describe("Native FFT Chain", function () {

  let raw_mod;
  let raw_mod_chunk;
  let fft_with_cp_hex;
  let fft_with_cp_ifloat;

  before(()=>{
    raw_mod = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk_mod.hex');
    raw_mod_chunk = raw_mod.slice(0,83*1024);

    fft_with_cp_hex = mutil.readHexFile('test/data/native_fft_256_cp.hex');
    fft_with_cp_ifloat = mutil.complexToIFloatMulti(mutil.IShortToComplexMulti(fft_with_cp_hex));
  });



it('Native FFT1', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;

    let sArray;
    let dut;
    let cap;

    let arrayChunkDone = function() {
    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Float64', chunk:1024*2}, arrayChunkDone);

    let floatin = [];

    for(let i = 0; i < 2048; i++) {
      let val = -1 + i/2048;
      if( i % 2 == 0) {
        val += 0.5;
      }
      floatin.push(val);
    }

    // console.log(floatin.slice(0,32));

    // load values into streamer, this should be done before pipe
    sArray.load(floatin);

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
        console.log('gotShutdown');

        // let convBack = mutil.streamableToIShort(cap.capture);

        // console.log(convBack.length);

        // assert(_.isEqual(convBack, raw_mod_chunk));

        resolve(true);
      }

      let addOutputCp = 0;
      let removeInputCp = 0;

      // step 1 make object
      dut = new nativeWrap.WrapNativeTransformStream({},{print:print,args:[removeInputCp,addOutputCp]}, fftChain, afterPipe, afterShutdown);


      cap = new streamers.ByteCaptureStream({},{print:print,disable:false});

      // pipe, which will start the streaming
      sArray.pipe(dut).pipe(cap);

      setTimeout(() => {try{
        dut.stop();
      }catch(e){reject(e)}},100);

    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('Native FFT2', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;
    let savePlot = false;

    let sArray;
    let dut;
    let cap;

    let arrayChunkDone = function() {
    }


    let theta = 0.1;
    let chunk = 1024;

    // let cp = 0;
    // let cp = 256;

    // build array stream object, passing arguments

    // let ds1 = new IShortGen1({});
    // let ds2 = new streamers.IShortToCF64({}, {});

    let source = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );


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
        console.log('gotShutdown');

        let cplx = cap.toComplex();
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        let maxBin = argmax(asmag0);

        // calculated through experiment
        let expectedMax = [ 240, 1264 ];

        // console.log(maxBin);

        // let asIShort = mutil.complexToIShortMulti(cplx);
        // mutil.writeHexFile('fft_with_cp.hex', asIShort);

        assert( _.isEqual(maxBin, expectedMax));

        // console.log("captured " + cplx.length);

        if( savePlot ) {
            setTimeout(() => {
              np.plot(asmag0, "native_fft_out0.png");
            }, 0);

            setTimeout(() => {
              np.plot(asreal0, "native_fft_real0.png");
            }, 2000);
        }

        resolve(true);
      }

      let addOutputCp = 256;
      let removeInputCp = 0;

      // step 1 make object
      dut = new nativeWrap.WrapNativeTransformStream({},{print:print,args:[removeInputCp,addOutputCp]}, fftChain, afterPipe, afterShutdown);


      cap = new streamers.F64CaptureStream({},{print:print,disable:false});

      // pipe, which will start the streaming
      source.pipe(dut).pipe(cap);

      setTimeout(() => {try{
        dut.stop();
      }catch(e){reject(e)}},100);

    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('Native FFT3', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;
    let savePlot = false;

    let sArray;
    let dut;
    let cap;

    let arrayChunkDone = function() {
    }

    // fft_with_cp_ifloat was generated
    let fft_with_cp_looped = [].concat(fft_with_cp_ifloat,fft_with_cp_ifloat,fft_with_cp_ifloat,fft_with_cp_ifloat);

    // let fft_with_cp_looped = [];

    // for(let j = 0; j < 4; j++) {
    //   for(let i = 0; i < 1280; i++) {
    //     fft_with_cp_looped.push(j);
    //     fft_with_cp_looped.push(i);
    //   }
    // }

    // console.log("fft_with_cp_looped input length " + fft_with_cp_looped.length);

    // for(x of fft_with_cp_ifloat) {
    //   console.log(x);
    // }

    let source = new streamers.StreamArrayType({},{type:'Float64', chunk:16});

    // load values into streamer, this should be done before pipe
    source.load(fft_with_cp_looped);


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
        console.log('gotShutdown');

        let cplx = cap.toComplex();
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        // let maxBin = argmax(asmag0);

        // for(let i = 0; i < cplx.length; i+=1024) {
        //   console.log(cplx.slice(i,i+8));
        // }

        assert(cplx.length == 4*1024);

        let oneChunk = cplx.slice(0,1024);

        // the input is the same fft frame 4 times with a cp
        // this test verifies that the we are correcly removing the cp buy looking at the first fft result
        // and then 
        for(let i = 1024; i < 1024*4; i+=1024) {
          let aChunk = cplx.slice(i,i+1024);
          assert.deepEqual(oneChunk, aChunk);
        }

        // console.log("captured " + cplx.length*2);

        if( savePlot ) {
            setTimeout(() => {
              np.plot(asmag0, "native_fft_out0.png");
            }, 0);

            setTimeout(() => {
              np.plot(asreal0, "native_fft_real0.png");
            }, 2000);
        }

        resolve(true);
      }

      let addOutputCp = 0;
      let removeInputCp = 256;

      // step 1 make object
      dut = new nativeWrap.WrapNativeTransformStream({},{print:print,args:[removeInputCp,addOutputCp]}, fftChain, afterPipe, afterShutdown);


      cap = new streamers.F64CaptureStream({},{print:print,disable:false});

      // pipe, which will start the streaming
      source.pipe(dut).pipe(cap);

      setTimeout(() => {try{
        dut.stop();
      }catch(e){reject(e)}},500);

    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it





}); // describe


