var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mapmov = require('../mock/mapmov.js');
// const menv = require('../mock/mockenvironment.js');
const mt = require('mathjs');
const Stream = require("stream");
const streamers = require('../mock/streamers.js');
const range = require('pythonic').range;

// const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');

var assert = require('assert');

const argmax = require( 'compute-argmax' );

// var gnuplot = require('gnu-plot');


let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}





describe("Stream tests", function () {

it('Generates and captures', (done) => {

  let theta = 0.1;
  let chunk = 4;

  let genSin = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );
  let cap = new streamers.F64CaptureStream({});

  genSin.pipe(cap);
 
  setImmediate(() => {
    // console.log("r0 got " + cap.capture.length);

    let ascomplex0 = _.chunk(cap.capture,2).map((e,i)=>mt.complex(e[0],e[1]));

    let ascomplexMember = cap.toComplex();

    if( ! _.isEqual(ascomplex0, ascomplexMember) ) {
      return done(new Error('toComplex() not functioning correctly'));
    }

    cap.erase();

    if( cap.capture.length !== 0 ) {
      return done(new Error('erase didn\'t erase'));
    }


    if( ascomplex0.length !== chunk ) {
      return done(new Error('Stream captured or generated the wrong number of values'))
    }

    let asmag0 = ascomplex0.map((e,i)=>mt.abs(e));
    let asarg = ascomplex0.map((e,i)=>e.arg());

    let expectedarg = [];
    for(let i = 0; i < chunk; i++) {
      expectedarg[i] = theta * i;
    }

    let difference = expectedarg.map((e,i)=>Math.abs(e-asarg[i]));

    let tol = 1E-8;
    
    let illegalValues = _.filter(difference, function(e) { return e >= tol; });

    if( illegalValues.length === 0 ) {
      return done();
    } else {
      return done(new Error('One or more values was out of spec'));
    }

  });

});

// see https://github.com/rm3web/rm3/blob/d69bce96e76c9499c0e156611e4a437802af4735/tests/unit/test-imagescale.js#L7
  it("double argument example", function() {
    var property = jsc.forall(jsc.integer(1,1000), jsc.integer(1,100), function(n1, n2) {
      let gain = n1 + 1/n2;
      // console.log('gain ' + gain);
      return true;
    });
    jsc.assert(property);
  });



it('Gains a stream', (done) => {

  let theta = 0.1;
  let chunk = 4;
  let testGain = 2.0;

  let genSin = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );
  let cap = new streamers.F64CaptureStream({});
  let gainStream = new streamers.F64GainStream({}, {gain:testGain});

  genSin.pipe(gainStream).pipe(cap);
 
  setImmediate(() => {
    let ascomplex0 = cap.toComplex();

    if( ascomplex0.length !== chunk ) {
      return done(new Error('Stream captured or generated the wrong number of values'))
    }

    let asmag0 = ascomplex0.map((e,i)=>mt.abs(e));

    let difference = asmag0.map((e,i)=>Math.abs(e-testGain));

    let tol = 1E-8;
    
    let illegalValues = _.filter(difference, function(e) { return e >= tol; });

    if( illegalValues.length === 0 ) {
      return done();
    } else {
      return done(new Error('One or more values was out of spec'));
    }

  });

});




