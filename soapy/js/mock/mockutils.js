const mt = require('mathjs');
const fs = require('fs');
const FFT = require('fft');

// Returns a 64 bit number.
function bufToI32(buf) {
    // let addr = Integer.fromBits(buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0]);
    // return addr;


  // let bar = new ArrayBuffer(4);
  var conv = new DataView(buf);

  conv.setUint8(0, buf[3]);
  conv.setUint8(1, buf[2]);
  conv.setUint8(2, buf[1]);
  conv.setUint8(3, buf[0]);

  return conv.getUint32(0);
}

function arrayToIShort(buf) {
  let bar = new ArrayBuffer(4);
  var conv = new DataView(bar);

  conv.setUint8(0, buf[3]);
  conv.setUint8(1, buf[2]);
  conv.setUint8(2, buf[1]);
  conv.setUint8(3, buf[0]);

  return conv.getUint32(0);
}

function I32ToBuf(i32) {
    // const buf = Buffer.alloc(4);
    // buf[0] = 32;

  let buf = new ArrayBuffer(4);
  var conv = new DataView(buf);

  conv.setUint32(0, i32);

  let ret2 = new ArrayBuffer(4);
  ret2[0] = conv.getUint8(3);
  ret2[1] = conv.getUint8(2);
  ret2[2] = conv.getUint8(1);
  ret2[3] = conv.getUint8(0);

  // let ret = [
  // conv.getUint8(0),
  // conv.getUint8(1),
  // conv.getUint8(2),
  // conv.getUint8(3)
  // ];

  return ret2;
}

function I32ToBufMulti(data) {
  let b = new ArrayBuffer(data.length*4);
  let uint8_view = new Uint8Array(b);
  let writeto = 0;
  let parts;
  for(let i = 0; i < data.length; i++) {
    parts = I32ToBuf(data[i]);
    // console.log(parts);
    uint8_view[writeto+0] = parts[0];
    uint8_view[writeto+1] = parts[1];
    uint8_view[writeto+2] = parts[2];
    uint8_view[writeto+3] = parts[3];
    writeto+=4;
  }
  return [b,uint8_view];
}

// takes an array of 32 bit numbers packed as [re,im] and returns an array
// of complex objects
function IShortToComplexMulti(vector) {
  let ret = [];
  const gain = 1 / ((2**15)-1);

  let thebuffer = new ArrayBuffer(4); // in bytes
  let uint8_out_view = new Uint8Array(thebuffer);
  let uint32_out_view = new Uint32Array(thebuffer);
  let dataView = new DataView(uint8_out_view.buffer);


  for( x of vector ) {
    dataView.setUint32(0, x, true);
    const re = dataView.getInt16(0, true);
    const im = dataView.getInt16(2, true);

    const c = mt.complex(re*gain,im*gain);
    ret.push(c);
  }

  return ret;
}

function complexToIShortMulti(vector) {
  let ret = [];
  const gain = ((2**15)-1);

  let thebuffer = new ArrayBuffer(4); // in bytes
  let uint8_out_view = new Uint8Array(thebuffer);
  let uint32_out_view = new Uint32Array(thebuffer);
  let dataView = new DataView(uint8_out_view.buffer);


  for( x of vector ) {
    // Math.min(Math.max(this, min), max)
    dataView.setInt16(0, 
      Math.min(Math.max(Math.round(x.re*gain), -0x7fff), 0x7fff), 
      true);
    dataView.setInt16(2, 
      Math.min(Math.max(Math.round(x.im*gain), -0x7fff), 0x7fff), 
      true);
    
    let packed = dataView.getUint32(0, true);

    // const c = mt.complex(re*gain,im*gain);
    ret.push(packed);
  }

  return ret;
}

function complexToIFloatMulti(cplx) {

  let res = Array(0);

  for(let i = 0; i < cplx.length; i++) {
    res.push(cplx[i].re);
    res.push(cplx[i].im);
  }

  return res;
}

