'use strict';
const Stream = require("stream");
const _ = require('lodash');
const mt = require('mathjs');
const FFT = require('fft');
const mutil = require("../mock/mockutils.js");
const RateKeep = require('../mock/ratekeep.js');
const pipeType = require('../mock/pipetypecheck.js');

// Writable stream which captures and saves float64 values send to it
// Float values do not need to be "complex floats", ie floats can come in odd numbers
// however there is the toComplex() function which will cast the saved data and return
// an array of complex numbers
// is this similar to ?  https://www.npmjs.com/package/stream-to-array
class F64CaptureStream extends Stream.Writable {
  
  constructor(options, options2={disable:false}){
    super(options) //Passing options to native constructor (REQUIRED)
    this.capture = [];

    Object.assign(this, options2);
  }
  
  /*
    Every stream class should have a _write method which is called intrinsically by the api,
    when <streamObject>.write method is called, _write method is not to be called directly by the object.
  */

  // consider using https://www.npmjs.com/package/buffer-dataview which may be faster?
  
  _write(chunk,encoding,cb) {

    if(this.print) {
      console.log("len: " + chunk.length + ' [' + JSON.parse(JSON.stringify(chunk)).data + ']' );
    }

    // console.log()
    // console.log('t chunk ' + kindOf(chunk));
    // console.log('t2 ' + kindOf(uint8));

    // Uint32Array
    // console.log('got '+ chunk.length);


    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    // console.log(uint8_view);
    // console.log(uint32_view);

    if( !this.disable ) {
      for(let i = 0; i < chunk.length/8; i++) {
        this.capture.push(dataView.getFloat64(i*8, true));
      }
    }

    cb();
  }

  toComplex() {
    return _.chunk(this.capture,2).map((e,i)=>mt.complex(e[0],e[1]));
  }

  erase() {
    this.capture = [];
  }
}

class ByteCaptureStream extends Stream.Writable {
  
  constructor(options, options2={disable:false, print:false}){
    super(options) //Passing options to native constructor (REQUIRED)
    this.capture = [];

    Object.assign(this, options2);
  }
  
  /*
    Every stream class should have a _write method which is called intrinsically by the api,
    when <streamObject>.write method is called, _write method is not to be called directly by the object.
  */

  // consider using https://www.npmjs.com/package/buffer-dataview which may be faster?
  
  _write(chunk,encoding,cb) {

    if(this.print) {
      console.log("len: " + chunk.length + ' [' + JSON.parse(JSON.stringify(chunk)).data + ']' );
    }

    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    // console.log(uint8_view);
    // console.log(uint32_view);

    if( !this.disable ) {
      for(let i = 0; i < chunk.length; i++) {
        this.capture.push(uint8_view[i]);
      }
    }

    cb();
  }

  erase() {
    this.capture = [];
  }
}

// Allows for removal of bytes from a stream by calling advance()
// all output leaving this stream will always be of options2.chunk size
class ByteTurnstileStream extends Stream.Transform {
  
  constructor(options, options2={}){
    super(options);
    this.capture = Buffer.alloc(0);

    Object.assign(this, options2);

    this.pending_advance = 0;

    this.name = 'ByteTurnstileStream';
    this.pipeType = {in:{type:'any',chunk:'any'},out:{type:'forward',chunk:'forward'}};
    this.on('pipe', this.pipeTypeCheck.bind(this));
  }

  // when pipe us used like this:
  // them.pipe(us)
  pipeTypeCheck(them) {
    // console.log(this.name + " got pipe from");
    // console.log(them);
  }
  
  advance(count) {
    this.pending_advance += count;
  }

  _transform(chunk, encoding, cb) {

    let longData = Buffer.concat([this.capture, chunk]);
    let len = longData.length;

    if( this.pending_advance > 0) {
      const dump = Math.min(this.pending_advance, len);
      longData = longData.slice(dump);
      len = longData.length;
      this.pending_advance -= dump;
      // console.log('trimming ' + dump);
    }

    const rem = len % this.chunk;
    const cutoff = len - rem;

    for (let i = 0; i < cutoff; i += this.chunk) {
        const piece = longData.slice(i, i+this.chunk);
        this.push(piece);
    }

    this.capture = longData.slice(cutoff, len);

    cb();
  }

  erase() {
    this.capture = Buffer.alloc(0);
  }
}

