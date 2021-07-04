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
const argmax = require( 'compute-argmax' );

let gp;
let savePlot = false;

if( savePlot ) {
   gp = require('../mock/gplot.js');
}



let raw;
let raw2;
let frame0;
let frame0c;
let rmover_input;
let rmover_expected;

describe("Rx Chain Debug", function () {

before(() => {
  raw = mutil.readHexFile('test/data/cooked_cs10_out.hex');
  raw2 = mutil.readHexFile('test/data/rx_frames.hex');
});


it('Debug Rx1', (done) => {
  let t = jsc.forall('bool', function () {

    let source = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024+256});

    source.load(raw);

    let removeCp = new streamers.RemoveCpStream({},{chunk:(1024+256)*4,cp:256*4});

    let conv = new streamers.IShortToCF64();

    let cp = 256;

    let fftStream = new streamers.CF64StreamFFT({}, {n:1024,cp:0,inverse:true});

    // build capture
    let cap = new streamers.F64CaptureStream({});

    // pipe, which will start the streaming
    source.pipe(removeCp).pipe(conv).pipe(fftStream).pipe(cap);
    // cannedSource.pipe(fftStream).pipe(cap);
    // ds1.pipe(ds2).pipe(fftStream);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // let cplx = mutil.fftShift(cap.toComplex());
        let cplx = cap.toComplex().slice(0,1024);
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        let maxBin = argmax(asmag0);

        // calculated through experiment
        // let expectedMax = [ 240, 1264 ];

        // assert( _.isEqual(maxBin, expectedMax));

        // console.log("captured " + cplx.length);

        if( savePlot ) {
          gp.plot(asmag0);
          gp.plot(asreal0);
            // setTimeout(() => {
            //   np.plot(asmag0, "fft_out0.png");
            // }, 0);

            // setTimeout(() => {
            //   np.plot(asreal0, "fft_real0.png");
            // }, 1500);
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},800);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it

it('Debug Rx2', (done) => {
  let t = jsc.forall('bool', function () {
    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{

        // console.log(raw2);


        let cplx = mutil.IShortToComplexMulti(raw2).slice(0,1024);

        // let cplx = mutil.fftShift(cap.toComplex());
        // let cplx = cap.toComplex().slice(0,1024);
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        let maxBin = argmax(asmag0);

        // calculated through experiment
        // let expectedMax = [ 240, 1264 ];

        // assert( _.isEqual(maxBin, expectedMax));

        // console.log("captured " + cplx.length);

        if( savePlot  ) {
          gp.plot(asmag0, {title:'mag'});
          gp.plot(asreal0, {title:'real'});
            // setTimeout(() => {
            //   np.plot(asmag0, "fft_out0.png");
            // }, 0);

            // setTimeout(() => {
            //   np.plot(asreal0, "fft_real0.png");
            // }, 1500);
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},800);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it

it('Debug Rx3', (done) => {
  let t = jsc.forall('bool', function () {
    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{

        // console.log(raw2);


        let cplx = mutil.IShortToComplexMulti(raw).slice(0,1024);

        // let cplx = mutil.fftShift(cap.toComplex());
        // let cplx = cap.toComplex().slice(0,1024);
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        let maxBin = argmax(asmag0);

        // calculated through experiment
        // let expectedMax = [ 240, 1264 ];

        // assert( _.isEqual(maxBin, expectedMax));

        // console.log("captured " + cplx.length);

        if( savePlot  ) {
          gp.plot(asmag0, {title:'Raw input mag'});
          gp.plot(asreal0, {title:'raw input real'});
            // setTimeout(() => {
            //   np.plot(asmag0, "fft_out0.png");
            // }, 0);

            // setTimeout(() => {
            //   np.plot(asreal0, "fft_real0.png");
            // }, 1500);
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},800);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it


it('Debug Tx1', (done) => {
  let t = jsc.forall('bool', function () {
    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {

        // console.log(raw2);

        let input = [];

        let this_phase = 0.0;
        let this_factor = 0.01;
        let this_chunk = 1024;

        let j = mt.complex(0,1);
        for(let i = 0; i < this_chunk; i++) {
          // console.log('' + this.factor + ' - ' + this.phase);
          let res = mt.exp(mt.multiply(j,this_phase));
          this_phase += this_factor;
          // cdata.push(res);


          input.push(res.re);
          input.push(res.im);
        }

        let inputEq = [];
        let inputEqCplx = [];

        for(let i = 0; i < 1024; i++) {
          let re = 0.5;
          let im = 0;

          // if( i > 512 ) {
          //   re /= i-511;
          // }

          inputEq.push(re); // real
          inputEq.push(im); // imag

          inputEqCplx.push(mt.complex(re,im));
        }

        let inputEqIShort = mutil.complexToIShortMulti(inputEqCplx);


        // console.log(input);

        let source = new streamers.StreamArrayType({}, {type:'Float64', chunk:1024*2});

        let eq = new streamers.IFloatEqStream({}, {chunk:1024, print:true});

        let cap = new streamers.F64CaptureStream({});

        // eq.defaultEq();
        // eq.loadEqIFloat(inputEq);
        eq.loadEqIShort(inputEqIShort);
        // eq.loadEqComplex(inputEqCplx);

        // load values into streamer, this should be done before pipe
        source.load(input);

        source.pipe(eq).pipe(cap);

      setTimeout(() => {try{

        // let cplx = mutil.IShortToComplexMulti(raw).slice(0,1024);

        // let cplx = mutil.fftShift(cap.toComplex());
        // let cplx = cap.toComplex().slice(0,1024);
        let cplx0 = mutil.IFloatToComplexMulti(input);
        let asmag0 = cplx0.map((e,i)=>mt.abs(e));
        let asreal0 = cplx0.map((e,i)=>e.re);
        let asarg0 = cplx0.map((e,i)=>e.arg());

        let cplx1 = cap.toComplex();
        let asmag1 = cplx1.map((e,i)=>mt.abs(e));
        let asreal1 = cplx1.map((e,i)=>e.re);
        let asarg1 = cplx1.map((e,i)=>e.arg());




        if( savePlot  ) {
          gp.plot(asmag0, {title:'Input Raw mag'});
          gp.plot(asreal0, {title:'Input raw real'});
          gp.plot(asarg0, {title:'Input Arg'});


          gp.plot(asmag1, {title:'Output Raw mag'});
          gp.plot(asreal1, {title:'Output raw real'});
          gp.plot(asarg1, {title:'Output Arg'});
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},800);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it



}); // describe