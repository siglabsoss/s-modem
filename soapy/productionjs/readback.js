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
const g3ctrl = require('./jsinc/grav3_ctrl.js');

// const settings = wrapHandle.settings;

// console.log('hi');

// const hwOptions = {staleInit};
// const setupHardware = new setupHardwareFile.SetupHardware(hwOptions, hwOk);

let behaviorInc;
let behavior;
let readback;
let eqReadback;
let wrapDb;
let role;
let keepAwake;
let zmq;

function hwOk() {

process.env.NAPI_SMODEM_ROLE = staleInit.exp.our.role;
sjs = require('./build/Release/sjs.node');
wrapHandle = new wrapNative.WrapNativeSmodem(sjs, staleInit.exp.our.role);
handleTx = new HandleFeedbackBusRecoveryFile.HandleFeedbackBusRecovery(sjs);

keepAwake = setInterval(()=>{},5000);

sjs.startSmodemFirst();

// only settings object and main higgs object exists here
// no eventDsp or fsm are alive
// sjs.higgsRunTunThread = false;
// disable print when rx side is not bootloaded
// won't work because we use dispatch for settings, not a direction funciton
// this should be changed

sjs.startSmodemSecond();

sjs.settings.setBool("exp.skip_check_fb_datarate", true);
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

let r0 = sjs.r[0];
let r1 = sjs.r[1];




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


  let handled3 = false;
  handled3 = behavior.handleRingbus(word);
  if( handled3 ) {
    print = false;
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
sjs.ringbusCallbackCatchType(hc.POWER_RESULT_PCCMD);
sjs.ringbusCallbackCatchType(hc.UART_READOUT_PCCMD);
// sjs.ringbusCallbackCatchType(0x10000000);

// sjs.ringbusCallbackCatchType(hc.MAPMOV_MODE_REPORT);

// sjs.ringbusCallbackCatchType(0x48000000);


handleTx.register();  // calls more ringbusCallbackCatchType

sjs.registerRingbusCallback(cb);


sjs.dsp.dsp_state_pending = hc.DEBUG_STALL;
setTimeout(()=>{sjs.dsp.dsp_state_pending = hc.DEBUG_STALL;}, 100);

/*


sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.safe_state('a') );

sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.tx_state('a') );
sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.rx_state('b') );

sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.dac_current(5) );

sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.rx_state('b', 5) );

sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.channel_b({state:'rx',dsa:0}) );



*/





let context = {
sjs
,behavior
,behaviorInc
,role
,keepAwake
,mutil
,hc
,handleTx
,r0
,r1
,g3ctrl
};
zmq.setContext(context);
context.zmq = zmq;
replify('sjs', remoteTerminal, context);


} // hwOk()

hwOk();