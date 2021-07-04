'use strict';
const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const enumerate = require('pythonic').enumerate;
const runtime = require('../runtime/runtime_core.js');



let settings = {
  rx_port: 9999
};



let server = new runtime.RuntimeServer(settings);


server.setupDsock();