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

const fftChain = require('../build/Release/TxChain1.node');

var assert = require('assert');

const argmax = require( 'compute-argmax' );


let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}




describe("Native Tx Chain", function () {

  let raw_mod;
  let raw_mod_chunk;
  let fft_with_cp_hex;
  let fft_with_cp_ifloat;
  let sin_1024;

  before(()=>{
    // raw_mod = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk_mod.hex');
    // raw_mod_chunk = raw_mod.slice(0,83*1024);

    // fft_with_cp_hex = mutil.readHexFile('test/data/native_fft_256_cp.hex');
    // fft_with_cp_ifloat = mutil.complexToIFloatMulti(mutil.IShortToComplexMulti(fft_with_cp_hex));

    sin_1024 = mutil.readHexFile('test/data/sin_1024.hex');
  });



it('Native Tx Chain1', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.constant(0), function () {

    let print = false;
    let savePlot = false;

    let sArray;
    let dut;
    let cap;

    let arrayChunkDone = function() {
    }

    // let source = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );

    let source = new streamers.StreamArrayType({},{type:'Uint32', chunk:16});

    source.load(sin_1024);


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


      cap = new streamers.F64CaptureStream({},{print:false,disable:false});

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



}); // describe


