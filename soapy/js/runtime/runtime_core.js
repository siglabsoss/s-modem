'use strict';
const _ = require('lodash');
const dgram = require('dgram');
var Prando = require('prando');
const mutil = require('../mock/mockutils.js');

const randomSeed = 0x9557d6dc;

function idealPacket(rng, randbytes, seq) {
    let countBuf = mutil.IShortToStreamable([seq]);

    let bufList = [];
    bufList.push(countBuf[0]);
    bufList.push(countBuf[1]);
    bufList.push(countBuf[2]);
    bufList.push(countBuf[3]);


    for(let i = 0; i < randbytes; i++) {
      const pull = rng.nextInt(0,255);
      bufList.push(pull);
    }

    return bufList;
}

function idealPacket2(rng, randbytes, seq) {
  let frontBytes = mutil.IShortToStreamable([seq, randbytes]);

  let bufList = [];
  bufList.push(frontBytes[0]);
  bufList.push(frontBytes[1]);
  bufList.push(frontBytes[2]);
  bufList.push(frontBytes[3]);
  bufList.push(frontBytes[4]);
  bufList.push(frontBytes[5]);
  bufList.push(frontBytes[6]);
  bufList.push(frontBytes[7]);


  for(let i = 0; i < randbytes; i++) {
    const pull = rng.nextInt(0,255);
    bufList.push(pull);
  }

  return bufList;
}

function idealPacket3(rng, randbytes, seq) {
  let frontBytes = mutil.IShortToStreamable([seq, randbytes]);

  let bufList = [];
  bufList.push(frontBytes[0]);
  bufList.push(frontBytes[1]);
  bufList.push(frontBytes[2]);
  bufList.push(frontBytes[3]);
  bufList.push(frontBytes[4]);
  bufList.push(frontBytes[5]);
  bufList.push(frontBytes[6]);
  bufList.push(frontBytes[7]);

  let start = (seq << 19) + seq + 0x1e000000 + (seq << 22);

  start = start ^ (start >> 16);

  start = start ^ (start >> (seq%32));

  start = start & 0xffffffff;

  for(let i = 0; i < randbytes/4; i++) {
    let word = mutil.IShortToStreamable([start+i]);

    bufList.push(word[0]);
    bufList.push(word[1]);
    bufList.push(word[2]);
    bufList.push(word[3]);
  }

  return bufList;
}




function idealPacket4(seq, seq2, seq3, start, lenWords) {
  let frontBytes = mutil.IShortToStreamable([seq, seq2, seq3]);

  let bufList = [];
  bufList.push(frontBytes[0]);
  bufList.push(frontBytes[1]);
  bufList.push(frontBytes[2]);
  bufList.push(frontBytes[3]);
  bufList.push(frontBytes[4]);
  bufList.push(frontBytes[5]);
  bufList.push(frontBytes[6]);
  bufList.push(frontBytes[7]);
  bufList.push(frontBytes[8]);
  bufList.push(frontBytes[9]);
  bufList.push(frontBytes[10]);
  bufList.push(frontBytes[11]);

  // let start = (seq << 19) + seq + 0x1e000000 + (seq << 22);

  // start = start ^ (start >> 16);

  // start = start ^ (start >> (seq%32));

  // start = start & 0xffffffff;

  for(let i = 0; i < lenWords; i++) {
    let word = mutil.IShortToStreamable([start+i]);

    bufList.push(word[0]);
    bufList.push(word[1]);
    bufList.push(word[2]);
    bufList.push(word[3]);
  }

  return bufList;
}







function getSettings(asWords) {
  let seq = asWords[0];
  let len = asWords[1];
  return [seq, len];
}

class RuntimeServer {
  
  constructor(options){
    Object.assign(this, options);

    this.dsock = dgram.createSocket('udp4');

    this.exp = null;

    this.times = [];
  }

  setupDsock() {

    this.dsock.on('error', (err) => {
      console.log(`dsock error:\n${err.stack}`);
      this.dsock.close();
    });

    this.dsock.on('message', (buf, rinfo) => {
      if( false ) {
        console.log(`dsock got ${buf.length} from ${rinfo.address}:${rinfo.port}`);
      }
      // let bufstr = JSON.stringify(buf);
      // console.log(bufstr);
      this.gotPacket(buf);
      // this.fbparse.gotPacket(buf);

    });

    this.dsock.on('listening', () => {
      const address = this.dsock.address();
      console.log(`dsock listening ${address.address}:${address.port}`);


      // this.fbparse = new FBParse.Parser();
      // this.fbparse.reset();

      // this.dsockListeningFinished();

    });

    // bind to TX port as this was taken from s-modem
    // and tx/rx are flipped as we are mock
    this.dsock.bind(this.rx_port);
  }

