var jsc = require("jsverify");

const _ = require('lodash');
const FBParse = require("../mock/fbparse.js");
const mutil = require('../mock/mockutils.js');
var assert = require('assert');
const hc = require('../mock/hconst.js');

const p0 = '{"type":"Buffer","data":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const p00 = '{"type":"Buffer","data":[0,0,0,0,0,0,0,0]}';
const p1 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const p2 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const p3 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const p4 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const p5 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';

const p1p2 = '{"type":"Buffer","data":[2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,32,0,0,0,0,0,0,0,0,0,0,128,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}';
const pbad = '{"type":"Buffer","data":[0,0,0,0,1,0,0,16,7]}';



// for * notation see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/for...of
function* getP(){
    // yield p0;
    yield p0;
    yield p1;
    yield p2;
    yield p3;
    yield p4;
    yield p5;
}


function* getPshort(){
    yield p00;
    yield p1;
    yield p2;
    yield p3;
    yield p4;
    yield p5;
}


describe("Feedback bus parser", function () {


// this example shows how to make a test pass or fail
// inside a callback using a promise
it('accepts double frame per packet', (done) => {

  const resolvingPromise = new Promise( (resolve) => {

    let fbparse = new FBParse.Parser();

    fbparse.reset();
    let call_count = 0;

    function cb(header, body) {
      assert.equal(FBParse.g_vtype(header), 13);
      assert.equal(FBParse.g_type(header), 2);
      // console.log("in cb " + header);
      // console.log(typeof header);
      call_count++;
      if( call_count === 2 ) {
        resolve('promise resolved');
      }
    }

    fbparse.regCallback(2, 13, cb);

    let buf = Buffer.from(JSON.parse(p1p2).data);
    fbparse.gotPacket(buf);

  });

  resolvingPromise.then( (result) => {
    done();
  });
});


it('correctly dispatches different packets', (done) => {


  const resolvingPromise = new Promise( (resolve) => {

    // console.log("------------------- entering ");
    let fbparse = new FBParse.Parser();
    fbparse.reset();

    let cb1_count = 0;
    let cb2_count = 0;

    function cb1(header, body) {
      assert.equal(FBParse.g_type(header), 2);
      assert.equal(FBParse.g_vtype(header), 13);
      cb1_count++;
      // console.log("in cb1(" + cb1_count + ') ' + header);
      if( cb1_count === 4 && cb2_count == 1) {
        resolve('promise resolved');
      }
    }

    fbparse.regCallback(2, 13, cb1);


    function cb2(header, body) {
      assert.equal(FBParse.g_type(header), 2);
      assert.equal(FBParse.g_vtype(header), 7);
      cb2_count++;
      // console.log("in cb2(" + cb2_count + ') ' + header);
      if( cb1_count === 4 && cb2_count == 1) {
        resolve('promise resolved');
      }
    }

    fbparse.regCallback(2, 7, cb2);

    for( let p of getPshort() ) {
        // console.log(p);
        let buf = Buffer.from(JSON.parse(p).data);
        fbparse.gotPacket(buf);
    }

  });

  resolvingPromise.then( (result) => {
    done();
  });
});



// reads in a hex file with 3 feedback bus packets
// we reply that the (2,7) is first and then last
// on the userdata packet, we check against the verilated model's output (saved to disk)
// on the last (2,7) packet we check everything and
it('correctly handles mapmov packets', (done) => {

  const resolvingPromise = new Promise( (resolve) => {

    // load in a session from a verilator test bench
    let in0 = mutil.readHexFile('test/data/mapmovfb_in2.hex');

    let asarray = [];

    for( x of in0 ) {
      let r1 = mutil.I32ToBuf(x);
      asarray = asarray.concat([r1[0],r1[1],r1[2],r1[3]]);
    }

    let buf = Buffer.from(asarray);

    // holds the result for the in2 file, see the readme in data folder
    let expected2 = mutil.readHexFile('test/data/mapmovfb_out1.hex');
    expected2 = expected2.slice(16);

    // console.log("------------------- entering ");
    let fbparse = new FBParse.Parser();
    fbparse.reset();

    let cb2_count = 0;
    let ud_count = 0;
    let ud_correct = false;

    function cb2(header, body) {
      cb2_count++;
      // console.log("in cb2(" + cb2_count + ') ' + header);
      if( cb2_count === 2 && ud_count === 1 && ud_correct) {
        return resolve(false); // for some reason, done below wants false for ok
      }

      if( cb2_count === 2 ) {
        if( ud_count !== 1 ) {
          return resolve(new Error('mapmov callback was not called'));
        }

        if( !ud_correct ) {
          return resolve(new Error('mapmoved data was not correct'));
        }
      }
    }

    fbparse.regCallback(2, 7, cb2);

    function cb_userdata(header, body) {
      ud_count++;
      // console.log(header);
      // console.log("in cbuserdata(" + ud_count + ') ' + body);

      ud_correct = _.isEqual(body,expected2);
    }

    fbparse.regUserdataCallback(cb_userdata);



    fbparse.gotPacket(buf);

  });

  resolvingPromise.then( (result) => {

    done(result);

  });
});



}); // describe