function IFloatToComplexMulti(iFloat) {
  let res = Array(0);

  for(let i = 0; i < iFloat.length; i+=2) {
    let re = iFloat[i]
    let im = iFloat[i+1];

    res.push(mt.complex(re,im));
  }

  return res;
}

// vector should be an array of floats, like:
// [r,i,r,i, ...]
function IFloatToIShortMulti(vector) {
  let ret = [];
  const gain = ((2**15)-1);

  let thebuffer = new ArrayBuffer(4); // in bytes
  let uint8_out_view = new Uint8Array(thebuffer);
  // let uint32_out_view = new Uint32Array(thebuffer);
  let dataView = new DataView(uint8_out_view.buffer);

  for(let i = 0; i < vector.length; i+=2) {

    // Math.min(Math.max(this, min), max)
    dataView.setInt16(0, 
      Math.min(Math.max(Math.round(vector[i]*gain), -0x7fff), 0x7fff), 
      true);
    dataView.setInt16(2, 
      Math.min(Math.max(Math.round(vector[i+1]*gain), -0x7fff), 0x7fff), 
      true);
    
    let packed = dataView.getUint32(0, true);

    // const c = mt.complex(re*gain,im*gain);
    ret.push(packed);
  }

  return ret;
}

// single version of above
function IShortToComplex(val) {
  return IShortToComplexMulti([val])[0];
}

// single version of above
function complexToIShort(val) {
  return complexToIShortMulti([val])[0];
}

function IShortToIFloatMulti(vector) {
  // wasteful to convert twice but I'm lazy to write more tests
  let cplx = IShortToComplexMulti(vector);

  let res = [];
  for( x of cplx) {
    res.push(x.re);
    res.push(x.im);
  }

  return res;
}



// converts something like an array, into an ArrayBuffer of size 4
// second argument is the offset
function sliceBuf4(buf, i0=0) {

  let bar = new ArrayBuffer(4);
  // let man = new Uint8Array(bar);

  bar[0] = buf[0+i0];
  bar[1] = buf[1+i0];
  bar[2] = buf[2+i0];
  bar[3] = buf[3+i0];

  // let bar2 = new Uint8Array(bar.slice(0,4)).buffer;

  return bar;
}

// file must not have trailing newline (fixme?)
function readHexFile(fname) {
  let out = [];
  fs.readFileSync(fname, 'utf-8').split(/\r?\n/).forEach(function(line){
    const a = parseInt(line, 16);
    out.push(a);
  });
  return out;
}

// write file without trailing newline
function writeHexFile(fname, values) {
  let str = '';
  const limit = values.length-1;
  for([i,x] of values.entries()) {
    const line = ((x&0xffffffff)>>>0).toString(16);
    const leftpad = '0'.repeat(8-line.length);

    str += leftpad;
    str += line;

    if(i < limit) {
      str += '\n';
    }
  }
  fs.writeFileSync(fname, str);
}

// When using lower triangular matrices, this function will return the correct index for the relationship between any to indices
// (aka mirrors an upper triangular index to lower triangular index)
// @param a An index
// @param b An index
// @returns an index that is always lower triangular
function lowerTriangular(a, b) {
  let idx;
  if(b > a) {
      idx = [b, a]
    }
  else {
      idx = [a, b]
    }
  return idx
}

function fftShift(a) {
  let split = Math.ceil(a.length/2);

  return a.slice(split).concat(a.slice(0,split));
}


// haystack and needle are both an array of numbers
// this will search the haystack and return the index of the first element when the sequence matches
function findSequence(haystack, needle) {
  function finder(element, index, array) {
    const len = needle.length;
    let target = needle[len-1];
    // look for last element match
    if( element == target ) {
      let icopy = index-1;
      let fault = false;
      // look backwards (why did I write it like this?)
      for( let i = len-2; i >= 0; i-- ) {
        if(array[icopy] === needle[i]) {

        } else {
          fault = true;
          break;
        }
        icopy--;
      }
      if( !fault ) {
        return true;
      }
    }
    return false;
  }


  let res = haystack.findIndex(finder);
  res -= needle.length-1;

  return res;
}

