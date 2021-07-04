var jsc = require("jsverify");

const _ = require('lodash');
const FBParse = require("../mock/fbparse.js");
const mutil = require('../mock/mockutils.js');
var assert = require('assert');
const hc = require('../mock/hconst.js');
const fs = require('fs');

let raw1;
let raw18;
let raw19;

describe("Feedback Bus Crash", function () {

before(()=>{
  raw1 = fs.readFileSync('test/data/crash_mapmov_1.raw');
  raw18 = fs.readFileSync('test/data/crash_mapmov_18.raw');
  raw19 = fs.readFileSync('test/data/crash_mapmov_19.raw');
  status_packet = Buffer.from([2,0,0,0,32,0,0,0,4,0,0,0,0,0,0,128,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]);
});


it('crash2', () => {
  let frames = 50*512;
  let target = 2100 * frames;

  let target2 = target;

  while(target2) {
    let run = Math.min(target2, 0x7fffff);
    console.log("run " + run);
    target2 -= run;
    // target2 -= min()
  }




  console.log(frames);
}); 

// this example shows how to make a test pass or fail
// inside a callback using a promise
it('crash1', (done) => {

  const resolvingPromise = new Promise( (resolve, reject) => {

    let fbparse = new FBParse.Parser({printPackets:true});

    fbparse.reset();
    let call_count = 0;

    let flushCount = 0;
    let statusCount = 0;
    let eqCount = 0;
    let scheduleCount = 0;
    let udCount = 0;

    function cbFlush(header, body) {
      // assert.equal(FBParse.g_vtype(header), 13);
      // assert.equal(FBParse.g_type(header), 2);
      // console.log("in cb " + header);
      console.log('                                             in cb flush ' + flushCount);
      flushCount++;
      // console.log(typeof header);
      // call_count++;
      // if( call_count === 2 ) {
      //   resolve('promise resolved');
      // }
    }

    function cbStatus(header, body) {
      // console.log(mutil.IShortToStreamable(header));
      // console.log(mutil.IShortToStreamable(body));
      console.log('                                             in cb status reply ' + statusCount);
      statusCount++;
    }

    function cbEq(header, body) {
      console.log('                                             in cb eq ' + eqCount);
      eqCount++;
    }

    function cbTxSchedule(header, body) {
      console.log('                                             in cb tx schedule ' + scheduleCount);
      scheduleCount++;
    }

    function ud(header, body) {
      console.log('                                             in user data ' + udCount);

      let s0 = FBParse.g_seq(header);
      let s1 = FBParse.g_seq2(header);

      console.log('s0 ' + s0 + ' s1 ' + s1);

      udCount++;
    }

    // let raw2 = raw1;//Buffer.from(raw1);
    let raw2 = raw1.slice(10240*4);

    raw2 = raw2.slice(32*4*8);


    let tail = 0; // 1040 + 784
    let mycut = (66 + 1040 + 1040 + 66 + 1040 + 1040 + 1040 + 1040 + 1040 + 80 + 1040 + 1040 + 144 + 144 + 1040 + 144 + 144 + 784 + 784 + 784 + 784 + 1040 + 784 + 784 + 784 + tail) * 4;

    // mycut = 0;

    raw2 = raw2.slice(mycut);

    let words3 = mutil.streamableToIShort(raw2);

    // for(let i = 0; i < 2000; i++) {
    //   console.log(words3[i].toString(16));
    // }

    // let raw2 = raw1.slice(32*4*100+100000);


    // console.log(raw1.length);
    console.log('raw2 length: ' + raw2.length);

    // console.log(raw1);

    fbparse.regUserdataCallback(ud);

    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_FLUSH, cbFlush);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_STATUS_REPLY, cbStatus);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_TX_EQ, cbEq);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_SCHEDULE, cbTxSchedule);

    // let buf = Buffer.from(JSON.parse(p1p2).data);

    let allAtOnce = true;

    if( allAtOnce ) {
        fbparse.gotPacket(raw2);
    } else {
      for(let i = 0; i < raw2.length; i+=4) {
        fbparse.gotPacket(raw2.slice(i,i+4));
      }
    }

    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);

    setTimeout(resolve, 200);

  });

  resolvingPromise.then( (result) => {
    done();
  });
});









