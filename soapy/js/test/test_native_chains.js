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

const exampleChain = require('../build/Release/ExampleChain.node');
const turnstileChain = require('../build/Release/TurnstileChain.node');

// const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');

var assert = require('assert');

const argmax = require( 'compute-argmax' );

// var gnuplot = require('gnu-plot');





describe("Native Example Chain", function () {


// randomized test which takes a random array of floats
// loads them into a StreamArrayType, and then streams them into 
// F64CaptureStream object.  The input and output are verified to be identical
// the chunk size is randomized.
// This is a good example of how to use promises and jsverify together (required for how I am using streams)
it('Example Chain chunk', function(done) {
  this.timeout(5000);
  let t = jsc.forall(jsc.integer(1,0xfffff), function (length) {

    let print = false;

    // length = 18;
    if( print ) {
      console.log('test with ' + length + " (" + length*4 + " bytes)")
    }

    let arrayIn = [];
    let expected = [];

    for(let i = 0; i < length; i++) {
      arrayIn[i] = i;
      expected[i] = i*8;
    }

    let bufferIn = mutil.IShortToStreamable(arrayIn);

    let bufferOut = Buffer.alloc(0);

    let onNewData = (res) => {
      // console.log('stream callback with ' + res.length);
      // console.log(res);
      bufferOut = Buffer.concat([bufferOut, res]);
    };

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      // this.resolveCopy = resolve;
      setImmediate(() => {

        // something is weird about this callback, it came from native code and resolve wont work without wrapping it in setImmediate first?
        let streamShutdown = () => {
          if(print) {
            console.log('stream closed');
          }
          setImmediate(streamShutdownAftermath);
          // setImmediate(()=>{resolve(true)});
        };

        let streamShutdownAftermath = () => {
          if(print) {
            console.log('converting');
          }
          let outputAsArray = mutil.streamableToIShort(bufferOut);
          if(print) {
            console.log('checking');
          }
          // console.log(outputAsArray.slice(0,10));
          // mutil.writeHexFile('output.hex', outputAsArray);
          // mutil.writeHexFile('expected.hex', expected);
          // assert.deepEqual(outputAsArray, arrayIn);
          // assert(_.isEqual(outputAsArray, arrayIn));
          assert(_.isEqual(outputAsArray, expected));
          resolve(true);
        }

        exampleChain.setStreamCallbacks(onNewData,streamShutdown);

        exampleChain.startStream(); // starts threads

        let fullAdd = false;

        if( fullAdd ) {
          exampleChain.writeStreamData(bufferIn, bufferIn.length);
        } else {
          let pos = 0;
          while(pos < length*4) {
            let pull = jsc.random(1,0xfff) * 4;
            // let pull = 0x7ffff;//jsc.random(1,0x3fffff);
            // let pull = 8;//jsc.random(1,0x3fffff);
            let slc = bufferIn.slice(pos, pos+pull);
            // console.log(slc);

            exampleChain.writeStreamData(slc, slc.length);

            pos += pull;
          }
        }


        setTimeout(()=>{
          if(print) {
            console.log('will stop');
          }
          exampleChain.stopStream();
        },100);

        // resolve( Math.random() > 0.0006 );
        // resolve( _.isEqual(input, cap.capture ) );
        // resolve(true)
      });
    }.bind(this)); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 20};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it






}); // describe



// copied from ByteGen1 because it was in a different file and I'm lazy
class ByteGen2 extends Stream.Readable{
  
  constructor(options, options2){
    super(options); //Passing options to native class constructor. (REQUIRED)
    this.count = 3; // number of chunks to readout
    this.chunk = 0x8;


    this.thebuffer = new ArrayBuffer(this.chunk); // in bytes
    this.uint8_view = new Uint8Array(this.thebuffer);
    // this.uint32_view = new Uint32Array(this.thebuffer);
    // this.uint16_view = new Uint16Array(this.thebuffer);

    for(let i = 0; i < this.chunk; i++) {
      // let im = i << 16;
      // let re = 0x42;
      // this.uint32_view[i] = im | re;
      this.uint8_view[i] = i;
    }

  }
    
  _read(sz){

    let ret = true;
    while(ret && this.count > 0) {
      ret = this.push(this.uint8_view);
      this.count--;
    }
  }
}



describe("Native Turnstile Chain", function () {


it('Native Turnstile properly removes samples', (done) => {
  let t = jsc.forall(jsc.integer(0,23), function (initialAdvance) {

    let print = false;

    // let initialAdvance = 2;

    if(print) {
      console.log('');
      console.log('');
      console.log('');
      console.log('test w/ advance: ' + initialAdvance);
    }

    let source = new ByteGen2({}, {});

    // step 4 set a pending advance right away before any streaming happens
    let afterPipe = function(cls) {
      if(print) {
        console.log('afterPipe');
      }
      cls.advance(initialAdvance);
    }



    

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {

      let afterShutdown = function() {
        // console.log('gotShutdown');
        // cls.advance(initialAdvance);

        // console.log('final:');
        // console.log(cap.capture);
        const expected = [0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 ].slice(initialAdvance);

        assert(_.isEqual(expected, cap.capture));

        resolve(true);
      }

      // step 1 make object
      let turnstileWrapped = new nativeWrap.WrapNativeTransformStream({},{print:print},turnstileChain, afterPipe, afterShutdown);


      // step 2 extend object
      turnstileWrapped.advance = function(a) {
        this.nativeStream.turnstileAdvance(a);
      }


      // build capture
      let cap = new streamers.ByteCaptureStream({},{disable:false,print:print});
      
      // step 3 call pipe
      source.pipe(turnstileWrapped).pipe(cap);



      setTimeout(() => {try{
        turnstileWrapped.stop();
      }catch(e){reject(e)}},10);


    });
  });

  const props = {tests: 30};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});


}); // describe
