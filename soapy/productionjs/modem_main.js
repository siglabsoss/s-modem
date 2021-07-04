'use strict';
if(!require('semver').satisfies(process.version, '>=11.0.0')) {
  console.log('\n\n    Node version was wrong, please run:   nvm use 11\n\n'); process.exit(1);
}

const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');
const wrapNative = require('./jsinc/wrap_native.js');
const HandleFeedbackBusRecoveryFile = require('./jsinc/handle_feedback_bus_recovery.js');
const fs = require('fs');
const replify = require('replify');
const remoteTerminal = require('http').createServer();
const inject = require('./jsinc/inject_offboard.js');
const wrapDbFile = require('./jsinc/wrap_db.js');
const setupHardwareFile = require('./jsinc/setup_hardware.js');
const handleZmqFile = require('./jsinc/handle_zmq.js');
const staleInit = require('../init.json');  // only use this before sjs is running
const g3ctrl = require('./jsinc/grav3_ctrl.js');
const HandlePowerFile = require('./jsinc/handle_power.js');
const DispatchRingbusFile = require('./jsinc/dispatch_ringbus.js');
const HandleSelfSyncFile = require('./jsinc/handle_self_sync.js');
const HandleMasksFile = require('./jsinc/handle_masks.js');
const HandleOOKFile = require('./jsinc/handle_ook.js');
const TortureFeedbackBusFile = require('./jsinc/torture_feedback_bus.js');
const HandleDuplexFile = require('./jsinc/handle_duplex.js');

let isDebugMode = typeof process.env.SMODEM_START_DEBUG !== 'undefined';

// const settings = wrapHandle.settings;

// console.log('hi');

const hwOptions = {staleInit};
const setupHardware = new setupHardwareFile.SetupHardware(hwOptions, hwOk, isDebugMode);

let sjs;
let wrapHandle;
let handleTx;
let behaviorInc;
let behavior;
let readback;
let eqReadback;
let wrapDb;
let role;
let keepAwake;
let zmq;
let pow;
let hrb;
let handleSelfSync;
let masks;
let handleOOK;
let handleTortureFb;
let handleDuplex;

function hwOk() {

process.env.NAPI_SMODEM_ROLE = staleInit.exp.our.role;
sjs = require('./build/Release/sjs.node');
sjs.isDebugMode = isDebugMode;
wrapHandle = new wrapNative.WrapNativeSmodem(sjs, staleInit.exp.our.role);
handleTx = new HandleFeedbackBusRecoveryFile.HandleFeedbackBusRecovery(sjs);

keepAwake = setInterval(()=>{},5000);

sjs.startSmodemFirst();
if( isDebugMode ) {
  sjs.enterDebugMode();
  console.log('write settings');
}
sjs.startSmodemSecond();

role = sjs.getStringDefault("___undefined", "exp.our.role");
sjs.role = role;

zmq = new handleZmqFile.HandleZmq(sjs, role);
sjs.zmq = zmq;

wrapDb = new wrapDbFile.WrapDb(sjs);

pow = new HandlePowerFile.HandlePower(sjs, role);
sjs.pow = pow;

hrb = new DispatchRingbusFile.DispatchRingbus(sjs);
sjs.hrb = hrb;

masks = new HandleMasksFile.HandleMasks(sjs);

handleOOK = new HandleOOKFile.HandleOOK(sjs);
sjs.handleOOK = handleOOK;

handleTortureFb = new TortureFeedbackBusFile.TortureFeedbackBus(sjs);
sjs.handleTortureFb = handleTortureFb;

handleDuplex = new HandleDuplexFile.HandleDuplex(sjs);
sjs.handleDuplex = handleDuplex;




let cb2 = (word) => {
  try {
    let print = true;
    const type = word & 0xff000000;
    let h1 = handleTx.rbcb(word);
    if(h1) {
      print = false;
    }

    if( role === 'tx' && eqReadback ) {
      let h2 = false;
      h2 = eqReadback.rbcb(word);
      if( h2 ) {
        print = false;
      }
    }

    let handled3 = false;
    if( behavior ) {
      handled3 = behavior.handleRingbus(word);
      if( handled3 ) {
        print = false;
      }
    }
    
    hrb.notify.emit(''+type, word);

    if( hrb.notify.listenerCount(''+type) ) {
      print = false;
    }

    if( print ) {
      console.log('in js cb->' + word.toString(16));
    }
  } catch (err) {
    console.log(err);
  }

}


let cb = (x) => {
  cb2(x);
}

sjs.ringbusCallbackCatchType(hc.GENERIC_PCCMD);
sjs.ringbusCallbackCatchType(hc.DEBUG_0_PCCMD);
sjs.ringbusCallbackCatchType(hc.DEBUG_14_PCCMD);
sjs.ringbusCallbackCatchType(hc.RX_CS21_DID_ADJUST);
sjs.ringbusCallbackCatchType(hc.POWER_RESULT_PCCMD);
sjs.ringbusCallbackCatchType(hc.EXFIL_READOUT_PCCMD);

sjs.registerRingbusCallback(cb);


handleSelfSync = new HandleSelfSyncFile.HandleSelfSync(sjs);

handleSelfSync.onDone(()=>{
  selfSyncOk();
});

setTimeout(handleSelfSync.run.bind(handleSelfSync), 100);
} // hwOk()


