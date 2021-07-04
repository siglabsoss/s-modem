var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const CoarseSync = require('../mock/coarsesync.js');
const streamers = require('../mock/streamers.js');
const csv = require('csv-parser');
const fs = require('fs');
const enumerate = require('pythonic').enumerate;

var assert = require('assert');

let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}


// let coarse0 = mutil.readHexFile('test/data/coarse_sync0.hex');
// let ideal_out0 = mutil.readHexFile('test/data/mapmov_out0.hex');


function parse123(vv, tag=[1,0xeea4649a,3], print=false) {
  let vectors = [];
  let markp;

  for(let i = 2; i < vv.length; i++) {
    if(
     vv[i-2] === tag[0] &&
     vv[i-1] === tag[1] &&
     vv[i-0] === tag[2]) {
      // console.log('match ' + i);

      if(typeof markp === 'undefined') {
        markp = i;
      } else {
        if( print ) {
          console.log('vectors[' + vectors.length + '] len ' + (i-markp-3) );
        }
        vectors.push(
          vv.slice(markp+1, (i-2))
        );
        markp = i;
        // break;
      }

    } // if
  } // for

  // for(let i = 0; i < 1024; i++) {
  //   console.log(vectors[0][i].toString(16));
  // }
  return vectors;
}

// fixme, move these to before()
const vectors0 = parse123(mutil.readHexFile('test/data/coarse_sync0.hex'),[1,2,3]);
const vectors1 = parse123(mutil.readHexFile('test/data/coarse_sync1.hex'),[1,0xeea4649a,3]);
// const lastrun = parse123(mutil.readHexFile('../../../../sim/verilator/debug_sync/cs00_out.hex'),[1,0xeea4649a,3]);
const vectors2 = parse123(mutil.readHexFile('test/data/coarse_sync2.hex'),[1,0xeea4649a,3]);

const raw = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk.hex');
const raw_mod = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk_mod.hex');

// temp complex is acumulated.  so each index has the sum of the previous and a not show number
const expected_temp_complex = [
0x728f15d,
0xe96e251,
0xe30e2e3,
0x160ed425,
0x1d60c56a,
0x2486b6c3,
0x2bf1a7b7,
0x2b8aa848,
0x3c059e59,
0x3cd490e3,
0x3b058000,
0x42e68000,
0x440f8000,
0x4edc8000,
0x59bf8000,
0x7acf12d,
0xf1ae21e,
0xeace2ac,
0x1689d3e9,
0x1ddfc532
];

// actual results from each stage, not summed
const expected_output_onetone_calculation_location = [
0xdff32,
0x147fd02,
0xffb3005d,
0x9eeef61,
0x3b5f6c3,
0x10ff32,
0x148fd02,
0xffb1005a,
0x10a2f4cf,
0x9cef87,
0xfbfeec74,
0x15afa3e,
0x32ff76,
0x320f789,
0xbe0ef37,
0x66fefc,
0x149fd01,
0xffab0055,
0x9f0ef62,
0x3bdf6c4

];