// acumulates input until we have options2.chunk bytes
// removes options2.cp bytes before passing to next stream
class RemoveCpStream extends Stream.Transform {
  
  constructor(options, options2={}){
    super(options);
    this.capture = Buffer.alloc(0);

    Object.assign(this, options2);

  }
  
  _transform(chunk, encoding, cb) {

    const longData = Buffer.concat([this.capture, chunk]);
    const len = longData.length;

    const rem = len % this.chunk;
    const cutoff = len - rem;

    for (let i = 0; i < cutoff; i += this.chunk) {
        const piece = longData.slice(i+this.cp, i+this.chunk);
        this.push(piece);
    }

    this.capture = longData.slice(cutoff, len);

    cb();
  }

  erase() {
    this.capture = Buffer.alloc(0);
  }
}


// acumulates input until we have options2.chunk bytes
// removes options2.cp bytes before passing to next stream
class AddCounterStream extends Stream.Transform {
  
  constructor(options, options2={}){
    super(options);
    this.capture = Buffer.alloc(0);

    Object.assign(this, options2);

    this.counter = 0;

  }
  
  _transform(chunk, encoding, cb) {

    const longData = Buffer.concat([this.capture, chunk]);
    const len = longData.length;

    const rem = len % this.chunk;
    const cutoff = len - rem;

    for (let i = 0; i < cutoff; i += this.chunk) {
        const piece = longData.slice(i, i+this.chunk);
        const counterBytes = mutil.IShortToStreamable([this.counter]);
        // console.log(counterBytes);
        // console.log(this.counter);
        this.push(Buffer.concat([counterBytes,piece]));
        this.counter = ((this.counter+1)&0xffffffff)>>>0;
    }

    this.capture = longData.slice(cutoff, len);

    cb();
  }

  erase() {
    this.capture = Buffer.alloc(0);
  }
}




// Consumes upstream and sends nothing downstream
class DummyStream extends Stream.Transform {
  
  constructor(options, options2={}){
    super(options) //Passing options to native constructor (REQUIRED)

    // Object.assign(this, options2);
  }

  _transform(chunk,encoding,cb){
    cb(null, null);
  }
}



class F64GainStream extends Stream.Transform {
  constructor(options, opt){
    super(options) //Passing options to native constructor (REQUIRED)
    Object.assign(this, opt);
  }
  
  _transform(chunk,encoding,cb){
    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    // in place modify of the copy
    for(let i = 0; i < chunk.length/8; i++) {
      let tmp = dataView.getFloat64(i*8, true) * this.gain;
      dataView.setFloat64(i*8, tmp, true);
      // this.capture.push();
    }
    this.push(uint8_view);

    cb();
  }
}

class IFloatEqStream extends Stream.Transform {
  constructor(options, options2){
    super(options) //Passing options to native constructor (REQUIRED)

    let {print = false, chunk=1024} = options;

    Object.assign(this, {print,chunk});

    this.width = 8*2;
    this.sz = chunk;  // redundant ?
    this.byte_sz = this.sz*this.width;

    this.IFloatEq = [];
  }

  defaultEq() {
    let c = mt.complex(1,0);
    for(let i = 0; i < this.chunk; i++) {
      this.IFloatEq.push(c);
    }
  }

  loadEqIFloat(l) {
    if( l.length !== this.chunk*2 ) {
      throw(new Error(`loadEqIFloat must have ${this.chunk*2} floats`));
    }

    this.IFloatEq = [];

    for(let i = 0; i < l.length; i+=2) {
      let c = mt.complex(l[i],l[i+1]);
      this.IFloatEq.push(c);
    }

  }

  loadEqIShort(l) {
    return this.loadEqComplex(mutil.IShortToComplexMulti(l));
  }

  loadEqComplex(l) {
    if( l.length !== this.chunk ) {
      throw(new Error(`loadEqComplex must have ${this.chunk} complex objects`));
    }

    this.IFloatEq=l.slice(0); // copy
  }
  