function selfSyncOk() {

// release both tx and rx fsms
sjs.settings.setBool('runtime.self_sync.done', true);

if( role === 'rx') {
  console.log('Starting as RX');
  behaviorInc = require('./jsinc/rx_behavior.js');
  behavior = new behaviorInc.RxBehavior(sjs);
  behavior.powerCb = pow.gotPower.bind(pow);
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

sjs.test_ring = behavior.test_ring.bind(behavior);
let test_ring = ()=>{
  return sjs.test_ring();
}

sjs.behavior = behavior;


let r0 = sjs.r[0];
let r1 = sjs.r[1];
let r2 = sjs.r[2];
let r3 = sjs.r[3];


// sjs.ringbusCallbackCatchType(0x10000000);

// sjs.ringbusCallbackCatchType(hc.MAPMOV_MODE_REPORT);

// sjs.ringbusCallbackCatchType(0x48000000);


handleTx.register();  // calls more ringbusCallbackCatchType



let watchPower = () => {
  sjs.ringbus(hc.RING_ADDR_CS31, hc.POWER_ESTIMATION_CMD | 0 );
};

// let watchPowerIntv = setInterval(watchPower, 2000);

let dsaUpdated = false;

let watchMode = () => {
  // sjs.ringbus(hc.RING_ADDR_CS31, hc.POWER_ESTIMATION_CMD | 0 );
  if(sjs.dsp.dsp_state_pending == hc.DEBUG_STALL) {
    if(!dsaUpdated) {
      dsaUpdated = true;
      sjs.dsa(10);
      console.log("updating DSA now that we are in debug stall");
    }
  }
};

// let watchModeIntv = setInterval(watchMode, 1000);




// sjs.dispatchMockRpc("hello worldd");

// console.log(sjs.settings);
// ['runtime','r0', 'skip_tdma']
// sjs.settings.setBool('runtime.r0.skip_tdma', false);
// sjs.settings.setBool('dsp.block_fsm', true);


// sjs.settings.setDouble('runtime.some_double', 234.3);

// setTimeout(()=>{
//   sjs.settings.setDouble('runtime.some_double', 10.1);
// }, 1000);

// setTimeout(()=>{
//   sjs.settings.setDouble('runtime.some_double', "10.2");
// }, 3000);

// setTimeout(()=>{
//   sjs.settings.setDouble('runtime.some_double', "asd 20");
// }, 6000);

// wrapHandle.setBool('runtime.foo', true);




// setTimeout(()=>{
//   // console.log("Doing Coarse sync from js");

//   // sjs.ringbus(hc.RING_ADDR_CS00, hc.SYNCHRONIZATION_CMD | (0x1) );

// }, 5000);

// directly to feedbac bus

 // sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_REPORT_ERRORS_CMD );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.FB_REPORT_STATUS_CMD );


// sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_MAIN_REPORT_STATUS_CMD );

// sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_PROGRESS_CMD | (10*24) );


// sjs.ringbus(hc.RING_ADDR_CS22, hc.TRIGGER_EXFIL_CMD );


// behavior.sendSize(20000*4*7);
// behavior.sendSize(20000*4);

// behavior.sendSize(18000*4);
// behavior.sendSize(16000*4);
// behavior.sendSize(15000*4);
// behavior.sendSize(14000*4);
// behavior.sendSize(13000*4);
// behavior.sendSize(12000*4);

if( role === "tx" ) {
  // behavior.sendSize(18*1000*4);
  // behavior.sendSize(30*1000*4);
  behavior.sendSize((348160-2000)*4);
  // behavior.setDrainRate(3.9);

  // sjs.debugTxFill()
}



// clearInterval(behavior.grabTimer);
// behavior.grabTimer = setInterval(()=>{behavior.stuffData()}, 500);

if( role === "tx" ) {
  setTimeout(()=> {
    handleTx.check();
  }, 2000);

  setTimeout(()=> {
    handleTx.printCapture();
  }, 2600);
}




// sjs.settings.setBool('global.print.PRINT_TX_BUFFER_FILL', false);

// sjs.settings.getBoolDefault(true, "global.print.PRINT_TX_BUFFER_FILL");

// sjs.sendZerosToHiggs(16*2048*1024)
// sjs.sendZerosToHiggs(2048*1024)
// sjs.sendZerosToHiggs(512*1024)
// sjs.sendZerosToHiggs(64*1024)
// sjs.sendZerosToHiggs(16)


// sjs.settings.setBool('exp.debug_tx_fill_level', true)
// sjs.settings.setBool('runtime.tx.trigger_warmup', true)

// sjs.settings.setBool('exp.retry.coarse.enable', false)


//  REQUEST_MAPMOV_REPORT
// sjs.ringbus(hc.RING_ADDR_ETH, hc.REQUEST_MAPMOV_REPORT );
// sjs.ringbus(hc.RING_ADDR_ETH, hc.ETH_GET_STATUS_CMD ); // ETH_GET_STATUS_CMD    (0x2C000000)

// sjs.ringbus(hc.RING_ADDR_ETH, hc.MAPMOV_RESET_CMD );



// sjs.ringbus(hc.RING_ADDR_CS11, hc.CS11_SET_GAIN_CMD | 22000 );
// sjs.ringbus(hc.RING_ADDR_CS11, hc.CS11_SET_GAIN_CMD | 18000 );
// sjs.ringbus(hc.RING_ADDR_CS11, hc.CS11_SET_GAIN_CMD | 8640 );  // default
// sjs.ringbus(hc.RING_ADDR_CS11, hc.CS11_SET_GAIN_CMD | 0 );


// works to drain fill
// sjs.ringbus(hc.RING_ADDR_CS20, hc.FB_BUS_RESET_CMD | 1 );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.FB_BUS_RESET_CMD | 2 );

// corrupt dma and rely on run till last to solve
// sjs.ringbus(hc.RING_ADDR_CS00, hc.CORRUPT_DMA_OUT_CMD | 33 );
// sjs.ringbus(hc.RING_ADDR_CS01, hc.CORRUPT_DMA_OUT_CMD | 33 );
// sjs.ringbus(hc.RING_ADDR_CS11, hc.CORRUPT_DMA_OUT_CMD | 33 );

// FFT stages
// sjs.ringbus(hc.RING_ADDR_CS10, 0x57000000 | 0x30000 | 0x0f );
// sjs.ringbus(hc.RING_ADDR_CS10, 0x57000000 | 0x40000 | 0x0f );


// sjs.ringbus(hc.RING_ADDR_CS21, 0x58000000 | 14 );

// cs10 report underflow status
// sjs.ringbus(hc.RING_ADDR_CS10, 0x61000000 );

/*

// disable rx mag adjust
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAGADJUST_CMD | 0 );

sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAGADJUST_CMD | 1 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAGADJUST_CMD | 2 );

sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 0 ); // disable filter
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 10000 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 6000 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 5000 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 3000 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 1000 );
sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.RX_PHASE_CORRECTION_CMD | 0 );

*/


// sjs.settings.setDouble("dsp.eq.magnitude_divisor", 10.0);

// sjs.ringbus(hc.RING_ADDR_ETH, hc.DAC_CFG_LOCK_CMD | 0 );



// sjs.setFlushHiggsPrint(true)
// sjs.setFlushHiggsPrint(false)

// flush any pending inputs?


// sjs.setFlushHiggsPrintSending(true)

// let a = sjs.flushHiggs.fullAge();
/*

sjs.dsp.setPartnerSfoAdvance(2, 1, 4)
sjs.dsp.setPartnerSfoAdvance(2, 1280-1, 4)

sjs.dsp.setPartnerSfoAdvance(1, 50, 4)
sjs.dsp.setPartnerSfoAdvance(1, 100, 4)
sjs.dsp.setPartnerSfoAdvance(1, 200, 4)
sjs.dsp.setPartnerSfoAdvance(1, 500, 4)
sjs.dsp.setPartnerSfoAdvance(1, 1280-100, 4)

*/

/*

let a = sjs.flushHiggs.getDataHistory();
console.log(a);

let b = mutil.streamableToIShort(a);

for (let x of b) {
  console.log(x.toString(16));
}


*/


// 
//   sjs.settings.setInt("dsp.coarse.coarse_sync_ofdm_num", 25600);
//   sjs.settings.setBool("exp.retry.coarse.enable", false);


// process.exit(0);

///////////////////
//
// Schedule stuff
//
/*

sjs.schedule.setEarlyFrames(3000)
sjs.schedule.setTimeslots([8, 33])

let ts = []; for(let i = 8; i < 50; i++) {ts.push(i)}; sjs.schedule.setTimeslots(ts);

sjs.schedule.setPacketSize(512*17)



sjs.schedule.setTimeslots([8])
sjs.schedule.setPacketSize(512*17*2)


sjs.schedule.setPacketSize(512*25)
sjs.schedule.setPacketSize(512*50)
sjs.schedule.setPacketSize(512*100)
sjs.schedule.setPacketSize(512*150)




sjs.ringbus(hc.RING_ADDR_CS31, hc.POWER_ESTIMATION_CMD | 0 );


 

*/

/*
Residue:

sjs.r[0].pause_residue = true
sjs.r[0].enable_residue_to_cfo = true
sjs.r[0].residue_to_cfo_factor = 0.3
sjs.r[0].residue_to_cfo_delay = 1000*1000

sjs.r[0].residue_phase_trend


sjs.r[0].cfo_estimated = 0.01; sjs.r[0].updatePartnerCfo()



sjs.r[0].cfo_estimated_sent / 29;  sjs.r[0].updatePartnerSfo()

sjs.r[0].sfo_estimated = 0.1;  sjs.r[0].updatePartnerSfo()


sjs.r[0].sfo_estimated = 5;  sjs.r[0].updatePartnerSfo()



sjs.dsp.setPartnerSfoAdvance(2, 1280-10, 4)


sjs.dsp.sendCustomEventToPartner(2, hc.EXIT_DATA_MODE_EV, 0);

sjs.dsp.sendCustomEventToPartner(1, hc.REQUEST_MAPMOV_DEBUG, 1);
sjs.dsp.sendCustomEventToPartner(1, hc.EXIT_DATA_MODE_EV, 0);

sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_TDMA_SC_CMD | 17 );

sjs.dsp.setPartnerTDMA(sjs.r[0].peer_id, 6, 0)


r0.dspRunStoEq(1.0);

r0.updatePartnerEq(false, false);



a = [r0.getChannelAngle(),
r0.getChannelAngleSent(),
r0.getChannelAngleSentTemp(),
r0.getEqualizerCoeffSent()];


sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_MASK_SC_CMD | 0x00000 | 17 );
sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_MASK_SC_CMD | 0x10000 | 19 );


sjs.settings.getIntDefault(99,"exp.our.id")


sjs.customEvent.send(10, 0);

r0.setPartnerTDMA(11, 64);

var opBoth = (a,b,c) => {
  r0.partnerOp(a, b, c);
  r1.partnerOp(a, b, c);
}

// 0   &lifetime_32
// 1   &frame_counter
// 2   &schedule->offset
// 3   &schedule->epoc_time



// interesting
r0.setPartnerTDMA(11, 4);   // is lifetime_32 correct?
r0.setPartnerTDMA(11, 96);  // is epoc correct?

setBoth(11, 97);
setBoth(11, 10);


sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_TDMA_SC_CMD | 17 ); // run this on 2nd radio


zmq.r(0).eval('sjs.settings.getIntDefault(-1,"exp.our.id")').then(console.log)
zmq.r(0).eval('Date.now()').then(console.log)

var stuffBoth = () => {
  //zmq.r(o).eval('behavior.stuffData()').then();
  //zmq.r(1).eval('behavior.stuffData()').then();
  let s = "(()=>{a = []; for(let i = 0; i < 100000; i++){a.push(i)}; sjs.queueTxData(a)})()";
  zmq.r(0).eval(s).then();
  zmq.r(1).eval(s).then();
};

(()=>{})()


sjs.dsp.sendCustomEventToPartner(1, hc.REQUEST_MAPMOV_DEBUG, 1);
sjs.dsp.sendCustomEventToPartner(2, hc.REQUEST_MAPMOV_DEBUG, 1);

sjs.dsp.sendCustomEventToPartner(1, hc.EXIT_DATA_MODE_EV, 0);
sjs.dsp.sendCustomEventToPartner(2, hc.EXIT_DATA_MODE_EV, 0);


var debugBoth = () => {
  let s = "(()=>{sjs.txSchedule.debugBeamform = true; return sjs.txSchedule.debugBeamform;)()";
  zmq.r(0).eval(s).then(console.log);
  zmq.r(1).eval(s).then(console.log);
};

sjs.txSchedule.debugBeamformSize = 20000;
sjs.txSchedule.debugBeamformSize = 40000;


var beam = () => {
  var epoc = Math.trunc(sjs.dsp.fuzzy_recent_frame / (512*50));
  epoc += 3;
  sjs.txSchedule.debugJoint(epoc, 12);
  console.log("requested beamform at " + epoc);
};

var handle = setInterval(beam,2000);

*/

/*

var EventLoopMonitor = require('evloop-monitor');
var monitor = new EventLoopMonitor(200);
monitor.start();

// let's get the status
var intv = setInterval(function() {
  console.log('');
    console.log(monitor.status());
}, 2000);

*/


/*

GET_EQ_UPDATE_SECONDS

sjs.settings.setDouble("dsp.eq.update_seconds", 2)
sjs.getDoubleDefault(0, "dsp.eq.update_seconds")


Get last mapmov received by cs11

sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.CHECK_LAST_USERDATA_CMD );



*/

/*

sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.TX_TONE_ONLY_CMD | 1, 1);

*/

// only run on tx side
let stoBoth = (x) => {
  const y = 1280 - x;
  sjs.dsp.setPartnerSfoAdvance(1, y, 3);
  sjs.cs31CoarseSync(y, 3);
}


let ss = (x) => {
  // true if rx should tx
  const enableRx = x === 0;

  let bothCurrent = sjs.getIntDefault(-1, 'hardware.tx.bootload.after.tx_channel.current');

  if( enableRx ) {
    // sjs.dsa(0);
    sjs.current(bothCurrent);
    sjs.zmq.id(1).eval('sjs.safe();sjs.dsa(0)').then((x)=>{ console.log(x); });
    console.log("RX radio is transmitting");
  } else {
    sjs.safe();
    sjs.dsa(0);
    sjs.zmq.id(1).eval(`sjs.current(${bothCurrent})`).then((x)=>{ console.log(x); });
    console.log("TX radio is transmitting");
  }

}


let fixeq = () => {
  r0.pause_eq = true;
  r0.use_sto_eq_method = false;

  r0.setRandomRotationEq();
  r0.dspRunStoEq(1.0);
  r0.updatePartnerEq(false, false);

  setTimeout(()=>{
    let r0sto = Math.floor( (r0.sto_estimated / 256.0) );

    if( r0sto > 0 ) {
      sjs.dsp.setPartnerSfoAdvance(1, r0sto, 3)
    } else {
      sjs.dsp.setPartnerSfoAdvance(1, -r0sto, 4)
    }

    r0.pause_eq = false;

  },8000);


}


let demo0 = () => {
  sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAGADJUST_CMD | 0 );
  for(let r of [r0,r1] ) {
    r.enable_residue_to_cfo = false;
  }
  sjs.settings.setBool('dsp.sto.adjust_using_eq.enable', false);
  sjs.settings.setBool('dsp.sfo.adjust_using_eq.enable', false);
};

let demo1 = () => {
  sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAGADJUST_CMD | 2 );
  for(let r of [r0,r1] ) {
    r.enable_residue_to_cfo = true;
  }
  sjs.settings.setBool('dsp.sto.adjust_using_eq.enable', true);
  sjs.settings.setBool('dsp.sfo.adjust_using_eq.enable', true);
};




