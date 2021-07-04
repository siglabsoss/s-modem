'use strict';
if(!require('semver').satisfies(process.version, '>=11.0.0')) {
  console.log('\n\n    Node version was wrong, please run:   nvm use 11\n\n'); process.exit(1);
}

const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');
let sjs;
const wrapNative = require('./jsinc/wrap_native.js');
let wrapHandle;
const HandleFeedbackBusRecoveryFile = require('./jsinc/handle_feedback_bus_recovery.js');
let handleTx;
const fs = require('fs');
const replify = require('replify');
const remoteTerminal = require('http').createServer();
const inject = require('./jsinc/inject_offboard.js');
const wrapDbFile = require('./jsinc/wrap_db.js');
const setupHardwareFile = require('./jsinc/setup_hardware.js');
const handleZmqFile = require('./jsinc/handle_zmq.js');
const staleInit = require('../init.json');  // only use this before sjs is running
const fbParseFile = require('../js/mock/fbparse.js');

// const settings = wrapHandle.settings;

// console.log('hi');

const hwOptions = {staleInit};
const setupHardware = new setupHardwareFile.SetupHardware(hwOptions, hwOk);

let behaviorInc;
let behavior;
let readback;
let eqReadback;
let wrapDb;
let role;
let keepAwake;
let zmq;
let our_id;
let arb;
let arbBehaviorInc;

function hwOk() {

process.env.NAPI_SMODEM_ROLE = staleInit.exp.our.role;
sjs = require('./build/Release/sjs.node');
wrapHandle = new wrapNative.WrapNativeSmodem(sjs, staleInit.exp.our.role);
handleTx = new HandleFeedbackBusRecoveryFile.HandleFeedbackBusRecovery(sjs);

keepAwake = setInterval(()=>{},5000);

sjs.startSmodem();
role = sjs.getStringDefault("___undefined", "exp.our.role");

zmq = new handleZmqFile.HandleZmq(sjs, role);

wrapDb = new wrapDbFile.WrapDb(sjs);


if( role === 'rx') {
  console.log('Starting as RX');
  behaviorInc = require('./jsinc/rx_behavior.js');
  behavior = new behaviorInc.RxBehavior(sjs);
} else if( role === 'tx') {
  console.log('Starting as TX');
  behaviorInc = require('./jsinc/tx_behavior.js');
  behavior = new behaviorInc.TxBehavior(sjs);

  readback = require('./jsinc/eq_readback.js');
  eqReadback = new readback.EqReadback(sjs);
} else {
  console.log('exp.our.role is not \'tx\' or \'rx\', quitting!');
  process.exit(0);
}


sjs.settings.setBool('global.print.PRINT_OUTGOING_ZMQ', false);
// GET_PRINT_OUTGOING_ZMQ



let r0 = sjs.r[0];
let r1 = sjs.r[1];

our_id = sjs.settings.getIntDefault(-1,"exp.our.id");

arbBehaviorInc = require('./jsinc/arb_behavior2.js'); // fixme merge these once ben/qam is merged into master
arb = new arbBehaviorInc.ArbBehavior(sjs, zmq, {attachedMode:true, startDelay:10000});




let cb2 = (word) => {
  let print = true;
  let h1 = handleTx.rbcb(word);
  if(h1) {
    print = false;
  }

  if( role == 'tx' ) {
    let h2 = false;
    h2 = eqReadback.rbcb(word);
    if( h2 ) {
      print = false;
    }
  }

  if( print ) {
    console.log('in js cb->' + word.toString(16));
  }
}


let cb = (x) => {
  cb2(x);
}

sjs.ringbusCallbackCatchType(0x00000000);
sjs.ringbusCallbackCatchType(0xde000000);
sjs.ringbusCallbackCatchType(0xff000000);
sjs.ringbusCallbackCatchType(0x22000000);
sjs.ringbusCallbackCatchType(0x20000000);
sjs.ringbusCallbackCatchType(hc.TX_PARSE_GOT_VECTOR_PCCMD);
sjs.ringbusCallbackCatchType(hc.FEEDBACK_HASH_PCCMD);
// sjs.ringbusCallbackCatchType(0x10000000);

// sjs.ringbusCallbackCatchType(hc.MAPMOV_MODE_REPORT);

// sjs.ringbusCallbackCatchType(0x48000000);


handleTx.register();  // calls more ringbusCallbackCatchType

sjs.registerRingbusCallback(cb);



let doTickle = () => {
  sjs.txSchedule.tickleAttachedProgress();
}

let inv1;
setTimeout(()=>{inv1=setInterval(doTickle, 500);},2000);



// setTimeout(()=>{
//   sjs.ringbus(hc.RING_ADDR_TX_0, hc.RING_TEST_CMD | (0x1) );
//   sjs.ringbus(hc.RING_ADDR_TX_0, hc.RING_TEST_CMD | (0x2) );
//   sjs.ringbus(hc.RING_ADDR_TX_0, hc.RING_TEST_CMD | (0x3) );
// },3000);




var ca = [
0x00000002,
16+16,
0x00000004,
0x80000000,
hc.FEEDBACK_VEC_STATUS_REPLY,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000
];

for(let i = 0; i < 16; i++) {
  ca.push(0);
}

var vb = [
 2
,1024+16
,4
,0
,hc.FEEDBACK_VEC_TX_EQ
];
while(vb.length < 16) {
  vb.push(0);
}
for(let i = 0; i < 1024; i++) {
  vb.push(0xf000 + i);
}



setTimeout(()=>{
  sjs.dsp.sendCustomEventToPartner(our_id, hc.REQUEST_MAPMOV_DEBUG, 1);
  clearInterval(inv1);
},4000);



var eq0 = [];
for(let i = 0; i < 1024; i++) {
  eq0.push(0xf000 + i);
}


/*

node_modules/.bin/rc /tmp/repl/sjs.sock


Run this to crash it:



sjs.dsp.setPartnerEq(our_id, eq0)



*/

// console.log(vb);


// fbParseFile.feedback_peer_to_mask(17);


// sjs.dsp.sendCustomEventToPartner(our_id, hc.REQUEST_MAPMOV_DEBUG, 1);


// let nextTime = 2000;

// let timeCrunch = () => {
//   sjs.sendToHiggs(vb);

//   nextTime -= 100;

//   if( nextTime < 0 ) {
//     return;
//   }

//   setTimeout(timeCrunch, nextTime);
// }

// setTimeout(timeCrunch, 5000);


/*

sjs.sendToHiggs(ca)
sjs.sendToHiggs(vb)

*/


if( role === "tx" ) {
  // behavior.sendSize(18*1000*4);
  // behavior.sendSize(30*1000*4);
  behavior.sendSize((348160-2000)*4);
  // behavior.setDrainRate(3.9);

  // sjs.debugTxFill()
}



if( role === "tx" ) {
  setTimeout(()=> {
    handleTx.check();
  }, 2000);

  setTimeout(()=> {
    handleTx.printCapture();
  }, 2600);
}



let setBoth = (a,b) => {
  r0.setPartnerTDMA(a, b);
  r1.setPartnerTDMA(a, b);
}




let context = {
sjs
,behavior
,behaviorInc
,role
,keepAwake
,mutil
,hc
,handleTx
,setupHardware
,r0
,r1
,setBoth
,ca
,vb
,fbParseFile
,our_id
,arbBehaviorInc
,arb
,eq0
};
zmq.setContext(context);
context.zmq = zmq;
replify('sjs', remoteTerminal, context);


} // hwOk()
