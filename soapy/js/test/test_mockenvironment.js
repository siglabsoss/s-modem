var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mapmov = require('../mock/mapmov.js');
const menv = require('../mock/mockenvironment.js');
const mt = require('mathjs');
const Stream = require("stream");
const streamers = require('../mock/streamers.js');

var assert = require('assert');


let np;
let savePlot = false;

if( savePlot ) {
   np = require('../mock/nplot.js');
}





describe("mock environment", function () {

it('accepts stream', (done) => {

  menv.reset();

  // let feed = new MockReadable({highWaterMark:64}, 0.1);
  let r0_tx = new streamers.CF64StreamSin({}, {theta:0.1,count:3,chunk:16} );
  let r0_rx = new streamers.F64CaptureStream({});

  menv.attachRadio(r0_tx, r0_rx);

  let r1_tx = new streamers.CF64StreamSin({}, {theta:0.33,count:3,chunk:16} );
  let r1_rx = new streamers.F64CaptureStream({});

  menv.attachRadio(r1_tx, r1_rx);

  // let got_out0 = mapmov.accept(in0);
  
  // assert(_.isEqual(got_out0,ideal_out0));

  setTimeout(() => {
    console.log("r0 got " + r0_rx.capture.length);


    let ascomplex0 = _.chunk(r0_rx.capture,2).map((e,i)=>mt.complex(e[0],e[1]));
    let asmag0 = ascomplex0.map((e,i)=>mt.abs(e));
    let asreal0 = ascomplex0.map((e,i)=>e.re);

    if( savePlot ) {
        setTimeout(() => {
          np.plot(asreal0, "test_env_r0_out.png");
        }, 0);
    }

    let ascomplex1 = _.chunk(r1_rx.capture,2).map((e,i)=>mt.complex(e[0],e[1]));
    let asmag1 = ascomplex1.map((e,i)=>mt.abs(e));
    let asreal1 = ascomplex1.map((e,i)=>e.re);

    if( savePlot ){
        setTimeout(() => {
          np.plot(asreal1, "test_env_r1_out.png");
        }, 3000);
    }

    done();
  }, 200);

  // done();
});



it('accepts gain stream', (done) => {

  menv.reset();

  // let feed = new streamers.CF64StreamSin({highWaterMark:64}, 0.1);
  let r0_tx = new streamers.CF64StreamSin({}, {theta:0.1,count:3,chunk:16} );
  let r0_rx = new streamers.F64CaptureStream({});

  let r0_gain = new streamers.F64GainStream({}, {gain:2.0});

  r0_tx.pipe(r0_gain);

  menv.attachRadio(r0_gain, r0_rx);

  let r1_tx = new streamers.CF64StreamSin({}, {theta:0.33,count:3,chunk:16} );
  let r1_rx = new streamers.F64CaptureStream({});

  menv.attachRadio(r1_tx, r1_rx);

  // let got_out0 = mapmov.accept(in0);
  
  // assert(_.isEqual(got_out0,ideal_out0));

  setTimeout(() => {
    console.log("r0 got " + r0_rx.capture.length);


    let ascomplex0 = _.chunk(r0_rx.capture,2).map((e,i)=>mt.complex(e[0],e[1]));
    let asmag0 = ascomplex0.map((e,i)=>mt.abs(e));
    let asreal0 = ascomplex0.map((e,i)=>e.re);


    if( savePlot ) {
        setTimeout(() => {
          np.plot(asreal0, "test_env_gain_r0_out.png");
        }, 0);
    }

    let ascomplex1 = _.chunk(r1_rx.capture,2).map((e,i)=>mt.complex(e[0],e[1]));
    let asmag1 = ascomplex1.map((e,i)=>mt.abs(e));
    let asreal1 = ascomplex1.map((e,i)=>e.re);


    if( savePlot ){
        setTimeout(() => {
          np.plot(asreal1, "test_env_gain_r1_out.png");
        }, 3000);
    }

    done();
  }, 200);

  // done();
});



});
