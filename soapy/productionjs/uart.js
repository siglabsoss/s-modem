'use strict';
if(!require('semver').satisfies(process.version, '>=11.0.0')) {
  console.log('\n\n    Node version was wrong, please run:   nvm use 11\n\n'); process.exit(1);
}

// const hc = require('../js/mock/hconst.js');
// const mutil = require('../js/mock/mockutils.js');
// const fs = require('fs');
const init = require('../init.json');  // only use this before sjs is running
const UartFile = require('./jsinc/grav3_uart.js');


if(  init
  && init.hardware
  && init.hardware.grav3
  && init.hardware.grav3.uart
  && init.hardware.grav3.uart.path
  ) {

let uart = new UartFile.Grav3Uart(init.hardware.grav3.uart.path);
} else {
  console.log('init.hardware.grav3.uart.path is missing, this file will not work');
}














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