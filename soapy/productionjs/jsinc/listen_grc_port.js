'use strict';
// const net = require('net');
const dgram = require('dgram');
// const Rpc = require('grain-rpc/dist/lib');
const mutil = require('../../js/mock/mockutils.js');
// const { spawn } = require('child_process');
// const _ = require('lodash');


class ListenGrcPort {

  constructor(options={}) {
    // grab defaults from options
    const { printConnections = false, launcherPort = 10033, cb = null } = options;
  
    // load defaults into object
    // this.printRpc = printRpc;
    this.printConnections = printConnections;
    this.launcherPort = launcherPort;
    this.cb = cb;

    // locals
    this.data_p = null;
    this.data = null;

    this.createAndOpenSocket();

    this.feedCbInterval = setInterval(this.feedCb.bind(this), 333);
  }


  gotSocketData(data) {

    // let temp = mutil.IShortToStreamable([34]);

    let pack = [data[0],data[1],0,0];
    let temp2 = mutil.streamableToIShort(pack);
    this.data = temp2[0];//this.cb()
  }

  feedCb() {
    if( this.data !== this.data_p ) {
      if( this.cb !== null ) {
        this.cb(this.data);
      }

    }
    this.data_p = this.data;
  }



  createAndOpenSocket() {

    this.server = dgram.createSocket('udp4');

    // server.on

    this.server.on('error', (err) => {
      throw err;
    });

    this.server.on('message', (msg, rinfo) => {
        if( this.printConnections ) {
          console.log('client data');
        }
        this.gotSocketData(msg);
      });
    this.server.bind(this.launcherPort, '0.0.0.0');

  }

} // class 


module.exports = {
    ListenGrcPort
}