describe("parse canned packets", function () {

  jsc.property('advance ok', "bool", function (bb) {

    let fbparse = new FBParse.Parser();
    fbparse.reset();

    for( let p of getP() ) {
        // console.log(p);
        let buf = Buffer.from(JSON.parse(p).data);
        const range = _.range(0, buf.length, 4);
        // using the range, we convert to words and get out as an array
        const aswords = range.map(x => mutil.bufToI32(mutil.sliceBuf4(buf, x)) );
        
        [error, adv] = fbparse.feedback_word_length(aswords);
        // console.log('advance: ' + adv + ' error: ' + error );
    }

    return true;
  });


  jsc.property("should throw with bad length", "bool", function (bb) {

    let fbparse = new FBParse.Parser();
    fbparse.reset();

    let buf = Buffer.from(JSON.parse(pbad).data);
    try {
      fbparse.gotPacket(buf);
      return false;
    }
    catch(err) {
      return true;
    }

    return false; // shouldn't reach here
  });



}); // describe

















describe("Feedback bus vector object", function () {


it('feedback frame getter setter', () => {
  let frame = new FBParse.FeedbackFrame();


  frame.type = 4;

  assert.equal(FBParse.g_type(frame), 4);
  assert.equal(frame.type, 4);

  frame.type = 5

  assert.equal(frame.type, 5);
  assert.equal(FBParse.g_type(frame), 5);

});

it('feedback vector constructor', () => {
  // fine sync
  let frame = new FBParse.FeedbackVector(
    hc.FEEDBACK_PEER_SELF,
    false,
    true,
    hc.FEEDBACK_VEC_FINE_SYNC
    );

  assert.equal(frame.type, hc.FEEDBACK_TYPE_VECTOR);
  assert.equal(frame.vtype, hc.FEEDBACK_VEC_FINE_SYNC);

  frame.setLength(4);

  assert.equal(frame.fblength, 16+4);
  assert.equal(frame.length, 16);

});

it('feedback stream constructor', () => {
  // stream all sc
  let frame = new FBParse.FeedbackStream(
    hc.FEEDBACK_PEER_SELF,
    false,
    true,
    hc.FEEDBACK_STREAM_ALL_SC
    );

  assert.equal(frame.type, hc.FEEDBACK_TYPE_STREAM);
  assert.equal(frame.stype, hc.FEEDBACK_STREAM_ALL_SC);
  assert.equal(frame.vtype, hc.FEEDBACK_STREAM_ALL_SC);

  frame.setLength(4);

  assert.equal(frame.fblength, 16+4);
  assert.equal(frame.length, 16);


});



}); // describe