let setBoth = (a,b) => {
  r0.setPartnerTDMA(a, b);
  r1.setPartnerTDMA(a, b);
}

// sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.AUTO_GAIN_CMD | 0x0 );
// sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.AUTO_GAIN_CMD | 0x1 );


let barrel = () => {
  // sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_MASK_SC_CMD | 0x00000 | 17 );
  // sjs.ringbus(hc.RING_ADDR_CS11, hc.COOKED_DATA_TYPE_CMD | 1 );
  // sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_CS11, hc.COOKED_DATA_TYPE_CMD | 1, 1);
  // sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_CS11, hc.SET_MASK_SC_CMD | 0x00000 , 1);
  sjs.ringbus(hc.RING_ADDR_CS20, hc.SET_MASK_SC_CMD | 0x00000 | 17 );
  
  sjs.ringbus(hc.RING_ADDR_CS11, hc.EQ_ROTATION_CMD | 1);

  sjs.ringbus(hc.RING_ADDR_CS11, hc.GET_TIMER_CMD);sjs.ringbus(hc.RING_ADDR_CS31, hc.GET_TIMER_CMD);

  sjs.ringbus(hc.RING_ADDR_CS12, hc.ZERO_TX_CMD | 150);

  sjs.dsp.ringbusPeerLater(17, hc.RING_ADDR_TX_FFT, 0x57000010, 1);
  sjs.dsp.ringbusPeerLater(17, hc.RING_ADDR_TX_FFT, 0x57010010, 1);
  sjs.dsp.ringbusPeerLater(17, hc.RING_ADDR_TX_FFT, 0x57020010, 1);
  sjs.dsp.ringbusPeerLater(17, hc.RING_ADDR_TX_FFT, 0x5703000f, 1);
  sjs.dsp.ringbusPeerLater(17, hc.RING_ADDR_TX_FFT, 0x5704000f, 1);



  let values = [
0x0f
,0x10
,0x0f
,0x0f
,0x0f
  ];



  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_FFT, 0x5700000f, 1);
  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_FFT, 0x5701000f, 1);
  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_FFT, 0x5702000f, 1);
  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_FFT, 0x5703000f, 1);
  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_FFT, 0x5704000e, 1);



  // outdoors fire hydrant
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5700000c);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5701000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5702000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5703000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5704000f);
  sjs.ringbus(hc.RING_ADDR_RX_FINE_SYNC, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x12);
  sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 6000 );






  // outdoors end of street
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5700000a);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5701000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5702000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5703000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5704000f);
  sjs.ringbus(hc.RING_ADDR_RX_FINE_SYNC, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x12);
  sjs.ringbus(hc.RING_ADDR_RX_EQ, hc.MAG_FILTER_GAIN_CMD | 10000 );







  // outdoors end of street again
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x57000009);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5701000e);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5702000e);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5703000f);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, 0x5704000f);
  sjs.ringbus(hc.RING_ADDR_RX_FINE_SYNC, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x14);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0c);
  sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f);



  sjs.ringbus(hc.RING_ADDR_CS31, hc.POWER_ESTIMATION_CMD | 0 );

