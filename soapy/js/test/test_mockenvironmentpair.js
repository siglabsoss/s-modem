var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mapmov = require('../mock/mapmov.js');
const menvPair = require('../mock/mockenvironmentpair.js');
const mt = require('mathjs');
const Stream = require("stream");
const streamers = require('../mock/streamers.js');

var assert = require('assert');


let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}


describe("mock environment pair", function () {

it('mock environment pair1', function(done) {
  let t = jsc.forall('array number', 'array number', function (input, input2) {
    menv = new menvPair.MockEnvironmentPair();

    // console.log(input.length);
    // console.log(input);
    // console.log(input2);

    // let feed = new MockReadable({highWaterMark:64}, 0.1);
    let r0_tx = new streamers.StreamArrayType({},{type:'Float64', chunk:16});
    let r0_rx = new streamers.F64CaptureStream({});

    r0_tx.load(input);

    menv.attachRadio(r0_tx, r0_rx);

    let r1_tx = new streamers.StreamArrayType({},{type:'Float64', chunk:16});
    let r1_rx = new streamers.F64CaptureStream({});

    r1_tx.load(input2);

    menv.attachRadio(r1_tx, r1_rx);
    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) { try {
        setTimeout(()=>{
          // console.log(r0_rx.capture);
          assert.deepEqual(r0_rx.capture, input2);
          assert.deepEqual(r1_rx.capture, input);
          resolve(true);
        },100);
      }catch(e){reject(e)}
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10E19, tests: 4};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



}); // describe
