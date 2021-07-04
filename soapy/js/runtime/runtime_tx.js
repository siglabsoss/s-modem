'use strict';
const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const enumerate = require('pythonic').enumerate;
const runtime = require('../runtime/runtime_core.js');



let settings = {
  connect_port: 9999
  ,connect_ip: "192.168.1.245"
  // ,connect_ip: "127.0.0.1"
};



let client = new runtime.RuntimeClient(settings);


let delay = 100;
let bytes = 1024;
let count = 200;

let work = () => {
  client.goSettings(bytes, delay, count, work);
  // delay -= 8;

  if( delay < 0 ) {
    delay = 1;
  }

  // count += 100;
}


client.go(work);

// client.go(()=>{
//   client.dsock.close();
// });