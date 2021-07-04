'use strict';
const dgram = require('dgram');
const FBParse = require("../mock/fbparse.js");
const mutil = require('./mockutils.js');
const hc = require('./hconst.js');
const util = require('util');
const EventEmitter = require('events').EventEmitter;
// const hc = require('./hconst.js');
// const _ = require('lodash');


function MockHiggsNotify() {
  EventEmitter.call(this);
  this.setMaxListeners(Infinity);
}
util.inherits(MockHiggsNotify, EventEmitter);


// As of writing, this is the most base class for Higgs
// This handles udp sockets, parsing feedback bus, but nothing else
class HiggsSocket {
  constructor() {
    this.opts = {};

    this._eth_seq = 0;
    this.rbsock = dgram.createSocket('udp4');
    this.dsock = dgram.createSocket('udp4');

    this.rb_callbacks = [];

    this.notify = new MockHiggsNotify();
  }

  sendRb(word) {
    let buf = mutil.I32ToBuf(word);
    let seqbuf = mutil.I32ToBuf(this._eth_seq);

    let buf3 = Buffer.from([
      seqbuf[0], seqbuf[1], seqbuf[2], seqbuf[3],
      buf[0], buf[1], buf[2], buf[3]
      ]);

    this._eth_seq++;

    this.rbsock.send( buf3, this.opts.RX_CMD_PORT, this.opts.SMODEM_ADDR );
  }

  // alias
  ring_block_send_eth(word) {
    return this.sendRb(word);
  }

  gotRb(addr, word) {

    const skipLog = [hc.NODEJS_RPC_CMD];


    let data = word & 0x00ffffff;
    let cmd_type = word & 0xff000000;

    let handled = false;
    for(let x of this.rb_callbacks) {
      if( x.mask == cmd_type && x.fpga == addr ) {

        let shouldLog = this.opts.log.rb;

        if( !this.opts.log.rbNoisy ) {
          let shouldSuppress = skipLog.indexOf(cmd_type) !== -1;
          // console.log("for " + cmd_type.toString(16) + " found " + shouldSuppress);
          if( shouldSuppress ) {
            shouldLog = false;
          }
        }

        if( shouldLog ) {
          console.log("got rb to " + addr.toString(16) + ': ' + word.toString(16));
        }

        x.cb(data);
        handled = true;
        break;
      }
    }

    if( !handled && this.opts.log.rbUnhandled ) {
      console.log("got UNHANDLED rb to " + addr.toString(16) + ': ' + word.toString(16));
    }

  }

  // similar to ring_register_callback on higgs however
  // we have an additional parameter of which fpga it's for
  regRingbusCallback(cb, fpga, mask) {
    if( typeof cb === 'undefined' || typeof fpga === 'undefined' || typeof mask === 'undefined') {
      throw(new Error(`regRingbusCallback cannot accept undefined ${cb}, ${fpga}, ${mask}`));
    }
    this.rb_callbacks.push({cb:cb,fpga:fpga,mask:mask});
  }

  setupRbsock() {

    this.rbsock.on('error', (err) => {
      console.log(`rbsock error:\n${err.stack}`);
      this.rbsock.close();
    });

    this.rbsock.on('message', (buf, rinfo) => {
      if( this.opts.log.rbsock ) {
        console.log(`rbsock got from ${rinfo.address}:${rinfo.port}`);
        let bufstr = JSON.stringify(buf);
        console.log(bufstr);
      }
      // let bufferOriginal = Buffer.from(JSON.parse(bufstr).data);

      let addr;
      let word;

      if( buf.length % 8 !== 0 ) {
        console.log("ringbus got wrong length");
      } else {
        for( let i = 0; i < buf.length; i+=8 ) {
          let addr = mutil.bufToI32(mutil.sliceBuf4(buf, i));
          let word = mutil.bufToI32(mutil.sliceBuf4(buf, i+4));
          this.gotRb(addr,word);
        }
      }


    });

    this.rbsock.on('listening', () => {
      const address = this.rbsock.address();
      console.log(`rbsock listening ${address.address}:${address.port}`);
    });

    // bind to TX port as this was taken from s-modem
    // and tx/rx are flipped as we are mock
    this.rbsock.bind(this.opts.TX_CMD_PORT);

  }

  setupDsock() {

    this.dsock.on('error', (err) => {
      console.log(`dsock error:\n${err.stack}`);
      this.dsock.close();
    });

    this.dsock.on('message', (buf, rinfo) => {
      if( this.opts.log.dsock ) {
        console.log(`dsock got ${buf.length} from ${rinfo.address}:${rinfo.port}`);
      }
      // let bufstr = JSON.stringify(buf);
      // console.log(bufstr);
      this.fbparse.gotPacket(buf);

    });

    this.dsock.on('listening', () => {
      const address = this.dsock.address();
      console.log(`dsock listening ${address.address}:${address.port}`);


      this.fbparse = new FBParse.Parser();
      this.fbparse.reset();

      this.dsockListeningFinished();

    });

    // bind to TX port as this was taken from s-modem
    // and tx/rx are flipped as we are mock
    this.dsock.bind(this.opts.TX_DATA_PORT);
  }

  startSocket() {
    this.setupRbsock();
    this.setupDsock();
  }

}


module.exports = {
    HiggsSocket
}