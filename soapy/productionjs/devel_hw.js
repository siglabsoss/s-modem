'use strict';
const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');
// const sjs = require('./build/Release/sjs.node');
const wrapNative = require('./jsinc/wrap_native.js');
// const wrapHandle = new wrapNative.WrapNativeSmodem(sjs);
const HandleCS20Tx = require('./jsinc/handle_cs20_tx.js');
// const handleTx = new HandleCS20Tx.HandleCS20Tx(sjs);
const fs = require('fs');
const replify = require('replify');
const remoteTerminal = require('http').createServer();
const inject = require('./jsinc/inject_offboard.js');
const wrapDbFile = require('./jsinc/wrap_db.js');
const setupHardwareFile = require('./jsinc/setup_hardware.js');
const staleInit = require('../init.json');  // only use this before sjs is running

// const settings = wrapHandle.settings;

// console.log('hi');

const hwOptions = {staleInit};
const setupHardware = new setupHardwareFile.SetupHardware(hwOptions, hwOk);

function hwOk() {

// let keepAwake = setInterval(()=>{},5000);



// function bar() {
//   var myPromise = new Promise(function(resolve, reject){
//     setTimeout(resolve,500);
//   });

//   return myPromise;
// }

// async function foo() {
//   await bar();
//   console.log('after await');
// }



// foo();



console.log('modem_main starts sjs...');

}