describe("coarse", function () {

it('conj mult', (done) => {


  let a = mutil.IShortToComplexMulti(vectors0[0]);
  let b = mutil.IShortToComplexMulti(vectors0[1]);
  let c = vectors0[2];

  // console.log(a.slice(0,8));
  // console.log(b.slice(0,8));

  let d = CoarseSync.conj_mult(a,b);

  // console.log(d[771].toString());
  // console.log(d[772].toString());
  // console.log(d[773].toString());

  let dd = mutil.complexToIShortMulti(d);

  for(let i = 0; i < c.length; i++) {
    let delta = c[i] - dd[i];
    assert(delta < 2);
    // if( delta !== 0 ) {
    //   console.log('i ' + i + ' delta ' + delta);
    // }
  }

  

  done();
});

// can coarse sync run twice?
// are zeros output correctly during coarse?
it('coarse twice', function(done) {
  this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let sArray;
    let coarse;
    let cap;

    let coarseResults = 0;

    let callback = function(val) {
      // console.log('got coarse callback with 0x' + val.toString(16));
      coarseResults++;
    }

    let chunksPushed = 0;

    let arrayChunkDone = function() {
      // console.log('a chunk was done');

      if(chunksPushed === 10) {
      //   console.log('triggering coarse')
        coarse.triggerCoarse();
      }

      if(chunksPushed === 35) {
      //   console.log('triggering coarse')
        coarse.triggerCoarse();
      }

      chunksPushed++;
    }

    // build array stream object, passing arguments
    sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1280}, arrayChunkDone);

    // load values into streamer, this should be done before pipe
    sArray.load(raw_mod.slice(0,66*1280));

    coarse = new CoarseSync.CoarseSync({},{ofdmNum:20,print2:false,print1:false,print3:false,print4:false}, callback);

    cap = new streamers.ByteCaptureStream({});

    // pipe, which will start the streaming
    sArray.pipe(coarse).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(() => {try{
        // console.log(`cap got ${cap.capture.length/4/1024} frames`);

        assert.equal(2, coarseResults, "coarse did not run twice or issue with callback")

        let vector = [];

        for( let i = 0; i < cap.capture.length; i+=4 ) {
          let ishort = mutil.arrayToIShort(cap.capture.slice(i,i+4));
          vector.push(ishort);
          // console.log(ishort);
        }

        let zeros = _.flatten(_.times(1024, _.constant([0])));

        let zeroRows = 0;

        for( let i = 0; i < vector.length; i+=1024) {
          // console.log(vector.slice(0,6));
          // mutil.printHexPad(vector.slice(i+0,i+6));

          let frame = vector.slice(i+0,i+1024);

          if(_.isEqual(frame, zeros)) {
            zeroRows++;
          }
        }

        assert.equal(zeroRows, 40, "Incorrect number of zero frames in output");


        resolve(true);
      }catch(e){reject(e)}}, 400);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it

it.skip('stream coarse', function(done) {
  this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    // values for the default test
    // 0x3cd490e3
    // 0x34266b97

    // +5
    // 0x38f88b30
    // 0x35816d82


    let temp_complex_pre = 0x3cd490e3;
    let temp_complex_pre_ = mutil.IShortToComplexMulti([temp_complex_pre])[0];
    console.log(`temp_complex_pre_ ${temp_complex_pre_} ${temp_complex_pre_.arg()} mag: ${mt.abs(temp_complex_pre_)}`);

    let t0 = 0x34266b97;
    let temp_angle = (t0 & 0xffff)>>>0;
    let temp_mag = ((t0>>16) & 0xffff)>>>0;
    console.log(`temp_angle ${temp_angle} mag: ${temp_mag}`);




    for( x of expected_output_onetone_calculation_location ) {
      let v = mutil.IShortToComplexMulti([x])[0];
      console.log(`${v.arg()} - ${v.toString()}`);
    }

    let callback = function(val) {
      console.log('got coarse callback with 0x' + val.toString(16));
    }

    // build array stream object, passing arguments
    let sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024});

    // load values into streamer, this should be done before pipe
    sArray.load(raw_mod.slice(11050,31530));
    // sArray.load(raw_mod.slice(11050,11050+1024*5));

    let coarse = new CoarseSync.CoarseSync({},{ofdmNum:20,print2:false,print1:true,triggerAtStart:true}, callback);

    let cap = new streamers.ByteCaptureStream({},{disable:true});

    // // build capture
    // let cap = new streamers.F64CaptureStream({});

    // // pipe, which will start the streaming
    sArray.pipe(coarse).pipe(cap);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        resolve(true);
        // resolve( Math.random() > 0.0006 );
        // resolve( _.isEqual(input, cap.capture ) );
      }catch(e){reject(e)}});
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it

//  there are 80 runs of different offsets in 
it.skip('find offset', function(done) {
  this.timeout(10000);

  const verilatorResults = [];
  let t = jsc.forall('bool', function (junk) {


    // for([i,verilator] of enumerate(verilatorResults)) {
    //   // console.log(JSON.stringify(verilator));
    //   console.log(`${i} Input: ${verilator.input} result: ${verilator.output}`);
    // }
    // return done();

    for(let [i,verilator] of enumerate(verilatorResults)) {

      let callback = function(val) {
        let delta = val - verilator.output;
        // console.log(`${i} ${verilator.output} ${delta}`);
        console.log(`${delta}`);
        // console.log(`${i} find offset coarse callback ${val.toString(16)} expected ${verilator.output.toString(16)}` );
      }

      // build array stream object, passing arguments
      let sArray = new streamers.StreamArrayType({},{type:'Uint32', chunk:1024});

      // load values into streamer, this should be done before pipe
      sArray.load(raw_mod.slice(11050+verilator.input,31530+verilator.input));

      let coarse = new CoarseSync.CoarseSync({},{ofdmNum:20,print2:false,print1:false,print3:false,triggerAtStart:true}, callback);

      let cap = new streamers.ByteCaptureStream({},{disable:true});
      // coarse.triggerCoarse();

      // // build capture
      // let cap = new streamers.F64CaptureStream({});

      // // pipe, which will start the streaming
      sArray.pipe(coarse).pipe(cap);
    }

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setImmediate(() => {try{
        resolve(true);
        // resolve( Math.random() > 0.0006 );
        // resolve( _.isEqual(input, cap.capture ) );
      }catch(e){reject(e)}});
    }); // promise
  }); // forall


   fs.createReadStream('test/data/coarse_log.csv')
      .pipe(csv())
      .on('data', (data) => verilatorResults.push(data))
      .on('end', () => {
        // remove ringbus id, and parse from hex
         for(verilator of verilatorResults) {
          verilator.input = parseInt(verilator.input);
          verilator.output = parseInt(verilator.output,16) & 0xffffff;
        }
        const props = {size: 10000000000, tests: 1};
        jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
      });
}); // it