function printHexNl(x, title = undefined) {
  let asStr = x.map( (y) =>  y.toString(16) + '\n' ).join('');

  if(typeof title !== 'undefined') {
    console.log(title);
    console.log('--------');
  }

  console.log(asStr);
}

function printHex(x, title = undefined) {
  let asStr = x.map( (y) => y.toString(16) ).join(',');

  if(typeof title !== 'undefined') {
    console.log(title+asStr);
  } else {
    console.log(asStr);
  }
}

function printHexPad(x, title = undefined) {
  let asStr = x.map( (y) => {
    const ln = ((y&0xffffffff)>>>0).toString(16);
    const leftpad = '0'.repeat(8-ln.length);
    return leftpad + ln;
  } ).join(',');

  if(typeof title !== 'undefined') {
    console.log(title+asStr);
  } else {
    console.log(asStr);
  }
}

// accepts a complex number, and returns what riscv would return from ATAN()
// only returns ANGLE part
function riscvATANComplex(cplx, shift=15) {

  if( shift !== 15 ) {
    throw(new Error('shift of 15 was the only version tested'));
  }
  // calculated by observation
  const arg_factor = 10430.0;
  const mag_factor = 13490.45;

  const mag = mt.abs(cplx);
  let arg = cplx.arg();

  if( arg < 0 ) {
    arg += 2*Math.PI;
  }

  if( arg < 0 ) {
    console.log('ERROR: got negative angle after adding 2 pi, forcing to 0');
    arg = 0;
  }

  let magint = Math.round(mag * mag_factor);
  let argint = Math.round(arg * arg_factor);

  // bound magnitude
  if( magint > 0xffff ) {
    magint = 0xffff;
  }

  // bound arg, not sure if this will ever happen
  if( argint > 0xffff ) {
    argint = 0xffff;
  }

  let res = ((magint << 16) | argint) >>> 0;

  return res;
}

function riscvATAN(uint) {
  return riscvATANComplex(IShortToComplex(uint));
}

// pass to ishort (r,i) numbers
// This function compares ra to rb and ia to ib
// the tolerance is the number of counts allowed for either to be off
// if tol is 1 and both i and r are off by 1, this still passes
function IShortEqualTol(a,b,tol=1) {
  let deltaLower = ((a&0xffff)>>>0) - ((b&0xffff)>>>0);
  let deltaUpper = (((a>>16)&0xffff)>>>0) - (((b>>16)&0xffff)>>>0);

  let abSame = true;

  if( Math.abs(deltaLower) > tol || Math.abs(deltaUpper) > tol ) {
    abSame = false;
  }

  return abSame;
}

function fftComplexDirection(input_c,inverse) {

  n = input_c.length;

  let fft = new FFT.complex(n, inverse);

  let fftInput = Array(n*2);

  for(let i = 0; i < n*2; i+=2) {
    fftInput[i] = input_c[i/2].re;
    fftInput[i+1] = input_c[i/2].im;
  }

  let fftOutput = [];
  const type = 'complex';

  fft.simple(fftOutput, fftInput, type);

  let output_c = Array(n);

  for(let i = 0; i < n; i++) {
    output_c[i] = mt.complex(fftOutput[i*2], fftOutput[(i*2)+1]);
  }

  return output_c;
}

// inefficent to use because it builds the table every time
function fft(x) {
  return fftComplexDirection(x,false);
}

function ifft(x) {
  return fftComplexDirection(x,true);
}