  _transform(chunk,encoding,cb){
    if(chunk.length !== this.byte_sz) {
      throw(new Error(`IFloatEqStream stream must have ${this.sz} float pairs`));
    }

    if(this.IFloatEq.length !== this.sz) {
      throw(new Error(`IFloatEqStream must load an Eq before first data`));
    }

    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    // in place modify of the copy
    let k = 0;
    for(let i = 0; i < chunk.length; i+=this.width) {
      let ii = i+(this.width/2);
      let real = dataView.getFloat64(i,  true);
      let imag = dataView.getFloat64(ii, true);
      // dataView.setFloat64(i*8, tmp, true);

      let sam = mt.complex(real, imag);
      let res = sam.mul(this.IFloatEq[k]);

      dataView.setFloat64(i,  res.re, true); // write back inplace
      dataView.setFloat64(ii, res.im, true);

      // console.log("re: " + real + ", im: " + imag);
      // console.log("re: " + res.re + ", im: " + res.im);
      // this.capture.push();

      k++;
    }
    this.push(uint8_view);

    cb();
  }
}


class StreamToUdp extends Stream.Writable {
  constructor(options, options2){
    super(options);
    Object.assign(this, options2);
  }
  
  _write(message,encoding,cb) {
    // message = new Buffer(message, "utf8");
    // console.log(message.slice(0,12));
    this.sock.send(message, 0, message.length, this.port, this.addr);
    cb();
  }
}



class CF64StreamSin extends Stream.Readable{
  
    //Every class should have a constructor, which takes options as arguments. (REQUIRED)
  
    constructor(options, options2){
        super(options); //Passing options to native class constructor. (REQUIRED)
        this.count = options2.count; //A self declared variable to limit the data which is to be read.
        this.factor = options2.theta;
        this.chunk = options2.chunk;

        this.debug_count = 0;
        this.phase = 0;


        // dv.setFloat64(0, number, false);

        // 8 is for bytes per float, 2 is for real/imag
        this.thebuffer = new ArrayBuffer(this.chunk*8*2); // in bytes
        this.uint8_view = new Uint8Array(this.thebuffer);
        this.uint32_view = new Uint32Array(this.thebuffer);
        this.float64_view = new Float64Array(this.thebuffer);
        // this.dv = new DataView(this.thebuffer);

        for(let i = 0; i < this.chunk; i++) {
          this.uint32_view[i] = i;
        }


    }
    
    /*
    Every stream class should have a _read method which is called intrinsically by the api,
    when <streamObject>.read method is called, _read method is not to be called directly by the object.
    */
    _read(sz){
        // console.log('sz ' + sz);
          // console.log('pushing');
            
          let ret = true;
          while(ret && this.count > 0) {


            let j = mt.complex(0,1);
            for(let i = 0; i < this.chunk; i++) {
              // console.log('' + this.factor + ' - ' + this.phase);
              let res = mt.exp(mt.multiply(j,this.phase));
              this.phase += this.factor;
              // cdata.push(res);


              this.float64_view[2*i] = res.re;
              this.float64_view[(2*i)+1] = res.im;

              // this.float64_view[2*i] = this.debug_count++;
              // this.float64_view[(2*i)+1] = 0;

              // combdata.push(res.re);
              // combdata.push(res.im);
            //   data.push(Math.random()*100);
            }


            // seems like we need to copy the buffer here
            // because the code above uses the same buffer over and over for this class
            // if we allocated a new buffer above we could remove the slice here
            ret = this.push(this.uint8_view.slice(0));
            this.count--;


          }
    }
}


// Converts the IShort format we use internally inside Higgs
//   0xIIIIRRRR
// to IFloat (complex float 64) which is
//   real, imag  (which is two floats, not a Complex() object)
// the reason for order reversal here is that the FFT library nativly uses the (real, imag)
// format so we use that to prevent waiting cpu with additional "swizzling"
class IShortToIFloat extends Stream.Transform {
  constructor(options){
    super(options) //Passing options to native constructor (REQUIRED)

    this.gain = 1 / ((2**15)-1);

    this.name = 'IShortToIFloat';
    this.pipeType = {in:{type:'IShort',chunk:'any'},out:{type:'IFloat',chunk:'forward'}};
    this.on('pipe', this.pipeTypeCheck.bind(this));

  }

  pipeTypeCheck(them) {
    // console.log(this.name + " got pipe from");
    // console.log(them);
  }
  
