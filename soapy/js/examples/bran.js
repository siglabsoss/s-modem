var Fili = require('fili');
const mt = require('mathjs')
// var iirCalculator = new Fili.CalcCascades();

var firCalculator = new Fili.FirCoeffs();






var firFilterCoeffs = firCalculator.lowpass({
    order: 10, // filter order
    Fs: 1000, // sampling frequency
    Fc: 250 // cutoff frequency
    // forbandpass and bandstop F1 and F2 must be provided instead of Fc
  });


console.log(firFilterCoeffs);

// filter object
var filter = new Fili.FirFilter(firFilterCoeffs);


const np = require('./nplot.js');




let data = [];

// for(let i = 0; i < 1024; i++) {
//   data.push(Math.random()*100);
// }

let cdata = [];

for(let i = 0; i < 1024; i++) {
  let j = mt.complex(0,1);
  cdata.push(mt.exp(mt.multiply(j,i/2.0)));
//   data.push(Math.random()*100);
}

// console.log(cdata);


let rea = cdata.map(function(e, i) {
  return e.re;
});

let ima = cdata.map(function(e, i) {
  return e.im;
});

// console.log(rea);

// var cunzip = {re:rea, im:ima};

// console.log(cunzip);


// var fft3 = new Fili.Fft(1024);

// var fftResult3 = fft3.inverse(rea, ima); 

// console.log(fftResult3);

// // fftResult = {re: [], im: []}. The array length equals the FFT radix
// var magnitude3 = fft3.magnitude(fftResult3); // magnitude
// var dB3 = fft3.magToDb(magnitude); // magnitude in dB
// var phase3 = fft3.phase(fftResult); // phase

// np.plot(db3,"db3.png");


// process.exit();
data = [1,2,3,4];


np.plot(data,"a.png");


b = filter.multiStep(data);


setTimeout(function() {
  np.plot(b, "b.png");
}, 2000);




var fft = new Fili.Fft(4);

var fftResult = fft.forward(data, 'none'); 

console.log(fftResult);

// fftResult = {re: [], im: []}. The array length equals the FFT radix
var magnitude = fft.magnitude(fftResult); // magnitude
var dB = fft.magToDb(magnitude); // magnitude in dB
var phase = fft.phase(fftResult); // phase

setTimeout(function() {
  np.plot(dB, "db_orig.png");
}, 4000);

fftResult.re[2] *= 4;

var originalBuffer = fft.inverse(fftResult.re, fftResult.im);

console.log(originalBuffer);


var fftResult2 = fft.forward(b, 'none'); 
// fftResult2 = {re: [], im: []}. The array length equals the FFT radix
var magnitude = fft.magnitude(fftResult2); // magnitude
var dB2 = fft.magToDb(magnitude); // magnitude in dB
var phase = fft.phase(fftResult2); // phase

setTimeout(function() {
  np.plot(dB2, "db_filt.png");
}, 8000);