// takes an array of javascript numbers
// returns a Uint8Array, which is mostly identical type to a Buffer
function IShortToStreamable(iShort) {

  let sz = iShort.length;

  let thebuffer = new ArrayBuffer(sz*4); // in bytes
  let uint8_view = new Uint8Array(thebuffer);
  let typedView = new Uint32Array(thebuffer);
  for(let i = 0; i < sz; i++) {
    typedView[i] = iShort[i];
  }

  return uint8_view;
}

// Takes a Buffer or Uint8Array and returns an array of Javascript numbers
function streamableToIShort(chunk) {
  const uint8_in_view = new Uint8Array(chunk, 0, chunk.length);
  let dataView = new DataView(uint8_in_view.buffer);

  // width of input in bytes
  const width = 4;

  // how many uint32's (how many samples) did we get
  const sz = chunk.length / width;

  let result = Array(sz);

  for(let i = 0; i < sz; i++) {
    let val = dataView.getUint32(i*4, true);
    result[i] = val;
  }

  return result;
}

function streamableToFloat(chunk) {
  const uint8_in_view = new Uint8Array(chunk, 0, chunk.length);
  let dataView = new DataView(uint8_in_view.buffer);

  // width of input in bytes
  const width = 8;

  // how many uint32's (how many samples) did we get
  const sz = chunk.length / width;

  let result = Array(sz);

  for(let i = 0; i < sz; i++) {
    let val = dataView.getFloat64(i*width, true);
    result[i] = val;
  }

  return result;
}

// from AirPacket TransformWords128::t
function transformWords128(inn) {
  const enab_words = 8;
  const div = Math.trunc(inn/enab_words);
  const sub = (2*enab_words)*div+(enab_words-1);
  const ret = sub - inn;
  return ret;
}

// copied from from AirPacket
// partial transform for bits on rx side
function bitMangleSliced(val) {
  let tword = (~val)>>>0;

  let uword = 0;
  for(let i = 0; i < 16; i++) {
    const bits = (tword>>>(i*2))&0x3;
    const bitrev = (((bits&0x1)<<1) | ((bits&0x2)>>1))>>>0;
    uword = (uword << 2) | bitrev;
  }

  return uword>>>0;
}


// does partial the work of:
// demodulateSliced
// bitMangleSliced
// unmangle's bytes
// this is only written for a single mode
function airpacketUnmangle(sliced_out) {
  let res = [];
  for(let i = 0; i < sliced_out.length; i++) {
    let j = transformWords128(i);  // converts the index of word we are looking at
    let val = bitMangleSliced(sliced_out[j]); // changes bits within individual word
    res.push(val);
    // console.log(bitMangleSliced(sliced_out[j]).toString(16));
  }
  return res;
}

function xorshift32(initial, data)
{
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  let hash = initial;
  for(let i = 0; i < data.length; i++) {
    hash += data[i];
    hash ^= hash << 13;
    hash ^= hash >>> 17;
    hash ^= hash << 5;
  }
  return hash;
}

function xorshift32_sequence(initial, len) {
  let state = initial;
  let out = [];

  for(let i = 0; i < len; i++) {
    state = xorshift32(state, [state])>>>0;
    out.push(state);
  }

  return out;
}




module.exports = {
    bufToI32
  , arrayToIShort
  , IShortToComplexMulti
  , complexToIShortMulti
  , complexToIShort
  , IShortToComplex
  , I32ToBuf
  , I32ToBufMulti
  , sliceBuf4
  , readHexFile
  , writeHexFile
  , lowerTriangular
  , fftShift
  , findSequence
  , printHexNl
  , printHex
  , printHexPad
  , riscvATAN
  , riscvATANComplex
  , IShortEqualTol
  , ifft
  , fft
  , IFloatToIShortMulti
  , complexToIFloatMulti
  , IFloatToComplexMulti
  , IShortToStreamable
  , streamableToIShort
  , streamableToFloat
  , streamableToIFloat : streamableToFloat
  , IShortToIFloatMulti
  , transformWords128
  , bitMangleSliced
  , airpacketUnmangle
  , xorshift32
  , xorshift32_sequence
};