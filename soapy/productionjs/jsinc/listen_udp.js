'use strict';
const dgram = require('dgram');
const mutil = require('../../js/mock/mockutils.js');


class ListenUDP {

  constructor(options={}) {
    // grab defaults from options
    const { printConnections = false, launcherPort = 10033, cb = null, ip = null } = options;
  
    // load defaults into object
    // this.printRpc = printRpc;
    this.printConnections = printConnections;
    this.launcherPort = launcherPort;
    this.cb = cb;
    this.listen_ip = ip;

    // locals
    this.data_p = null;
    this.data = null;

    this.createAndOpenSocket();

    this.setupTrack();

    // this.feedCbInterval = setInterval(this.feedCb.bind(this), 333);
  }

  bodyOk(body) {
    let st = body[0];
    let bodyOk = true;

    for(let i = 0; i < body.length; i++) {
      if( body[i] != st ) {
        console.log('bad body at ' + i);
        bodyOk = false;
        break;
      }
      st++;
    }
    return bodyOk;
  }


  setupTrack() {
    this.t = {};
    // this.finished = {};
    this.highest = 0;

  }


  checkFinished(seq1) {
    let k1 = ''+seq1;
    let found = k1 in this.t;

    if( !found ) {
      return false;
    }

    let arr = this.t[k1];

    if( arr.length < 2 ) {
      return false;
    }

    // let s2;
    let s3;

    let build = [];

    for(let i = 0; i < arr.length; i++) {
      let entry = arr[i];

      if( i == 0) {
        // s2 = entry.s2;
        s3 = entry.s3;
      }

      if( s3 != entry.s3 ) {
        console.log('entry for ' + seq1 + ' had a mismatching total');
      }

      if( !entry.bok ) {
        console.log('body corrupt for ' + seq1 + ', ' + entry.s2 );
      }

      build.push(entry.s2);
    }

    let ideal = [];

    for(let i = 0; i < s3; i++) {
      ideal.push(i);
    }


    // build.every(e=>[0,1,2].includes(e))

    if( JSON.stringify(build) != JSON.stringify(ideal) ) {

      let missing = [];

      for( let i = 0; i < s3; i++) {
        if( !build.includes(i) ) {
          missing.push(i);
        }
      }

      console.log("packet bunch was wrong for " + seq1 + " ( " + build.length + " " + ideal.length +  " ) missing: ");
      console.log(''+missing);



      // console.log("packet bunch was wrong. ideal:");
      // console.log(ideal);
      // console.log("                        got  :");
      // console.log(build);
      return false;
    } else {
      return true;
    }

  }

  gotPacket(seq1, seq2, seq3, body) {
    // console.log("   seq1:  " + seq1);
    // console.log("   seq2:  " + seq2);
    // console.log("   seq3:  " + seq3);
    let bodyOk = this.bodyOk(body);

    let k1 = ''+seq1;
    let k2 = ''+seq2;

    // do we already have a key?
    let found = k1 in this.t;

    if( !found ) {
      this.t[k1] = [];
    }

    this.t[k1].push({'s2':seq2, 's3':seq3, 'bok': bodyOk});

    this.handleLife(seq1);

    // console.log(this.t);
  }

  handleLife(seq1) {

    let high = this.highest;
    
    if( seq1 > high ) {
      this.highest = seq1;
    } else {
      return;
    }

    let res = this.checkFinished(high-1);
    if( res ) {
      let arr = this.t[''+high];
      console.log('all ok for ' + high + '    (' + arr[0].s3 + ' packets)');
    }

  }


  gotSocketData(data) {
    // console.log(data);
    let asWords = mutil.streamableToIShort(data);
    // console.log(asWords);

    if( asWords < 3 ) {
      console.log('ListenUDP got malformed packets');
      console.log(data);
      return;
    }



    let seq1  = asWords[0];
    let seq2 = asWords[1];
    let seq3 = asWords[2];
    let body = asWords.slice(3);



    this.gotPacket(seq1, seq2, seq3, body);
    return;
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
    this.server.bind(this.launcherPort, this.listen_ip);

  }

} // class 


module.exports = {
    ListenUDP
}
