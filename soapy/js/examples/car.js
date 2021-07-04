const FFT = require('fft');
const mt = require('mathjs');
const _ = require('lodash');
const np = require('./nplot.js');



// let cdata = [];

let combdata = [];

for(let i = 0; i < 1024; i++) {
  let j = mt.complex(0,1);
  let res = mt.exp(mt.multiply(j,i/2.0));
  // cdata.push(res);

  combdata.push(res.re);
  combdata.push(res.im);
//   data.push(Math.random()*100);
}

// console.log(combdata);



n = 1024;

var fft = new FFT.complex(n, true);

let output = [];
// let input2 = [1,0,2,0,3,0,4,0];
let type='complex';

// let input = Float64Array.from(input2);
 
/* Output and input should be float arrays (of the right length), type is either 'complex' (default) or 'real' */
// fft.process(output, outputOffset, outputStride, input, inputOffset, inputStride, type)
 
/* Or the simplified interface, which just sets the offsets to 0, and the strides to 1 */
// input and output to fft is
// [re,im,re,im]

console.log(combdata.slice(0,8));

// fft.simple(output, input, type);
fft.simple(output, combdata, type);

console.log(output.slice(0,8));

let cres = [];

// output = output.slice(0,8);

// console.log(output);

let out1 = _.chunk(output,2);

let out2 = out1.map((e,i)=>mt.complex(e[0],e[1]));

let mag = out2.map((e,i)=>mt.abs(e));

np.plot(mag, "car.png");