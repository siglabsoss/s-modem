'use strict';

const mutil = require('../../js/mock/mockutils.js');

// const hc = require('../../js/mock/hconst.js');
// const util = require('util');



let channel = [2 * Math.random(), 2 * Math.random()];
let mmse = (vector, start) => {
  let eq = start;
  const lut = [[1, -1], [-1, -1,], [-1, 1], [1, 1]].map(cplx => cplx.map(val => val * Math.sqrt(2) / 2));
  let final = vector.map(x => {
    const x1 = cmul(x, eq);
    const quad = (x1[0] > 0) ? ((x1[1] > 0) ? 0 : 3) : ((x1[1] > 0) ? 1 : 2);
    const x2 = lut[quad];
    const x3 = cmul(x1, x2);
    const x4 = 1 / (x3[0] * x3[0] + x3[1] * x3[1] + 0.001);
    const x5 = [x4 * x3[0], x4 * -x3[1]];
    irr(eq, x5, 0.01);
    const x6 = cmul(x1, eq);
    // console.log(eq);
    // return [eq[0], eq[1]];
    return x6;
  });

  // console.log(eq);

  return [final,eq];
};

// let addNoise = (x, s) => {
//   const mag = 1 + awgn(s[0]);
//   const fi = s[1] * awgn(1);
//   const n = [mag * Math.cos(fi), mag * Math.sin(fi)];
//   return cmul(x, n);
// };

// let awgn = (scale) => (scale || 1) * (Math.random() + Math.random() + Math.random() + Math.random() - 4 / 2);
let cmul = (x, y) => [x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0]];
let noise = [ 0.1, 0.3];

// inplace modifies a
// b is the new sample (I think)
// g is the gain of the filter
let irr = (a, b, g) => {
  a[0] = g * (b[0] - a[0]) + a[0];
  a[1] = g * (b[1] - a[1]) + a[1];
};

// let r1 = () => {
//   const lut = [[1,1], [-1,1], [-1,-1], [1,-1]].map(cplx => cplx.map(val => val * Math.sqrt(2) / 2));
//   const a = Array(500).fill().map(() => lut[(4 * Math.random()) |0]);
//   const bb = rrot;
//   const b = a
//     .map(cplx => addNoise(cplx, noise))
//     .map(cplx => cmul(cplx, channel));
//   const c = mmse(bb, [1, 0]);
//   return [{data: bb, name: 'received'}, {data: c, name: 'equalized'}, {data: a, name: 'target'}];
// };

// let r3 = (din) => {
//   const bb = din;
//   const [c,newq] = mmse(bb, [1, 0]);
//   return [c,newq];
// };


let kalmanReducer = (prev, cur) => {
  const a = prev[prev.length - 1];
  const g = 0.05;
  prev.push(a + g * (cur - a));
  return prev;
}

let evm = vec => vec.map(cplx =>
  10 * Math.log10(
    cplx.reduce((prev, val) =>
      prev + Math.pow(Math.abs(val) - 0.707, 2), 0
    )
  )
);

// let r2 = r1();
// let r4 = r3(rrot);

// let initial = [1,0];
// initial = [ 1.8679233934514763, -0.40771497870712947 ];


// // let r5 = mmse(rrot, r4[1]);

// // console.log(r2[1].data);

// const evm_raw0 = evm(r4[0]);
// // const evm_raw1 = evm(r5[0]);

// let evm_smooth0 = evm_raw0.reduce(kalmanReducer, [-23]);
// // let evm_smooth1 = evm_raw1.reduce(kalmanReducer, [0]);

// for( let x of evm_smooth0 ) {
//   console.log(x);
// }



class CalcEvm {
  constructor() {

    this.eqtaps = [1,0];
    this.kalmanValue = 0;

  }

  ishortEvm(x) {

    // console.log('got ' + x.length);


    let y = mutil.IShortToComplexMulti(x);

    // console.log(y[0]);
    // console.log(y[1]);
    // console.log(y[2]);

    let z = [];
    for(let w of y) {
      z.push([w.re,w.im]);
    }

    return this.cplxEvm(z);

  }

  cplxEvm(x) {
    // console.log('got ' + x.length);

    let r4 = mmse(x, this.eqtaps);

    this.eqtaps = r4[1]; // update eq

    const evm_raw0 = evm(r4[0]);

    let evm_smooth0 = evm_raw0.reduce(kalmanReducer, [this.kalmanValue]);

    this.kalmanValue = evm_smooth0[evm_smooth0.length-1]; // update kalman smoothing

    return evm_smooth0;

  }

}

module.exports = {
    CalcEvm
}