  _transform(chunk,encoding,cb){
    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_in_view = new Uint8Array(chunk, 0, chunk.length);
    let dataView = new DataView(uint8_in_view.buffer);

    // width of input in bytes
    const width = 4;

    // how many uint32's (how many samples) did we get
    const sz = chunk.length / width;

    const outwidth = 2*8; // how many bytes per output sample

    // output buffer
    let thebuffer = new ArrayBuffer(sz*outwidth); // in bytes
    let uint8_out_view = new Uint8Array(thebuffer);
    let float64_out_view = new Float64Array(thebuffer);

    for(let i = 0; i < sz*2; i+=2) {
      let re = dataView.getInt16(i*2, true);
      let im = dataView.getInt16((i*2)+2, true);

      float64_out_view[i] = re * this.gain;
      float64_out_view[i+1] = im * this.gain;
    }

    // push uint8 view of floats downstream
    this.push(uint8_out_view);

    cb();
  }
}

class IFloatToIShort extends Stream.Transform {
  constructor(options, options2={}){
    super(options) //Passing options to native constructor (REQUIRED)

    let {printType = false} = options2;

    this.printType = printType;

    this.gain = ((2**15)-1);

    this.name = 'IFloatToIShort';
    this.pipeType = {in:{type:'IFloat',chunk:'any'},out:{type:'IShort',chunk:'forward'}};
    this.on('pipe', this.pipeTypeCheck.bind(this));

  }

  pipeTypeCheck(them) {
    // console.log(this.name + " got pipe from");
    // console.log(them);
    pipeType.warn(this,them, this.printType);
  }
  
  _transform(chunk,encoding,cb){
    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_in_view = new Uint8Array(chunk, 0, chunk.length);
    let dataView = new DataView(uint8_in_view.buffer);

    // width of input in bytes
    const width = 2*8;

    // how many IFloat (pairs of floats) (how many samples) did we get
    const sz = chunk.length / width;

    const outwidth = 4; // how many bytes per output sample

    // output buffer
    let thebuffer = new ArrayBuffer(sz*outwidth); // in bytes
    let uint8_out_view = new Uint8Array(thebuffer);
    // let float64_out_view = new Float64Array(thebuffer);
    let int16_out_view = new Uint16Array(thebuffer);

    for(let i = 0; i < sz*2; i+=2) {
      // let re = dataView.getInt16(i*2, true);
      // let im = dataView.getInt16((i*2)+2, true);

      let re = dataView.getFloat64(i*8, true);
      let im = dataView.getFloat64((i*8)+8, true);

      int16_out_view[i] = re * this.gain;
      int16_out_view[i+1] = im * this.gain;

      // float64_out_view[i] = re * this.gain;
      // float64_out_view[i+1] = im * this.gain;
    }

    // push uint8 view of floats downstream
    this.push(uint8_out_view);

    cb();
  }
}


// same as IFloatToIShort but work is metered by an instance of WorkKeeper
class IFloatToIShortMetered extends Stream.Transform {
  constructor(options, options2={}){
    super(options) //Passing options to native constructor (REQUIRED)

    const {printType = false, keeper=null, keeperId=null} = options2;

    this.printType = printType;
    this.keeper = keeper;
    this.keeperId = keeperId;

    this.gain = ((2**15)-1);

    this.name = 'IFloatToIShort';
    this.pipeType = {in:{type:'IFloat',chunk:'any'},out:{type:'IShort',chunk:'forward'}};
    this.on('pipe', this.pipeTypeCheck.bind(this));

  }

  pipeTypeCheck(them) {
    // console.log(this.name + " got pipe from");
    // console.log(them);
    pipeType.warn(this,them, this.printType);
  }

  _transform(chunk, encoding, cb) {
    this.keeper.do(this.keeperId, ()=>{this._transform2(chunk, encoding, cb)});
  }
  
  _transform2(chunk,encoding,cb) {
    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_in_view = new Uint8Array(chunk, 0, chunk.length);
    let dataView = new DataView(uint8_in_view.buffer);

    // width of input in bytes
    const width = 2*8;

    // how many IFloat (pairs of floats) (how many samples) did we get
    const sz = chunk.length / width;

    const outwidth = 4; // how many bytes per output sample

    // output buffer
    let thebuffer = new ArrayBuffer(sz*outwidth); // in bytes
    let uint8_out_view = new Uint8Array(thebuffer);
    // let float64_out_view = new Float64Array(thebuffer);
    let int16_out_view = new Uint16Array(thebuffer);

    for(let i = 0; i < sz*2; i+=2) {
      // let re = dataView.getInt16(i*2, true);
      // let im = dataView.getInt16((i*2)+2, true);

      let re = dataView.getFloat64(i*8, true);
      let im = dataView.getFloat64((i*8)+8, true);

      int16_out_view[i] = re * this.gain;
      int16_out_view[i+1] = im * this.gain;

      // float64_out_view[i] = re * this.gain;
      // float64_out_view[i+1] = im * this.gain;
    }

    // push uint8 view of floats downstream
    this.push(uint8_out_view);

    cb();
  }
}