// 00000004 5700000f
// 00000004 57010010
// 00000004 5702000f
// 00000004 5703000f
// 00000004 5704000f


};

let cfoshift = () => {

  // note the CFO must be enabled for this barrel shift to be in effect
  // sjs.ringbus(hc.RING_ADDR_CS12, hc.TX_CFO_LOWER_CMD | 1 );
  // sjs.ringbus(hc.RING_ADDR_CS12, hc.TX_CFO_UPPER_CMD | 0x010000 );
  sjs.ringbus(hc.RING_ADDR_CS12, hc.TX_CFO_LOWER_CMD | 1 );
  sjs.ringbus(hc.RING_ADDR_CS12, hc.TX_CFO_UPPER_CMD | 0x010000 );


  sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.APP_BARREL_SHIFT_CMD | 0x0e, 1);

};


let maskIntv;
let checkSetMask = () => {
  if( role !== 'rx') {
    clearInterval(maskIntv);
    return;
  }
  // console.log(sjs.dsp.dsp_state_pending);
  if( sjs.dsp.dsp_state_pending == hc.COARSE_SCHEDULE_0 ) {
    r0.setAllEqMask(m0);
    //sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.APP_BARREL_SHIFT_CMD | 0x0f, 1);
    clearInterval(maskIntv);
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------    SET EQMASK");
    //r0.updatePartnerEq(false, false);

  }
};