// this example shows how to make a test pass or fail
// inside a callback using a promise
it('crash18', (done) => {

  const resolvingPromise = new Promise( (resolve, reject) => {

    let fbparse = new FBParse.Parser({printPackets:true});

    fbparse.reset();
    let call_count = 0;

    let flushCount = 0;
    let statusCount = 0;
    let eqCount = 0;
    let scheduleCount = 0;
    let udCount = 0;

    function cbFlush(header, body) {
      // assert.equal(FBParse.g_vtype(header), 13);
      // assert.equal(FBParse.g_type(header), 2);
      // console.log("in cb " + header);
      console.log('                                             in cb flush ' + flushCount);
      flushCount++;
      // console.log(typeof header);
      // call_count++;
      // if( call_count === 2 ) {
      //   resolve('promise resolved');
      // }
    }

    function cbStatus(header, body) {
      // console.log(mutil.IShortToStreamable(header));
      // console.log(mutil.IShortToStreamable(body));
      console.log('                                             in cb status reply ' + statusCount);
      statusCount++;
    }

    function cbEq(header, body) {
      console.log('                                             in cb eq ' + eqCount);
      eqCount++;
    }

    function cbTxSchedule(header, body) {
      console.log('                                             in cb tx schedule ' + scheduleCount);
      scheduleCount++;
    }

    function ud(header, body) {
      console.log('                                             in user data ' + udCount);

      let s0 = FBParse.g_seq(header);
      let s1 = FBParse.g_seq2(header);

      console.log('s0 ' + s0 + ' s1 ' + s1);

      udCount++;
    }

    // let raw2 = raw1;//Buffer.from(raw1);
    // let raw2 = raw1.slice(10240*4);

    // raw2 = raw2.slice(32*4*8);


    // let tail = 0; // 1040 + 784
    // let mycut = (66 + 1040 + 1040 + 66 + 1040 + 1040 + 1040 + 1040 + 1040 + 80 + 1040 + 1040 + 144 + 144 + 1040 + 144 + 144 + 784 + 784 + 784 + 784 + 1040 + 784 + 784 + 784 + tail) * 4;

    // mycut = 0;

    // raw2 = raw2.slice(mycut);

    let words3 = mutil.streamableToIShort(raw18);
    // mutil.writeHexFile('test/data/crash_mapmov_18.hex', words3);


    // for(let i = 0; i < 2000; i++) {
    //   console.log(words3[i].toString(16));
    // }

    // let raw2 = raw1.slice(32*4*100+100000);


    // console.log(raw1.length);
    // console.log('raw2 length: ' + raw18.length);
    // console.log('words3 length: ' + words3.length);

    // console.log(raw1);

    fbparse.regUserdataCallback(ud);

    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_FLUSH, cbFlush);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_STATUS_REPLY, cbStatus);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_TX_EQ, cbEq);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_SCHEDULE, cbTxSchedule);

    // let buf = Buffer.from(JSON.parse(p1p2).data);

    let allAtOnce = true;

    if( allAtOnce ) {
        fbparse.gotPacket(raw18);
    } else {
      for(let i = 0; i < raw18.length; i+=4) {
        fbparse.gotPacket(raw18.slice(i,i+4));
      }
    }

    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);

    setTimeout(resolve, 200);

  });

  resolvingPromise.then( (result) => {
    done();
  });
});







// this example shows how to make a test pass or fail
// inside a callback using a promise
it('crash19', (done) => {

  const resolvingPromise = new Promise( (resolve, reject) => {

    let fbparse = new FBParse.Parser({printPackets:true});

    fbparse.reset();
    let call_count = 0;

    let flushCount = 0;
    let statusCount = 0;
    let eqCount = 0;
    let scheduleCount = 0;
    let udCount = 0;

    function cbFlush(header, body) {
      // assert.equal(FBParse.g_vtype(header), 13);
      // assert.equal(FBParse.g_type(header), 2);
      // console.log("in cb " + header);
      console.log('                                             in cb flush ' + flushCount);
      flushCount++;
      // console.log(typeof header);
      // call_count++;
      // if( call_count === 2 ) {
      //   resolve('promise resolved');
      // }
    }

    function cbStatus(header, body) {
      // console.log(mutil.IShortToStreamable(header));
      // console.log(mutil.IShortToStreamable(body));
      console.log('                                             in cb status reply ' + statusCount);
      statusCount++;
    }

    function cbEq(header, body) {
      console.log('                                             in cb eq ' + eqCount);
      eqCount++;
    }

    function cbTxSchedule(header, body) {
      console.log('                                             in cb tx schedule ' + scheduleCount);
      scheduleCount++;
    }

    function ud(header, body) {
      console.log('                                             in user data ' + udCount);

      let s0 = FBParse.g_seq(header);
      let s1 = FBParse.g_seq2(header);

      console.log('s0 ' + s0 + ' s1 ' + s1);

      udCount++;
    }

    // let raw2 = raw1;//Buffer.from(raw1);
    // let raw2 = raw1.slice(10240*4);

    // raw2 = raw2.slice(32*4*8);


    // let tail = 0; // 1040 + 784
    // let mycut = (66 + 1040 + 1040 + 66 + 1040 + 1040 + 1040 + 1040 + 1040 + 80 + 1040 + 1040 + 144 + 144 + 1040 + 144 + 144 + 784 + 784 + 784 + 784 + 1040 + 784 + 784 + 784 + tail) * 4;

    // mycut = 0;

    // raw2 = raw2.slice(mycut);

    let words3 = mutil.streamableToIShort(raw19);
    // mutil.writeHexFile('test/data/crash_mapmov_19.hex', words3);


    // console.log(raw1.length);
    // console.log('raw2 length: ' + raw18.length);
    // console.log('words3 length: ' + words3.length);

    // console.log(raw1);

    fbparse.regUserdataCallback(ud);

    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_FLUSH, cbFlush);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_STATUS_REPLY, cbStatus);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_TX_EQ, cbEq);
    fbparse.regCallback(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_SCHEDULE, cbTxSchedule);

    // let buf = Buffer.from(JSON.parse(p1p2).data);

    let allAtOnce = true;

    if( allAtOnce ) {
        fbparse.gotPacket(raw18);
    } else {
      for(let i = 0; i < raw18.length; i+=4) {
        fbparse.gotPacket(raw18.slice(i,i+4));
      }
    }

    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);

    // fbparse.gotPacket(status_packet);
    // fbparse.gotPacket(status_packet);

    setTimeout(resolve, 200);

  });

  resolvingPromise.then( (result) => {
    done();
  });
});










}); // describe