// pass size and cp length
class IFloatStreamFFT extends Stream.Transform {
  constructor(options, options2){
    super(options); //Passing options to native constructor (REQUIRED)

    // this.gain = 1 / ((2**15)-1);

    Object.assign(this, options2);

    this.fft = new FFT.complex(this.n, this.inverse);

  }
  
  _transform(chunk,encoding,cb){
    const ffttype = 'complex';

    // width of input in bytes
    const width = 8*2;

    // how many uint32's (how many samples) did we get
    const sz = chunk.length / width;

    if( sz != this.n ) {
      throw(new Error('Illegal chunk size for IFloatStreamFFT: ' + sz + ' expecting: ' + this.n));
    }


    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    var dataView = new DataView(uint8_view.buffer);

    // it's possible that a proxy may be faster here instead of building bufferInput/fftInput
    // https://ponyfoo.com/articles/es6-proxies-in-depth
    
    let bufferInput = new ArrayBuffer(sz*width); // in bytes
    let bufferOutput = new ArrayBuffer(sz*width); // in bytes


    let fftInput = new Float64Array(bufferInput);
    let fftOutput = new Float64Array(bufferOutput);
    let uint8_out_view = new Uint8Array(bufferOutput);

    // in place modify of the copy
    for(let i = 0; i < sz*2; i+=2) {
      let re = dataView.getFloat64(i*8, true);// * this.gain;
      let im = dataView.getFloat64((i*8)+8, true);// * this.gain;
      // console.log('re: ' + re + ' im: ' + im);
      fftInput[i] = re;
      fftInput[i+1] = im;
    }

    this.fft.simple(fftOutput, fftInput, ffttype);

    // push cp
    if(this.cp !== 0) {
      // could be constant
      const n_byte = this.n * width;
      const cp_byte = this.cp * width;
      this.push(uint8_out_view.slice(n_byte-cp_byte));
    }

    // push output
    this.push(uint8_out_view);

    cb();
  }
}





// call the constructor with a type
// call the load() method with an array of elements of that type
// and this class will stream out bytes representing the elements
// the units of chunk are the of the type, not bytes
class StreamArrayType extends Stream.Readable {
  
  constructor(options, options2, callback){
    super(options) //Passing options to native constructor (REQUIRED)
    this.willChop = [];
    // this.willChopOriginal = [];

    this.precompute = [];
    this.readBackIndex = 0;
    
    const {type = 'uint8', chunk = 1, repeat = false} = options2;

    this.repeat = !!repeat;

    if(typeof type !== 'string') {
      throw(new Error('Second argument must have a type member'));
    }

    this.typeStr = type;

    this._chunk_done_callback = callback;

    switch(this.typeStr.toLowerCase()) {
      case 'float64':
        this.switchInt = 0;
        this.typeBytes = 8;
        break;
      case 'uint32':
        this.switchInt = 1;
        this.typeBytes = 4;
        break;
      case 'uint8':
        this.switchInt = 2;
        this.typeBytes = 1;
        break;
      default:
        throw(new Error('Unsupported type ' + this.type));
        break;
    }

    if( typeof chunk !== 'number' ) {
      throw(new Error('Second argument must have a chunk member typed number'));
    }

    // chunk is defined to count types, not to count the underlying bytes
    this.chunk = _.toInteger(chunk);

    if( this.chunk < 1 ) {
      throw(new Error('Chunk must be greater than 0'));
    }

    // console.log("StreamArrayType " + this.typeStr + " ctons chunk: " + this.chunk);

  }

  // adds data to the rhs of the array to be streamed out
  load(l) {
    if( typeof l === 'undefined' ) {
      throw(new Error('Illegal to pass undefined to load'));
    }
    if( this.willChop.length !== 0 ){
      throw(new Error('Illegal to load more than once'));
    }
    // FIXME: rename to willDestroy
    this.willChop = l;
    this.chopIntoPrecompute();

    // console.log(this.precompute);

    // this.willChopOriginal = this.willChopOriginal.concat(l); // lazy
  }
  
