'use strict';
// const net = require('net');
const dgram = require('dgram');
// const Rpc = require('grain-rpc/dist/lib');
const mutil = require('../../js/mock/mockutils.js');
// const { spawn } = require('child_process');
// const _ = require('lodash');

const rcore = require('../../js/runtime/runtime_core.js');


class SendUDP {

  constructor(options={}) {
    // grab defaults from options
    const { printConnections = false, launcherPort = 10033, cb = null, ip = null, delay = 2000 } = options;
  
    // load defaults into object
    // this.printRpc = printRpc;
    this.printConnections = printConnections;
    this.launcherPort = launcherPort;
    this.cb = cb;
    this.listen_ip = ip;
    this.delay = delay;

    // locals
    this.data_p = null;
    this.data = null;

    this.createAndOpenSocket();

    this.seq = 0;
    this.len = 300;
    this.count = 10;
    this.waitAfter = 10; // 1 ms

    this.auto3();

    // this.feedCbInterval = setInterval(this.feedCb.bind(this), 333);
  }


  gotSocketData(data) {
    console.log(data);

    // let temp = mutil.IShortToStreamable([34]);

    // let pack = [data[0],data[1],0,0];
    // let temp2 = mutil.streamableToIShort(pack);
    // this.data = temp2[0];//this.cb()
  }

  feedCb() {
    if( this.data !== this.data_p ) {
      if( this.cb !== null ) {
        this.cb(this.data);
      }

    }
    this.data_p = this.data;
  }

  send(message) {
    this.server.send(message, this.launcherPort, this.listen_ip, (err)=>{
      if( err != null ) {
        console.log('error in send');
        console.log(err);
      }
    });
  }



  createAndOpenSocket() {
    this.server = dgram.createSocket('udp4');
    this.server.on('error', (err) => {
      throw err;
    });
  }

  // getPacket() {
  //   // mode
  //   this.sz = 3000;
  //   this.chunk = 1000;

  //   let added = 0;

  //   let out = [];

  //   while(1) {

  //   }
  // }

  // seq
  // total number of packets
  // start of random data in each packet
  // length of each packet
  batch(seq, total, start, len) {
    let out = [];
    for(let i = 0; i < total; i++) {
      let a = rcore.idealPacket4(seq, i, total, 0x0f0f0f0f, len);
      let b = Buffer.from(a);
      out.push(b);
    }
    return out;
  }

  auto2() {
    let b = this.batch(0, 3, 0xff, 10);
    for( let x of b ) {
      this.send(x);
    }
  }

  autoSend() {
    let startv = (this.seq * 0xf0f0) & 0xffffffff;

    let b = this.batch(this.seq, this.count, startv, this.len);
    this.seq++;

    let ttime = 0;

    for( let i = 0; i < b.length; i+= this.waitAfter) {

      setTimeout(()=>{

        for( let j = i; j < (i+this.waitAfter) && j < b.length; j++ ) {
          let x = b[j];
          this.send(x);
          // console.log(j);
        }
      },ttime);

      ttime++;
    }
  }


  auto3() {
    this.intv = setInterval(this.autoSend.bind(this), this.delay);
  }


  auto() {
    let a = rcore.idealPacket4(1, 2, 3, 0x0f0f0f0f, 10);
    let b = Buffer.from(a);
    this.send(b);
    // console.log(a);
    // this.send(a);
    // return a;
  }




} // class 


module.exports = {
    SendUDP
}