  gotSetup(buf) {
    let tail = buf.slice(4);

    let str = tail.toString('ascii');

    // console.log(str);

    if( this.exp !== null ) {
      console.log("previous experiment: " + JSON.stringify(this.exp) + " was already running");
      this.exp = null;
    }

    this.exp = {status:{got:0,gotOk:0},params:JSON.parse(str)};

    this.rng = new Prando(0x9557d6dc);

    this.times = [];

    this.times.push(Date.now());

    console.log(this.exp); 
  }

  gotPacket(buf) {
    let headerFound = buf.indexOf('ffffffff', 0, 'hex');
    // console.log("inc: " + headerFound);
    if( headerFound === 0 ) {
      this.gotSetup(buf);
      return;
    }

    this.times.push(Date.now());

    let received = [...buf];

    let randbytes = this.exp.params.bytes - 4;
    let seq = this.exp.status.got;
    let expected = idealPacket(this.rng, randbytes, seq);

    // console.log(received);
    // console.log(expected);

    let match = _.isEqual(expected, received);

    if( !match ) {
      console.log("DID NOT MATCH");
      // fixme
    } else {
      this.exp.status.gotOk++;
    }

    let timeb = this.times[this.times.length-1];
    let timea = this.times[this.times.length-2];
    let delta = timeb - timea;

    let tol = 3;

    if( Math.abs(delta-this.exp.params.delay) >= tol ) {
      console.log("Packet " + this.exp.status.got + " was " + delta + "ms but expected " + this.exp.params.delay + "ms");
    }

    // console.log("timea " + timea + " timeb " + timeb);
    // console.log("delta: " + (delta));

    this.exp.status.got++;

    if( this.exp.status.got === this.exp.params.count ) {
      if( this.exp.status.gotOk === this.exp.status.got ) {
        console.log("Id " + this.exp.params.id + " finished ok");
      } else {
        console.log("Id " + this.exp.params.id + " finished WITH ERRORS " + this.exp.status.got + ", " + this.exp.status.gotOk );
      }
      console.log('');

      this.exp = null;
    }

    // this.send(Buffer.from(bufList));

  }
  
}

class RuntimeClient {
  
  constructor(options){
    Object.assign(this, options);

    this.dsock = dgram.createSocket('udp4');
  }

  go(cb = null) {
    // let message = Buffer.from([1,2,3,4]);
    // console.log(message);
    this.goSettings(32, 10, 20, cb);
  }

  goSettings(bytes, delay, count, cb) {
    let id = Math.trunc(Math.random()*1024);

    this.exp = {status:{sent:0},params:{bytes:bytes, delay:delay, count:count, id:id}};
    this.rng = new Prando(0x9557d6dc);

    console.log("Starting with id: " + this.exp.params.id);

    this.cb = cb;

    this.sendHeader();

    this.timer = setTimeout(this.execute.bind(this), this.exp.params.delay);
  }

  send(message) {
    this.dsock.send(message, 0, message.length, this.connect_port, this.connect_ip);
  }

  sendHeader() {
    let bufList = [];
    bufList.push(0xff);
    bufList.push(0xff);
    bufList.push(0xff);
    bufList.push(0xff);

    let msg = JSON.stringify(this.exp.params);

    let msgBuffer = Buffer.from(msg);

    // console.log(msgBuffer);

    let finalBuffer = Buffer.concat([Buffer.from(bufList), msgBuffer]);

    this.send(finalBuffer);

    // console.log(finalBuffer);
    // process.exit(0);

  }

  execute() {
    let randbytes = this.exp.params.bytes - 4;

    let seq = this.exp.status.sent;

    let bufList = idealPacket(this.rng, randbytes, seq);

    this.send(Buffer.from(bufList));

    // console.log(countBuf);
    // console.log();

    this.exp.status.sent++;

    if( this.exp.status.sent == this.exp.params.count ) {
      console.log("done sending " + this.exp.status.sent);
      if( this.cb !== null ) {
        this.cb();
      }
    } else {

      this.timer = setTimeout(this.execute.bind(this), this.exp.params.delay);
    }

  }



}

module.exports = {
    RuntimeServer
  , RuntimeClient
  , idealPacket
  , idealPacket2
  , idealPacket3
  , idealPacket4
  , getSettings
  , randomSeed
}