  chopIntoPrecompute() {

    let ret = true;
    while(ret) {
      // how many to read this go
      let thisRead = Math.min(this.willChop.length, this.chunk);

      // console.log("chopIntoPrecompute with " + this.willChop.length + " " + thisRead);

      // break if empty
      if(thisRead === 0) {
        ret = false;
        break;
      }

      let thebuffer = new ArrayBuffer(thisRead*this.typeBytes); // in bytes
      let uint8_view = new Uint8Array(thebuffer);

      let typedView;
      switch(this.switchInt) {
        case 0:
          typedView = new Float64Array(thebuffer);
          break;
        case 1:
          typedView = new Uint32Array(thebuffer);
          break;
        case 2:
          typedView = uint8_view; // tricky, just use the same view
          break;
        default:
          throw(new Error('somehow in _read() with illegal setup'));
          break;
      }

      // load types in as bytes
      for(let i = 0; i < thisRead; i++) {
        typedView[i] = this.willChop[i];
      }

      // shorten our data
      this.willChop = this.willChop.slice(thisRead);

      // console.log(this.readableFlowing);

      // console.log('pushing ' + thisRead);

      this.precompute.push(uint8_view);

    } // while ret
  }

  _read(sz) {

    let ret = true;
    while(ret && (this.readBackIndex < this.precompute.length) ) {

      if(!this.repeat) {
        ret = this.push(this.precompute[this.readBackIndex]);
      } else {
        ret = false;
        let freezeIndex = this.readBackIndex; // becuase of timeout this value will change without this copy
        setTimeout(()=>{this.push(this.precompute[freezeIndex]);}, 0);
      }

      this.readBackIndex++;

      if(this.repeat) {
        let a = this.readBackIndex;
        this.readBackIndex %= this.precompute.length;
        let b = this.readBackIndex;
        // console.log("a, b: " + a + " " + b);
      }

      if( typeof this._chunk_done_callback !== 'undefined' ) {
        this._chunk_done_callback();
      }

    } // while ret
  } // _read()

  erase() {
    this.capture = [];
  }
} // StreamArrayType


// streams out 32bit IShort (0xIIIIRRRR)
class RateStreamHexFile extends Stream.Readable{
  
  constructor(options, options2={rate:0,chunk:1024}){
    super(options); //Passing options to native class constructor. (REQUIRED)
    // this.count = 1; // number of chunks to readout
    Object.assign(this, options2);

    this.width = 4;

    this.progress = 0;

    this.capture = mutil.IShortToStreamable(mutil.readHexFile(this.fname));

    this.limiter = new RateKeep.RateKeep({
        rate:this.rate, // chunks per second
        mode:RateKeep.Mode.Queue // if running too fast, call it later for us
      });

    this.thebuffer = new ArrayBuffer(4*this.chunk); // in bytes
    this.uint8_view = new Uint8Array(this.thebuffer);
    this.uint32_view = new Uint32Array(this.thebuffer);
    // this.uint16_view = new Uint16Array(this.thebuffer);

  }
    
  _read(sz){

    let ret = true;

    let work = () => {
      let adv = this.chunk*this.width;

      if( (this.progress+adv) > this.capture.length ) {
        // out of samples
        // if( !this.repeat ) {}
      } else {
        // ok to go
        ret = this.push(this.capture.slice(this.progress, this.progress+adv));
        this.progress += adv;
        // console.log('y');

        // did recent push just make us out of samples?
        // there is a bug here
        if( this.repeat && ( (this.progress+adv) > this.capture.length ) ) {
          this.progress = 0;
        }

      }

    }

    if(this.rate !== 0 ) {
      this.limiter.do(work.bind(this));
    } else {
      work();
    }

  }
}



module.exports = {
    F64CaptureStream
  , ByteCaptureStream
  , ByteTurnstileStream
  , F64GainStream
  , CF64StreamSin
  , StreamArrayType
  , IFloatToIShort
  , IFloatToIShortMetered
  , IShortToCF64 : IShortToIFloat // alias
  , IShortToIFloat
  , CF64StreamFFT : IFloatStreamFFT // alias
  , IFloatStreamFFT
  , DummyStream
  , RemoveCpStream
  , StreamToUdp
  , AddCounterStream
  , RateStreamHexFile
  , IFloatEqStream
};
