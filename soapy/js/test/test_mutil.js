const jsc = require("jsverify");
const assert = require('assert');
const mutil = require("../mock/mockutils.js");
const mt = require('mathjs');
const _ = require('lodash');
const range = require('pythonic').range;
const streamers = require('../mock/streamers.js'); // only for 1 test




describe("test conv and back", function () {


  it('arrayToIShort', function(done) {
    let a = [0,0,0,0xf];
    let r = mutil.arrayToIShort(a);
    // console.log(r.toString(16));
    assert(r == 0x0f000000);
    done();
  });

  // IShortEqualTol compares the real and imag parts of a 32bit number (this type is called IShort)
  // we pick two random values for real and imag.
  // we randomly tweak the real and imag a random +1 or -1
  // then we compare
  // we also add 3 to force a failure and then theck that
  it('IShortEqualTol', function(done) {
    let t = jsc.forall(jsc.integer(0xf,0xfff0),jsc.integer(0xf,0xfff0), function (a,b) {
      return new Promise(function(resolve, reject) {
        setImmediate(() => {try{

          let input = ((a<<16) | b)>>>0;

          let a_copy = a;
          let b_copy = b;

          // tweak lower (real) ?
          if(!!jsc.random(0,1)) {
            let sign = 1;
            if(!!jsc.random(0,1)) {
              sign = -1;
            }
            a_copy += sign;
          }

          // tweak upper (imag) ?
          if(!!jsc.random(0,1)) {
            let sign = 1;
            if(!!jsc.random(0,1)) {
              sign = -1;
            }
            b_copy += sign;
          }

          let compare = ((a_copy<<16) | b_copy)>>>0;

          assert( mutil.IShortEqualTol(input, compare) );

          let compare_fail_l = ((a_copy<<16) | (b_copy+3))>>>0;
          let compare_fail_h = (( (a_copy+3)<<16) | (b_copy+3))>>>0;

          assert( !mutil.IShortEqualTol(input, compare_fail_l) );
          assert( !mutil.IShortEqualTol(input, compare_fail_h) );


          resolve(true);
        }catch(e){reject(e)}});
      }); // promise
    }); // forall

    // when using 'array nat' above jsverify internally limits the length of the array
    // to the log of the size param, while each element is a random value [0, size]
    const props = {size: 10000000000, tests: 10000};
    jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
  }); // it


  it('IFloat', function() {

      let asComplex = [mt.complex(0.0,0.1), mt.complex(0.2,0.3)];

      let backAgain = mutil.complexToIShortMulti(asComplex);

      let ifloat = [0.0,0.1,0.2,0.3];

      let backAgain2 = mutil.IFloatToIShortMulti(ifloat);

      assert.deepEqual(backAgain, backAgain2);

      // mutil.printHex(backAgain);
      // mutil.printHex(backAgain2);
  });

  it('Complex IFloat', function() {

      let asComplex = [mt.complex(0.0,0.1), mt.complex(0.2,0.3)];

      // let backAgain = mutil.complexToIShortMulti(asComplex);

      let conv = mutil.complexToIFloatMulti(asComplex);

      let ifloat = [0.0,0.1,0.2,0.3];

      // let backAgain2 = mutil.IFloatToIShortMulti(ifloat);

      assert.deepEqual(conv, ifloat);

      // mutil.printHex(backAgain);
      // mutil.printHex(backAgain2);
  });



  jsc.property("convert in", jsc.constant(0), function () {
    let a = jsc.random(0,255); // inclusive
    let b = jsc.random(0,255); // inclusive
    let c = jsc.random(0,255); // inclusive
    let d = jsc.random(0,255); // inclusive

    let input = new ArrayBuffer(4);
    input[0] = a;
    input[1] = b;
    input[2] = c;
    input[3] = d;

    let r = mutil.bufToI32(input);

    // console.log(r);

    return true;
  });

  jsc.property("convert out", jsc.constant(0), function () {
    let a = jsc.random(0,0xffffffff); // inclusive

    let r = mutil.I32ToBuf(a);

    // console.log(r);

    return true;
  });

  jsc.property("convert in out", jsc.constant(0), function () {
    let initial = jsc.random(0,0xffffffff); // inclusive

    let asParts = mutil.I32ToBuf(initial);

    let asWhole = mutil.bufToI32(asParts);

    // console.log(initial);
    // console.log(asWhole);
    // console.log('');


    // console.log(r);

    return initial == asWhole;
  });


  jsc.property("streamable convert in out", jsc.integer(0x1,0xff), function (length) {

    let in1 = Array(0);
    for(let x of range(length)) {
      let a = jsc.random(0,0xffffffff); // inclusive
      in1.push(a);
      // console.log(a.toString(16));
    }

    let asBuffer = mutil.IShortToStreamable(in1);
    let out1 = mutil.streamableToIShort(asBuffer);

    assert.deepEqual(in1, out1);

    return true;
  });

  jsc.property("streamable Buffer() convert in out", jsc.integer(0x1,0xff), function (length) {

    let in1 = Array(0);
    for(let x of range(length)) {
      let a = jsc.random(0,0xffffffff); // inclusive
      in1.push(a);
      // console.log(a.toString(16));
    }

    let asBuffer = mutil.IShortToStreamable(in1);

    let asBuffer2 = Buffer.from(asBuffer);

    let out1 = mutil.streamableToIShort(asBuffer);

    assert.deepEqual(in1, out1);

    return true;
  });



it('streamableToIFloat works', function(done) {
  let t = jsc.forall('array number', function (input) {

    // generate a random chunk size
    let rchunk = jsc.random(1,input.length);

    // console.log(input);

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
    let cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {
        // resolve( Math.random() > 0.0006 );
        let backAsFloat = mutil.streamableToIFloat(cap.capture);
        // console.log("back " + backAsFloat);
        assert( _.isEqual(input, backAsFloat ) );
        resolve(true);
      });
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000, tests: 1000};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


  // jsc.property("name 3", "bool", function (b) {
  //   let a = jsc.random(0,3); // inclusive
  //   return a < 3;
  // });

  it('IFloatToComplexMulti', function() {
    let floats = [1,2,3,4];
    let expected = [mt.complex(1,2),mt.complex(3,4)];
    let got = mutil.IFloatToComplexMulti(floats);
    assert.deepEqual(got, expected);
  });


  it('correctly converts Multi functions', function(done) {

    let t = jsc.forall('array nat', function (in1) {

      let asComplex = mutil.IShortToComplexMulti(in1);

      let backAgain = mutil.complexToIShortMulti(asComplex);

      // console.log(in1.slice(0,4));
      // console.log(backAgain.slice(0,4));

      // console.log(asComplex);

      return new Promise(function(resolve, reject) {
        setImmediate(() => {

          let pass = true;

          for(let i = 0; i < in1.length; i++) {
            let deltaLower = ((in1[i]&0xffff)>>>0) - ((backAgain[i]&0xffff)>>>0);
            let deltaUpper = (((in1[i]>>16)&0xffff)>>>0) - (((backAgain[i]>>16)&0xffff)>>>0);

            // if( deltaLower !== 0 || deltaUpper !== 0 ) {
            //   console.log(`${in1[i].toString(16)} -> ${backAgain[i].toString(16)} (${deltaUpper} ${deltaLower})`);
            // }

            if( Math.abs(deltaLower) > 1 || Math.abs(deltaUpper) > 1 ) {
              pass = false;
            }
          } // for

          resolve(pass);
        }); // setTimeout
      }); // promise
    }); // forall

    let oneFail = false;

    let props = {size:0xffffffff, tests: 1000};
    jsc.check(t, props).then( r => r === true ? oneFail = true : done(new Error(JSON.stringify(r))));
    props.rngState = "8e1da6702e395bc84f"; // this is a known bad state
    jsc.check(t, props).then( r => r === true ? oneFail = true : done(new Error(JSON.stringify(r))));

    if(!oneFail) {
      done(); // this oneFail flag allows us to avoid calling done more than once
    }

  }); // it


}); // describe







describe("test out of socket", function () {

  // it("are greater than or equal to 0", function () {
  //   var property = jsc.forall("nat", function (n) {
  //     return n >= 0;
  //   });

  //   jsc.assert(property);
  // });

  const s1 = '{"type":"Buffer","data":[0,0,0,0,1,0,0,16]}';
  const s2 = '{"type":"Buffer","data":[0,0,0,0,2,0,0,16]}';


  jsc.property("out of sock p1", jsc.constant(0), function () {

    let buf = Buffer.from(JSON.parse(s1).data);

    let addr = mutil.bufToI32(mutil.sliceBuf4(buf, 0));
    let word = mutil.bufToI32(mutil.sliceBuf4(buf, 4));

    // console.log('addr: ' + addr.toString(16));
    // console.log('word: ' + word.toString(16));


    return addr == 0 && word == 0x10000001;
  });

  jsc.property("out of sock p2", jsc.constant(0), function () {

    let buf = Buffer.from(JSON.parse(s2).data);

    let addr = mutil.bufToI32(mutil.sliceBuf4(buf, 0));
    let word = mutil.bufToI32(mutil.sliceBuf4(buf, 4));

    // console.log('addr: ' + addr.toString(16));
    // console.log('word: ' + word.toString(16));


    return addr == 0 && word == 0x10000002;
  });



  // jsc.property("into of sock", "bool", function (bb) {

  //   let buf = mutil.I32ToBuf(14152342);

  //   let buf2 = Buffer.from( buf );

  //   let buf3 = Buffer.from([buf[0],buf[1],buf[2],buf[3]]);

  //   let buf4 = Buffer.concat([buf2,buf3]);

  //   console.log(buf);
  //   console.log(buf2);
  //   console.log(buf3);
  //   console.log(buf4);

  //   // console.log('addr: ' + addr.toString(16));
  //   // console.log('word: ' + word.toString(16));


  //   // return addr == 0 && word == 0x01000010;
  //   return true;
  // });


});


describe("simple converters", function () {
  it('fftShift even length', (done) => {

    let a = [1,2,3,4,5,6];

    let b = mutil.fftShift(a);

    assert(!_.isEqual(a,b), "fftShift() had no effect");

    let c = mutil.fftShift(b);

    assert(_.isEqual(a,c), "fftShift(fftShift()) did not result in original data for even length");

    done();
  });


  it('fftShift odd length', (done) => {


    let a = [1,2,3,4,5,6,7];

    let b = mutil.fftShift(a);

    assert( a.length == b.length, "Wrong length after fftShift() with odd length input" );

    assert(!_.isEqual(a,b), "fftShift() had no effect");

    a.sort();
    b.sort();

    assert(_.isEqual(a,b), "Some values were missing after fftShift() with odd length input");

    done();
  });
});


  

describe("riscv atan", function () {

  // code here gets run even if these tests don't
  let atan_results;

  // put reading from disk in a before() to improve performance of other tests
  before(()=>{
    atan_results = mutil.readHexFile('test/data/atan_in_out_0.hex');
  });

  it.skip('atan cooked', function(done) {

    let len = atan_results.length;
    // len = 8;

    let mag_fac_a = [];
    let arg_fac_a = [];

    for(let i = 0; i < len; i+=2) {
      let input = atan_results[i];
      let output = atan_results[i+1];


      let temp_complex_pre = input;
      let temp_complex_pre_ = mutil.IShortToComplexMulti([temp_complex_pre])[0];
      let arg = temp_complex_pre_.arg();
      let mag = mt.abs(temp_complex_pre_);

      if( arg < 0 ) {
        arg += 2*Math.PI;
      }
      console.log(`temp_complex_pre_ ${temp_complex_pre_} ${arg} mag: ${mag} input: ${input.toString(16)}`);

      let t0 = output;
      let temp_angle = (t0 & 0xffff)>>>0;
      let temp_mag = ((t0>>16) & 0xffff)>>>0;
      console.log(`temp_angle ${temp_angle} mag: ${temp_mag}`);

      let mag_fac = temp_mag / mag;
      let arg_fac = temp_angle / arg;

      console.log(`arg factor: ${arg_fac} mag factor: ${mag_fac}`);

      mag_fac_a.push(mag_fac);
      arg_fac_a.push(arg_fac);

    }

    console.log(_.mean(arg_fac_a));
    console.log(_.mean(mag_fac_a));

    done();
  });

  it('atan compare', function(done) {

    let bad = 0x00f5b05d;

    let len = atan_results.length;
    // len = 8;

    for(let i = 0; i < len; i+=2) {
      let input = atan_results[i];
      let expected_output = atan_results[i+1];

      node_output = mutil.riscvATAN(input);


      // console.log('------------');
      // console.log(expected_output.toString(16));
      // console.log(node_output.toString(16));
      // console.log('');

      assert(mutil.IShortEqualTol(node_output, expected_output, 4));

    }

    done();
  });




}); // describe

describe("misc", function () {

  it('ifft', function(done) {
    // let input1 = [1,0,2,0,3,0,4,0];
    // let expected_output1 = [ 10, 0, -2, -2, -2, 0, -2, 2 ];

    // input2 = [1,-1,2,4,3,0,4,0];
    // let expected_output2 = [ 10, 3, -6, -3, -2, -5, 2, 1 ];
    
    let in_c = [
    mt.complex(1,-1),
    mt.complex(2,4),
    mt.complex(3,0),
    mt.complex(4,0)
    ];

    // console.log(in_c[1]);

    let output = mutil.ifft(in_c);

    // console.log(output);

    done();

  });

}); // describe