it.skip('offsets into coarse_sync', function(done) {
  this.timeout(5000);


  for( let i = 0; i < 1024; i++ ) {

    const st = 1024 + i; // start, must be larger than 0
    const stlen = 1280; // length

    let a = mutil.IShortToComplexMulti(raw.slice(st,st+stlen)); // input
    let b = mutil.IShortToComplexMulti(raw.slice(st-1,st+stlen-1)); // input delayed?

    let res =     CoarseSync.coarse_sync(a,b);
    console.log(res);

  }


  done();
});



it.skip('pick random ofdm offsets for verilator', (done) => {

  let top = 4000;
  let stride = 50;

  for(let i = 0; i < top; i+=stride) {
    let pick = Math.max(0, i+ Math.round(Math.random()*15) );

    // console.log(`${pick}`);
    console.log(`OFDM_OFFSET=${pick} make quickt`);
  }
});


it.skip('prototype when I had wrong idea that input delayed was by 1 sample', (done) => {

  const st = 1024; // start, must be larger than 0
  const stlen = 1280; // length

  let a = mutil.IShortToComplexMulti(raw.slice(st,st+stlen)); // input
  let b = mutil.IShortToComplexMulti(raw.slice(st-1,st+stlen-1)); // input delayed?

  console.log('a len ' + a.length);
  console.log('b len ' + b.length);

  let asreal_a = a.map((e,i)=>e.re);

  let ym = CoarseSync.conj_mult(a,b);

  let asreal_ym = ym.map((e,i)=>e.re);
  let asarg_ym = ym.map((e,i)=>e.arg());

  let expcomplex = mutil.IShortToComplexMulti(CoarseSync.exp_data_1280_ext_15); // exp data

  console.log('expcomplex len ' + expcomplex.length );
  console.log('ym len ' + ym.length );

  let am_inner = CoarseSync.mult(ym,expcomplex.slice(0,stlen));

  let sum = mt.complex(0,0);
  for(let i = 0; i < stlen; i++) {
    // console.log(am_inner[i]);
    sum = mt.add(sum, am_inner[i]);
  }
    console.log('sum ' + sum);
    console.log('sum arg ' + sum.arg());





  // for( x of a ) {
  //   console.log(x.toString(16));
  // }
  // console.log('-----');

  // for( x of b ) {
  //   console.log(x.toString(16));
  // }

  // console.log(a.slice(10+64,16+64));
  // console.log(b.slice(10,16));




  let asreal_b = expcomplex.map((e,i)=>e.re);



  // savePlot = false;




  let plotDelay = 0;

  // if( savePlot ) {
  //     setTimeout(() => {
  //       np.plot(asmag_a, "coarse_input_mag.png");
  //     }, plotDelay);
  // }
  // plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asreal_a, "raw_real.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asreal_ym, "ym_real.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asarg_ym, "asarg_ym.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  // if( savePlot ) {
  //     setTimeout(() => {
  //       np.plot(asreal_b, "coarse_sin_real.png");
  //     }, plotDelay);
  // }
  // plotDelay += 1000;

  done();
});


