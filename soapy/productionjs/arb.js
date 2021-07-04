'use strict';
if(!require('semver').satisfies(process.version, '>=11.0.0')) {
  console.log('\n\n    Node version was wrong, please run:   nvm use 11\n\n'); process.exit(1);
}

const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');
let sjs;
const wrapNative = require('./jsinc/wrap_native.js');
let wrapHandle;
const fs = require('fs');
const replify = require('replify');
const remoteTerminal = require('http').createServer();
// const inject = require('./jsinc/inject_offboard.js');
const wrapDbFile = require('./jsinc/wrap_db.js');
// const setupHardwareFile = require('./jsinc/setup_hardware.js');
const handleZmqFile = require('./jsinc/handle_zmq.js');
// const staleInit = require('../init.json');  // only use this before sjs is running

// const settings = wrapHandle.settings;

// console.log('hi');

// const hwOptions = {staleInit};
// const setupHardware = new setupHardwareFile.SetupHardware(hwOptions, hwOk);

let behaviorInc;
let behavior;
let wrapDb;
let role;
let keepAwake;
let zmq;

process.env.NAPI_SMODEM_ROLE = "arb";

sjs = require('./build/Release/sjs.node');


sjs.startArb();
role = sjs.getStringDefault("___undefined", "exp.our.role");
wrapHandle = new wrapNative.WrapNativeSmodem(sjs, role);





zmq = new handleZmqFile.HandleZmq(sjs, role);
// wrapDb = new wrapDbFile.WrapDb(sjs);


if( role === 'rx') {
} else if( role === 'tx') {
} else if( role === 'arb') {
  behaviorInc = require('./jsinc/arb_behavior.js');
  behavior = new behaviorInc.ArbBehavior(sjs, zmq);

} else {
  console.log('exp.our.role is not \'tx\' or \'rx\' or \'arb\', quitting!');
  process.exit(0);
}
keepAwake = setInterval(()=>{},5000);

behavior.print_tx_time_updates = true;

// let r0 = sjs.r[0];
// let r1 = sjs.r[1];


let threadReady = () => {
  console.log("");
  console.log("");

  console.log("thisNodeReady     ready? " + sjs.thisNodeReady());
  console.log("flushHiggsReady   ready? " + sjs.flushHiggsReady());
  console.log("zmqThreadReady    ready? " + sjs.zmqThreadReady());
  console.log("boostThreadReady  ready? " + sjs.boostThreadReady());
  console.log("dspThreadReady    ready? " + sjs.dspThreadReady());
  console.log("txScheduleReady   ready? " + sjs.txScheduleReady());
  console.log("demodThreadReady  ready? " + sjs.demodThreadReady());
  console.log("");

}

// setTimeout(threadReady, 2000);

let agent = behavior.agent;


let context = {
sjs
,behavior
,behaviorInc
,role
,keepAwake
,mutil
,hc
,agent
// ,handleTx
// ,setupHardware
// ,r0
// ,r1
};
zmq.setContext(context);
context.zmq = zmq;
replify('sjs', remoteTerminal, context);