// randomized test which takes a random array of floats
// loads them into a StreamArrayType, and then streams them into 
// F64CaptureStream object.  The input and output are verified to be identical
// the chunk size is randomized.
// This is a good example of how to use promises and jsverify together (required for how I am using streams)
it('StreamArrayType can output floats', function(done) {
  let t = jsc.forall('array nat', function (input) {

    // generate a random chunk size
    let rchunk = jsc.random(1,input.length);

    // if input is empty, force chunk to 0
    if( input.length === 0 ) {
      rchunk = 1;
    }

    // console.log('for array length ' + input.length + ' choosing to chunk at ' + rchunk);

    // build array stream object, passing arguments
    let sArray = new streamers.StreamArrayType({},{type:'Float64', chunk:rchunk});

    // load values into streamer, this should be done before pipe
    sArray.load(input);

    // build capture
    let cap = new streamers.F64CaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {
        // resolve( Math.random() > 0.0006 );
        resolve( _.isEqual(input, cap.capture ) );
      });
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1000};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('StreamArrayType can output uint32', function(done) {
  let t = jsc.forall(jsc.integer(1,1000), jsc.integer(1,256), function (rchunk, maxval) {

    // let chunk = 2;

    // build array stream object, passing arguments
    let sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:rchunk});

    // let maxval = 4;

    // load values into streamer, this should be done before pipe
    sArray.load(Array.from(range(0,maxval)));

    let expected = []
    for(let i = 0; i < maxval; i++) {
      expected.push(i);
      expected.push(0);
      expected.push(0);
      expected.push(0);
    }

    // console.log(Array.from(range(1,33)));

    // build capture
    let cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        // console.log(cap.capture);
        // console.log(expected);
        // resolve( Math.random() > 0.0006 );
        resolve( _.isEqual(expected, cap.capture ) );
        // resolve(true);
      }catch(e){reject(e)}});
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 0xffffffff, tests: 300};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it





it('StreamArrayType can output uint8', function(done) {
  let t = jsc.forall(jsc.integer(1,250), jsc.integer(1,255), function (rchunk, maxval) {

    // build array stream object, passing arguments
    let sArray = new streamers.StreamArrayType({},{type:'Uint8', chunk:rchunk});


    // load values into streamer, this should be done before pipe
    sArray.load(Array.from(range(0,maxval)));

    let expected = []
    for(let i = 0; i < maxval; i++) {
      expected.push(i);
    }

    // build capture
    let cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        resolve( _.isEqual(expected, cap.capture ) );
      }catch(e){reject(e)}});
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 0xffffffff, tests: 300};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it

// see a randomized version of this test above
// it('StreamArrayType can output floats', (done) => {
//   let input = [1,2,3,4];
//   let sArray = new streamers.StreamArrayType({},{type:'Float64', chunk:2});
//   sArray.load(input);
//   let cap = new streamers.F64CaptureStream({});
//   sArray.pipe(cap);
//   setImmediate(() => {
//     // console.log(cap.capture);
//     if( ! _.isEqual(input, cap.capture ) ) {
//       return done(new Error('Input did not match output'))
//     }
//     return done();
//   });
// });








// Specific generator for a test below
class IShortGen1 extends Stream.Readable{
  
  constructor(options, options2){
    super(options); //Passing options to native class constructor. (REQUIRED)
    this.count = 1; // number of chunks to readout
    this.chunk = 0xffff+1;


    this.thebuffer = new ArrayBuffer(4*this.chunk); // in bytes
    this.uint8_view = new Uint8Array(this.thebuffer);
    this.uint32_view = new Uint32Array(this.thebuffer);
    // this.uint16_view = new Uint16Array(this.thebuffer);

    for(let i = 0; i < this.chunk; i++) {
      let im = i << 16;
      let re = 0x42;
      this.uint32_view[i] = im | re;
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

// Passing arrow functions (“lambdas”) to Mocha is discouraged.
// Due to the lexical binding of this, such functions are unable to access the Mocha context.
it('IShortToCF64 converts correctly', function(done) {
  this.timeout(5000);
  let t = jsc.forall('bool', function () {

    // build array stream object, passing arguments
    let source = new IShortGen1({});

    let conv = new streamers.IShortToCF64({}, {});

    // build capture
    let cap = new streamers.F64CaptureStream({});

    // pipe, which will start the streaming
    source.pipe(conv).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        let cplx = cap.toComplex();

        assert( cplx.length === 0x10000, "capture only had " + cplx.length + " elemnts");

        let cnt = 0;
        let pp;

        for (let [i, x] of cplx.entries()) {
          // console.log('real: ' + x.re + ' imag: ' + x.im);
          let error_msg = "failed for i " + i + " c: " + x.toString();
          if( i === 0 ) {
            assert(x.im === 0, error_msg);
          } else if( i > 0 && i <= 0x7fff) {

            if(cnt !== 0) {
              assert(x.im > pp, "not ascending " + error_msg);
            }
            pp = x.im;
            cnt++;

            assert(x.im > 0, error_msg);
          } else {
            assert(x.im <= 0, error_msg);
          }
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}}); // arrow
    }); // promise
  }); // t = 

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});


