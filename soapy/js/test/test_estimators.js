var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const streamers = require('../mock/streamers.js');
const fs = require('fs');
const enumerate = require('pythonic').enumerate;
const estimators = require('../mock/estimators.js');

var assert = require('assert');

let np;
const savePlot = false;
if( savePlot ) {
   np = require('../mock/nplot.js');
}

let raw;
let frame0;
let frame0c;

describe("sfo", function () {

before(() => {
  // raw = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk.hex');
  // frame0 = raw.slice(256,1280);

  let off = 1280*2;
  raw = mutil.readHexFile('test/data/mapmov_cs10.hex');
  frame0 = raw.slice(off+256,off+1280);


  frame0c = mutil.IShortToComplexMulti(frame0);
});

it('estimate sfo plot', (done) => {
  // console.log(raw.slice(0,10));
  // console.log(frame0.length);
  console.log(frame0[1023].toString(16));

  let f0_re = frame0c.map((e,i)=>e.re);

  let f0_fft = mutil.fft(frame0c);

  let f0_fft_mag = f0_fft.map((e,i)=>mt.abs(e));

  let f0_fft_arg = f0_fft.map((e,i)=>e.arg());


  if( savePlot ) {
    let plotDelay = 0;
    setTimeout(() => {
      np.plot(f0_re, "f0_re.png");
    }, plotDelay);
    plotDelay += 1000;

    setTimeout(() => {
      np.plot(f0_fft_mag, "f0_fft_mag.png");
    }, plotDelay);
    plotDelay += 1000;

    setTimeout(() => {
      np.plot(f0_fft_arg, "f0_fft_arg.png");
    }, plotDelay);
    plotDelay += 1000;

  }


  done();
}); // it


it('estimate sfo fn', (done) => {

  // sum(from n = 0 to n = 30) (X_(4n+6)Conj(X_(4n+2)))

  // sum(from n =1 to n = 31)(X_(4n+4)Conj(X_(4n)))

  // let plan0 = [];

  // for(let n = 0; n <= 30; n++){
  //   let a = 4*n+6;
  //   let b = 4*n+2;
  //   plan0.push([a,b]);
  //   // console.log(`${a} vs ${b}`);
  // }

  // let plan1 = [];

  // for(let n = 1; n <= 31; n++){
  //   let a = 4*n+4;
  //   let b = 4*n;
  //   plan1.push([a,b]);
  //   // console.log(`${a} vs ${b}`);
  // }

  // console.log(plan0.slice(0,10));
  // console.log(plan1.slice(0,10));



  let res0 = estimators.calculateSfoUser0(frame0c);
  console.log('calculateSfo0 ' + res0);

  let res1 = estimators.calculateSfoUser1(frame0c);
  console.log('calculateSfo1 ' + res1);


  done();
}); // it

});