it.skip('plot values internal to stages', (done) => {

  // let v = [0x00007fff, 0xff5f7fff, 0xfebe7ffe, 0xfe1d7ffc, 0xfd7d7ffa, 0xfcdc7ff6, 0xfc3b7ff2, 0xfb9a7fed, 0xfafa7fe7, 0xfa597fe0, 0xf9b87fd9, 0xf9187fd0, 0xf8777fc7, 0xf7d67fbd, 0xf7367fb3, 0xf6957fa7, 0xf5f57f9b, 0xf5557f8e, 0xf4b47f80, 0xf4147f72, 0xf3747f62, 0xf2d47f52, 0xf2347f41, 0xf1947f2f, 0xf0f57f1d, 0xf0557f0a, 0xefb57ef5, 0xef167ee1, 0xee767ecb, 0xedd77eb5, 0xed387e9d, 0xec997e85, 0xebfa7e6d, 0xeb5b7e53, 0xeabc7e39, 0xea1e7e1e, 0xe9807e02, 0xe8e17de5, 0xe8437dc8, 0xe7a57da9, 0xe7077d8a, 0xe66a7d6b, 0xe5cc7d4a, 0xe52f7d29, 0xe4927d07, 0xe3f47ce4, 0xe3587cc0, 0xe2bb7c9c, 0xe21e7c77, 0xe1827c51, 0xe0e67c2a, 0xe04a7c03, 0xdfae7bda, 0xdf137bb1, 0xde777b88, 0xdddc7b5d, 0xdd417b32, 0xdca77b06, 0xdc0c7ad9, 0xdb727aab, 0xdad87a7d, 0xda3e7a4e, 0xd9a57a1e, 0xd90b79ee, 0xd87279bc, 0xd7d9798a, 0xd7417957, 0xd6a87924, 0xd61078ef, 0xd57878ba, 0xd4e17885, 0xd449784e, 0xd3b27817, 0xd31c77df, 0xd28577a6, 0xd1ef776c, 0xd1597732, 0xd0c376f7, 0xd02e76bb, 0xcf99767f, 0xcf047642, 0xce707604, 0xcddc75c5, 0xcd487586, 0xccb47546, 0xcc217505, 0xcb8e74c3, 0xcafc7481, 0xca69743e, 0xc9d773fa, 0xc94673b6, 0xc8b57371, 0xc824732b, 0xc79372e4, 0xc703729d, 0xc6737255, 0xc5e4720d, 0xc55571c3, 0xc4c67179, 0xc437712e, 0xc3a970e3, 0xc31c7097, 0xc28e704a, 0xc2016ffc, 0xc1756fae, 0xc0e96f5f, 0xc05d6f0f, 0xbfd26ebf, 0xbf476e6e, 0xbebc6e1c, 0xbe326dca, 0xbda86d77, 0xbd1f6d23, 0xbc966ccf, 0xbc0d6c7a, 0xbb856c24, 0xbafe6bce, 0xba766b77, 0xb9ef6b1f, 0xb9696ac7, 0xb8e36a6e, 0xb85e6a14, 0xb7d869ba, 0xb754695f, 0xb6d06903, 0xb64c68a7, 0xb5c9684a, 0xb54667ec, 0xb4c3678e, 0xb442672f, 0xb3c066d0, 0xb33f666f, 0xb2bf660f, 0xb23f65ad, 0xb1bf654b, 0xb14064e9, 0xb0c26485, 0xb0436421, 0xafc663bd, 0xaf496358, 0xaecc62f2, 0xae50628c, 0xadd56225, 0xad5961bd, 0xacdf6155, 0xac6560ec, 0xabeb6083, 0xab726019, 0xaafa5fae, 0xaa825f43, 0xaa0a5ed7, 0xa9935e6b, 0xa91d5dfe, 0xa8a75d91, 0xa8325d23, 0xa7bd5cb4, 0xa7495c45, 0xa6d55bd5, 0xa6625b65, 0xa5f05af4, 0xa57e5a82, 0xa50c5a10, 0xa49b599e, 0xa42b592b, 0xa3bb58b7, 0xa34c5843, 0xa2dd57ce, 0xa26f5759, 0xa20256e3, 0xa195566d, 0xa12955f6, 0xa0bd557e, 0xa0525506, 0x9fe7548e, 0x9f7d5415, 0x9f14539b, 0x9eab5321, 0x9e4352a7, 0x9ddb522b, 0x9d7451b0, 0x9d0e5134, 0x9ca850b7, 0x9c43503a, 0x9bdf4fbd, 0x9b7b4f3e, 0x9b174ec0, 0x9ab54e41, 0x9a534dc1, 0x99f14d41, 0x99914cc1, 0x99304c40, 0x98d14bbe, 0x98724b3d, 0x98144aba, 0x97b64a37, 0x975949b4, 0x96fd4930, 0x96a148ac, 0x96464828, 0x95ec47a2, 0x9592471d, 0x95394697, 0x94e14611, 0x9489458a, 0x94324502, 0x93dc447b, 0x938643f3, 0x9331436a, 0x92dd42e1, 0x92894258, 0x923641ce, 0x91e44144, 0x919240b9, 0x9141402e, 0x90f13fa3, 0x90a13f17, 0x90523e8b, 0x90043dff, 0x8fb63d72, 0x8f693ce4, 0x8f1d3c57, 0x8ed23bc9, 0x8e873b3a, 0x8e3d3aab, 0x8df33a1c, 0x8dab398d, 0x8d6338fd, 0x8d1c386d, 0x8cd537dc, 0x8c8f374b, 0x8c4a36ba, 0x8c063629, 0x8bc23597, 0x8b7f3504, 0x8b3d3472, 0x8afb33df, 0x8aba334c, 0x8a7a32b8, 0x8a3b3224, 0x89fc3190, 0x89be30fc, 0x89813067, 0x89452fd2, 0x89092f3d, 0x88ce2ea7, 0x88942e11, 0x885a2d7b, 0x88212ce4, 0x87e92c4e, 0x87b22bb7, 0x877b2b1f, 0x87462a88, 0x871129f0, 0x86dc2958, 0x86a928bf, 0x86762827, 0x8644278e, 0x861226f5, 0x85e2265b, 0x85b225c2, 0x85832528, 0x8555248e, 0x852723f4, 0x84fa2359, 0x84ce22bf];
  
  // let vv = mutil.IShortToComplexMulti(v);
  // console.log(vv);

  let fake_input_location = mutil.IShortToComplexMulti(vectors1[3]);
  // console.log(fake_input_location.slice(0,8));

  let exp_data_location = mutil.IShortToComplexMulti(vectors1[4]);
  // console.log(exp_data_location.slice(0,8));
  let asmag4 = exp_data_location.map((e,i)=>mt.abs(e));
  let asreal4 = exp_data_location.map((e,i)=>e.re);

  // output
  let output_onetone_calculation_location = mutil.IShortToComplexMulti(vectors1[5]);
  // console.log(output_onetone_calculation_location.slice(0,8));
  let asmag5 = output_onetone_calculation_location.map((e,i)=>mt.abs(e));
  let asreal5 = output_onetone_calculation_location.map((e,i)=>e.re);
  let asarg5 = output_onetone_calculation_location.map((e,i)=>e.arg());

  let bank_address_onetone_calculation_location = mutil.IShortToComplexMulti(vectors1[6]);
  // console.log(bank_address_onetone_calculation_location.slice(0,8));

  let permutation_address_onetone_calculation_location = mutil.IShortToComplexMulti(vectors1[7]);
  // console.log(permutation_address_onetone_calculation_location.slice(0,8));

  let all_one_location = mutil.IShortToComplexMulti(vectors1[8]);
  // console.log(all_one_location.slice(0,8));


  let plotDelay = 0;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asmag5, "onetone_mag.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asreal5, "onetone_real.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asarg5, "onetone_arg.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asmag4, "exp_mag.png");
      }, plotDelay);
  }
  plotDelay += 1000;

  if( savePlot ) {
      setTimeout(() => {
        np.plot(asreal4, "exp_real.png");
      }, plotDelay);
  }
  plotDelay += 1000;



  done();
});