//maskIntv = setInterval(checkSetMask, 5);

let maskIntv2;
let checkSetMask2 = () => {
  if( role !== 'rx') {
    clearInterval(maskIntv);
    return;
  }
  if( sjs.dsp.dsp_state_pending == hc.DO_TRIGGER_0 ) {
    r0.setAllEqMask([]);
    //sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.APP_BARREL_SHIFT_CMD | 0x0f, 1);
    //r0.updatePartnerEq(false, false);
    clearInterval(maskIntv2);
    console.log("-----------------------    SET EQMASK");
  }
  // console.log("                          " + sjs.dsp.dsp_state_pending);
};

//maskIntv2 = setInterval(checkSetMask2, 1);










let maskIntv3;
let checkSetMask3 = () => {
  if( role !== 'rx') {
    clearInterval(maskIntv);
    return;
  }
  // console.log(sjs.dsp.dsp_state_pending);
  let st = r0.radio_state_pending;
  // console.log(st);
  if( st == hc.SFO_STATE_0 ) {
    r0.setAllEqMask(m4);
   //sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.APP_BARREL_SHIFT_CMD | 0x0d, 1);
    clearInterval(maskIntv3);
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------");
    console.log("-----------------------    R0 SET SFO EQMASK");
    //r0.updatePartnerEq(false, false);
    // pow.force(-0.1);
    pow.force(10);
  }
};