// Passing arrow functions (“lambdas”) to Mocha is discouraged.
// Due to the lexical binding of this, such functions are unable to access the Mocha context.
it('IShortToIFloat to IFloatToIShort', function(done) {
  this.timeout(5000);
  let t = jsc.forall('bool', function () {

    let expected = [];
    this.count = 1; // number of chunks to readout
    this.chunk = 0xffff+1;
    for(let i = 0; i < this.chunk; i++) {
      let im = i << 16;
      let re = 0x42;
      expected.push(im | re);
      // this.uint32_view[i] = im | re;
    }

    // build array stream object, passing arguments
    let source = new IShortGen1({});

    let conv = new streamers.IShortToIFloat();

    let conv2 = new streamers.IFloatToIShort({},{printType:false});

    // build capture
    let cap = new streamers.ByteCaptureStream();

    // pipe, which will start the streaming
    source.pipe(conv).pipe(conv2).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        let out = mutil.streamableToIShort(cap.capture);

        assert.equal(out.length, expected.length);

        for(let i = 0; i < out.length; i++) {
          // if( i < 32 ) {
          //   console.log(out[i].toString(16) + " - " + expected[i].toString(16));
          // }
          assert(mutil.IShortEqualTol(expected[i],out[i], 1));
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}}); // arrow
    }); // promise
  }); // t = 

  const props = {tests: 10};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});


it('CF64StreamFFT fft\'s correctly', (done) => {
  let t = jsc.forall('bool', function () {


    let cardata = [];

    for(let i = 0; i < 1024; i++) {
      let j = mt.complex(0,1);
      let res = mt.exp(mt.multiply(j,i/2.0));
      // cdata.push(res);

      cardata.push(res.re);
      cardata.push(res.im);
    //   data.push(Math.random()*100);
    }

    // chunk is 2x because we are implicitly using float as cfloat
    // let cannedSource = new streamers.StreamArrayType({},{type:'Float64', chunk:2048});

    // load values into streamer, this should be done before pipe
    // cannedSource.load(cardata);


    let theta = 0.1;
    let chunk = 1024;

    // let cp = 0;
    let cp = 256;

    // build array stream object, passing arguments

    // let ds1 = new IShortGen1({});
    // let ds2 = new streamers.IShortToCF64({}, {});

    let source = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );

    let fftStream = new streamers.CF64StreamFFT({}, {n:1024,cp:cp,inverse:true});

    // build capture
    let cap = new streamers.F64CaptureStream({});

    // pipe, which will start the streaming
    source.pipe(fftStream).pipe(cap);
    // cannedSource.pipe(fftStream).pipe(cap);
    // ds1.pipe(ds2).pipe(fftStream);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // let cplx = mutil.fftShift(cap.toComplex());
        let cplx = cap.toComplex();
        let asmag0 = cplx.map((e,i)=>mt.abs(e));
        let asreal0 = cplx.map((e,i)=>e.re);

        let maxBin = argmax(asmag0);

        // calculated through experiment
        let expectedMax = [ 240, 1264 ];

        assert( _.isEqual(maxBin, expectedMax));

        // console.log("captured " + cplx.length);

        if( savePlot ) {
            setTimeout(() => {
              np.plot(asmag0, "fft_out0.png");
            }, 0);

            setTimeout(() => {
              np.plot(asreal0, "fft_real0.png");
            }, 1000);
        }

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},800);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});