it.skip('randomize sc_16_144_880_1008_bpsk.hex', (done) => {

  let tfm = [];
  let mp = 33;
  let add = 0;
  for( x of raw ) {
    tfm.push(x+(add%mp));
    add++;
  }

    mutil.writeHexFile('conv.hex', tfm);

  let foo = [1, 0xff00, 0xdeadbeef];
  // mutil.writeHexFile('ben.hex', foo);

  // console.log(tfm.slice(0,8));


  done();
});



it.skip('find offsets in sim/verilator/debug_sync/cs00_out.hex', (done) => {

  for( let i = 0; i < 40; i++) {
    let res = mutil.findSequence(raw_mod,lastrun[i]);
    console.log('res' + i + ' ' + res);
    
  }


  done();
});



it.skip('dump results at each stage', function(done) {

  // vectors2 80

  for( let i = 0; i < 32; i+=4 ) {
    mutil.printHexNl(vectors2[0+i].slice(0,8), 'input_conj_multi_location_0 ' + i/4);
    mutil.printHexNl(vectors2[1+i].slice(0,8), 'input_conj_multi_location_1 ' + i/4);
    mutil.printHexNl(vectors2[2+i].slice(0,8), 'output_conj_multi_location ' + i/4);
    mutil.printHexNl(vectors2[3+i].slice(0,8), 'exp_data ' + i/4);
  }

  done();
});





}); // describe