//maskIntv3 = setInterval(checkSetMask3, 10);

let maskIntv4;
let checkSetMask4 = () => {
  if( role !== 'rx') {
    clearInterval(maskIntv);
    return;
  }
  let st = r0.radio_state_pending;
  if( st == hc.CFO_0 ) {
    r0.setAllEqMask([]);
    //sjs.dsp.ringbusPeerLater(1, hc.RING_ADDR_TX_TIME_DOMAIN, hc.APP_BARREL_SHIFT_CMD | 0x0f, 1);
    //r0.updatePartnerEq(false, false);
    clearInterval(maskIntv4);
    console.log("-----------------------    SET EQMASK");

    setTimeout(()=>{pow.check()}, 2000);
  }
  // console.log("                          " + sjs.dsp.dsp_state_pending);
};

//maskIntv4 = setInterval(checkSetMask4, 1);










let sfo = (x) => {
  sjs.r[0].sfo_estimated = x;  sjs.r[0].updatePartnerSfo(); sjs.r[0].sfo_estimated = 0;
}





/*

sjs.settings.setBool('global.print.PRINT_TX_BUFFER_FILL', true)

// cs32 fine sync
sjs.ringbus(hc.RING_ADDR_RX_FINE_SYNC, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x07);
sjs.ringbus(hc.RING_ADDR_RX_FINE_SYNC, hc.APP_BARREL_SHIFT_CMD | 0x020000 | 0x00);

// cs31 coarse
sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0f);
sjs.ringbus(hc.RING_ADDR_RX_FFT, hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f);

*/

// sjs.dsp.downsample_gnuradio = 4;

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
,r2
,r3
,setBoth
,pow
,barrel
,g3ctrl
,watchMode
// ,watchModeIntv
,demo0
,demo1
,fixeq
,sfo
,hrb
,stoBoth
,ss
,handleSelfSync
,isDebugMode
,test_ring
};
zmq.setContext(context);
context.zmq = zmq;
replify('sjs', remoteTerminal, context);

} // hwOk()
