'use strict';
const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');
const sjs = require('./build/Release/sjs.node');
const wrapNative = require('./jsinc/wrap_native.js');
const wrapHandle = new wrapNative.WrapNativeSmodem(sjs);
const HandleCS20Tx = require('./jsinc/handle_cs20_tx.js');
const handleTx = new HandleCS20Tx.HandleCS20Tx(sjs);
const fs = require('fs');

// const settings = wrapHandle.settings;

// console.log('hi');


let keepAwake = setInterval(()=>{},5000);

sjs.startSmodem();


let cb2 = (word) => {
  let print = true;
  let h1 = handleTx.rbcb(word);
  if(h1) {
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
// sjs.ringbusCallbackCatchType(0x10000000);
sjs.ringbusCallbackCatchType(0x48000000);
// sjs.ringbusCallbackCatchType(hc.MAPMOV_MODE_REPORT);

// sjs.ringbusCallbackCatchType(hc.CS02_PROGRESS_REPORT_PCCMD);
// sjs.ringbusCallbackCatchType(hc.CS02_EPOC_REPORT_PCCMD);



handleTx.register();  // calls more ringbusCallbackCatchType

sjs.registerRingbusCallback(cb);


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


let mydouble = sjs.getDoubleDefault(0.5, "dsp.sfo.background_tolerance.lowerr");
console.log("mydouble " + mydouble);
setTimeout(()=>{
  let mybool = sjs.getBoolDefault(false, "global.print.PRINT_PEER_RB");
  console.log("mybool " + mybool);
}, 10);


setTimeout(()=>{
  let myint = sjs.getIntDefault(-1, "dsp.sfo.symbol_num");
  console.log("myint " + myint);
}, 40);

setTimeout(()=>{
 sjs.settings.setString("global.file_dump.sliced_data._commetnt", "this is amazing!");
  // console.log("mystr " + mystr);
}, 100);



setTimeout(()=>{
  let mystr = sjs.settings.getStringDefault("not found", "global.file_dump.sliced_data._commetnt");
  console.log("mystr fetch " + mystr);
}, 500);


setTimeout(()=>{
  // console.log("Doing Coarse sync from js");

  // sjs.ringbus(hc.RING_ADDR_CS00, hc.SYNCHRONIZATION_CMD | (0x1) );

}, 5000);

// directly to feedbac bus

 // sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_REPORT_ERRORS_CMD );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.FB_REPORT_STATUS_CMD );


// sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_MAIN_REPORT_STATUS_CMD );

// sjs.ringbus(hc.RING_ADDR_CS20, hc.CS02_PROGRESS_CMD | 1000 ); handleTx.check()
// sjs.ringbus(hc.RING_ADDR_CS20, hc.CS20_MAIN_REPORT_STATUS_CMD );


setTimeout(()=> {
  handleTx.check();
}, 2000);

setTimeout(()=> {
  handleTx.printCapture();
}, 2600);



let raw1;
raw1 = fs.readFileSync('../js/test/data/crash_mapmov_1.raw');
// mutil.writeHexFile('crash_mapmov_1.hex', mutil.streamableToIShort(raw1));
let raw2 = raw1.slice(10240*4);
raw2 = raw2.slice(32*4*8);
let tail = 0; // 1040 + 784
let mycut = (66 + 1040 + 1040 + 66 + 1040 + 1040 + 1040 + 1040 + 1040 + 80 + 1040 + 1040 + 144 + 144 + 1040 + 144 + 144 + 784 + 784 + 784 + 784 + 1040 + 784 + 784 + 784 + tail) * 4;
raw2 = raw2.slice(mycut);

// trying to send just start. also seems to corrput it?
// raw2 = raw2.slice(0,1040);

let raw3 = mutil.streamableToIShort(raw2);

// for(let i = 0; i < 20; i++) {
//   console.log(raw3[i].toString(16));
// }


// process.exit(0);


console.log('raw1 length ' + raw1.length);
console.log('raw3 length ' + raw3.length);

let advanceEpoc = ()=> {

  // announce for a bit
  sjs.ringbus(hc.RING_ADDR_CS20, hc.CS02_PROGRESS_CMD | 1000 );

  // reset to 0
  sjs.ringbus(hc.RING_ADDR_CS20, hc.RESET_LIFETIME_CMD );

  setTimeout(()=>{
    const frames = 50*512;
    const max_bump = 0x7fffff; // 327 * frames;

    let target_epoc = 2100;
    
    let target = target_epoc * frames;
    let target2 = target;

    while(target2) {
      let run = Math.min(target2, max_bump);
      console.log("run " + run);
      target2 -= run;

      sjs.ringbus(hc.RING_ADDR_CS20, hc.ADD_LIFETIME_CMD | run );
      // target2 -= min()
    }

    setTimeout(sendCorruptingData, 3000);
  }, 50);

}

let sendCorruptingData = ()=> {
  sjs.sendToHiggs(raw3);
  console.log('now');
}

sjs.sendZerosToHiggs(16*128);

setTimeout(advanceEpoc, 6000);

/*

handleTx.check();

handleTx.printCapture();

*/


// sjs.sendZerosToHiggs(16*1024)
// sjs.sendZerosToHiggs(16*128)


// sjs.settings.setBool('exp.debug_tx_fill_level', true)


//  REQUEST_MAPMOV_REPORT
// sjs.ringbus(hc.RING_ADDR_ETH, hc.REQUEST_MAPMOV_REPORT );

// sjs.ringbus(hc.RING_ADDR_ETH, hc.MAPMOV_RESET_CMD );


// sjs.ringbus(hc.RING_ADDR_CS20, hc.TDMA_CMD | (21<<16) ); sjs.ringbus(hc.RING_ADDR_CS20, hc.SCHEDULE_CMD | 0xffff )
// sjs.ringbus(hc.RING_ADDR_CS20, hc.SCHEDULE_RESET_CMD );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.RESET_LIFETIME_CMD );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.ADD_LIFETIME_CMD | 51200 );
// sjs.ringbus(hc.RING_ADDR_CS20, hc.ADD_LIFETIME_CMD | (0xffffff - 100000) );





// sjs.setFlushHiggsPrint(true)
// sjs.setFlushHiggsPrint(true)

// flush any pending inputs?


// sjs.setFlushHiggsPrintSending(true)

// let a = sjs.flushHiggsFullAge();

// let a = sjs.getDataHistory();
// fs.writeFileSync('crash_mapmov_21.hex', sjs.getDataHistory())
// require('fs').writeFileSync('buf4.raw', sjs.getDataHistory())

// mutil.writeHexFile('crash_mapmov_29.hex', mutil.streamableToIShort(sjs.getDataHistory()))

// console.log(a);


// let b = mutil.streamableToIShort(a);

// for (let x of b) {
//   console.log(x.toString(16));
// }

// process.exit(0);