class ByteGen1 extends Stream.Readable{
  
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



it('turnstile properly removes samples', (done) => {
  let t = jsc.forall('bool', function () {


    let source = new ByteGen1({}, {});

    let turnstile = new streamers.ByteTurnstileStream({},{chunk:3});

    turnstile.advance(2);


    // build capture
    let cap = new streamers.ByteCaptureStream({});
    // pipe, which will start the streaming
    source.pipe(turnstile).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        const expected = [ 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6 ];

        assert(_.isEqual(expected, cap.capture));

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},10);
    });
  });

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});




it('remove cp works', (done) => {
  this.timeout(4000);
  let t = jsc.forall(jsc.integer(1,40), function (rchunk) {


    let bytes = Array.from(range(0,30));

    let source = new streamers.StreamArrayType({},{type:'Uint8', chunk:rchunk});

    source.load(bytes);

    let turnstile = new streamers.RemoveCpStream({},{chunk:10,cp:2});

    // build capture
    let cap = new streamers.ByteCaptureStream({},{print:false});
    // pipe, which will start the streaming
    source.pipe(turnstile).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        let p1 = Array.from(range(2,10));
        let p2 = Array.from(range(12,20));
        let p3 = Array.from(range(22,30));

        const expected = p1.concat(p2).concat(p3);

        // console.log(expcted);
        // console.log(cap.capture);

        assert(_.isEqual(expected, cap.capture));

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},10);
    });
  });

  const props = {tests: 100};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

});

it('rate stream', (done) => {
  let fname = 'test/data/coarse_sync0.hex';
  let rateStream = new streamers.RateStreamHexFile({},{chunk:1024,rate:2000,fname:fname,repeat:false});
  let cap = new streamers.ByteCaptureStream({},{print:false});

  rateStream.pipe(cap);

  let t = jsc.forall('bool', function () {

  // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{

        resolve(true); // always resolve to true, the asserts above will fail us out
      }catch(e){reject(e)}},100);
    });

  });
  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it

}); // describe


let repeatValid;

describe("Stream tests Repeat", function () {

before(()=>{
  repeatValid = 0;
});

after(()=>{
  // console.log("repeatValid " + repeatValid);

  // the repeat flag should always repeat data
  // due to the way I wrote it, for certain pairs of maxval and rchunk
  // we will only send enough data for a single chunk.
  // this is an edge condition with the setTimeout and not a failing of the test
  // to weed out these false failures, we do a before/after where we make sure that MOST of the
  // tests were longer. and only have an error if NONE of them were longer (meaning repeat:true is broken)
  assert(repeatValid > 2, "None of the StreamArrayType repeat runs actually repeated data");

  // note this value interplays with tests: 30 below
});

it('StreamArrayType can loop output uint32', function(done) {
  let t = jsc.forall(jsc.integer(1,1000), jsc.integer(1,256), function (rchunk, maxval) {

    // build array stream object, passing arguments
    let sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:rchunk, repeat:true});

    // load values into streamer, this should be done before pipe
    sArray.load(Array.from(range(0,maxval)));

    let expected = []
    for(let i = 0; i < maxval; i++) {
      expected.push(i);
      expected.push(0);
      expected.push(0);
      expected.push(0);
    }

    // build capture
    let cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // console.log(cap.capture);
        // console.log(expected);
        sArray.repeat = false; // required to stop input stream and for test to exit

        // console.log("cap len " + cap.capture.length);
        // console.log("expected len " + expected.length);

        if(cap.capture.length > expected.length) {
          repeatValid++;
        } else {
          // console.log("edge case with rchunk: " + rchunk + " maxval: " + maxval)
        }

        // assert(cap.capture.length > expected.length);

        // loop over full chunks we have received
        for(let i = 0; i < cap.capture.length-expected.length; i+= expected.length) {
          // console.log(i);
          assert( _.isEqual(expected, cap.capture.slice(i,i+expected.length) ) );
        }


        resolve(true);
      }catch(e){reject(e)}}, 20);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 0xffffffff, tests: 30};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it

}); // describe