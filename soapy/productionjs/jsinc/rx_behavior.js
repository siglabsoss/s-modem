'use strict'

const hc = require('../../js/mock/hconst.js');
const Prando = require('prando');
const runtimeRandom = require('../../js/runtime/runtime_core.js');
const mutil = require('../../js/mock/mockutils.js');
const fs = require('fs');
const re = require('./radio_estimate.js');
const catchEvent = require('./catch_custom_event.js');
const sharedFile = require('./txrx_shared_behavior');
const evmFile = require('./calc_evm.js');


class RxBehavior extends sharedFile.TxRxSharedBehavior {
  constructor(sjs) {
    super(sjs);
    console.log('RxBehavior boot');
    // this.sjs = sjs;
    this.name = 'RxBehavior';

    if( typeof this.peer_arb === 'undefined' ) {
      this.peer_arb = sjs.settings.getIntDefault(-1, 'exp.peers.arb');
    }

    if( typeof this.our_id === 'undefined' ) {
      this.our_id = sjs.settings.getIntDefault(-1, 'exp.our.id');
    }

    this.setup();
    this.setupCustomEvent();
    this.setupRadioEstimate();
    this.setupDebugBeamform();
    this.setupDb();
    this.setupPower();
    this.setupGrav3();
    this.setupEvm();
  }

  setupEvm() {
    this.calcEvm = new evmFile.CalcEvm();
    this.evmInt = setInterval(this.tickEvm.bind(this), 250);
  }

  tickEvm() {
    // console.log('tick');
    let data = this.sjs.evmThread.getSourceData();

    if( typeof data === 'undefined') {
      return;
    }

    let res = this.calcEvm.ishortEvm(data);

    let sum = 0;
    for(let x of res) {
      sum += x;
    }
    sum /= res.length;

    this.sjs.evmThread.evm = sum;
    // console.log("Evm: " + sum + " db");

  }

  setupGrav3() {
    let b1 = this.sjs.getBoolDefault(false, 'hardware.rx.bootload.after.set_dsa.enable');

    if( !b1  ) {
      console.log("RxBehavior will not touch grav3 rx mode or dsa");
      return;
    }

    const apath = 'hardware.rx.bootload.after.set_dsa.attenuation';

    let attenuation = this.sjs.getIntDefault(-1, apath);

    const willSetAtt = (attenuation !== -1);

    if( !willSetAtt ) {
      console.log('RxBehavior will not set dsa attenuation ' + apath + ' was missing');
    }

    if( attenuation > 63 ) {
      attenuation = 63;
      console.log('RxBehavior capping attenuation to 63');
    }

    if( attenuation < 0 ) {
      attenuation = 0;
      console.log('RxBehavior capping attenuation to 0');
    }

    console.log("setting mode rx");

    this.sjs.rx();

    if( willSetAtt ) {
      console.log("setting dsa attenuation to " + attenuation);
      this.sjs.dsa(attenuation);
    }

    // console.log("chan: " + chan);
    // process.exit(0);

  }

  setupRadioEstimate() {
    console.log('setupRadioEstimate');

    // should we enable the javascript fsm for Radio Estimate?
    let enableFsm = ( 'js' === this.sjs.settings.getStringDefault('c', 'fsm.radio_estimate.type') );

    let options = {printTransitions:true, disable:!enableFsm};


    this.rjs = [];
    this.rjs[0] = new re.RadioEstimate(this.sjs, 0, this.catchEvent.notify, options);
    // this.rjs[1] = new re.RadioEstimate(this.sjs, 1);
  }

  setupCustomEvent() {
    this.catchEvent = new catchEvent.CatchCustomEvent();

    let cb = (x) => {
      this.catchEvent.cb(x);
    };

    this.sjs.customEvent.registerCallback(cb);
  }

  setupPower() {
    this.powerl = 0;
    this.powerh = 0;
    this.powerCb = null;
  }



  gotPower() {
    // console.log("GOT POWER ");
    let shift = (this.powerh>>16) & 0xff;
    // console.log("shift: " + shift);
    // console.log(this.powerl.toString(16));
    // console.log(this.powerh.toString(16));

    let mag2 = 1.0 * this.powerl;

    for(let i = 0; i < shift; i++) {
      mag2 = mag2*2;
    }

    let mag = Math.sqrt(mag2);

    let p1 = 30000.0; // reference
    let p2 = mag;

    let db = 10*Math.log10(p2/p1);

    // console.log("");
    // console.log("");
    console.log("mag2: " + mag2)
    console.log("mag: " + mag)
    console.log("db : " + db + "db")

    if( this.powerCb !== null ) {
      this.powerCb(db, mag, mag2);
    }
    // console.log("");
    // console.log("");


  }

  handlePowerRing(word) {
    let dmode = (word & 0x00ff0000)>>>16;
    let data =  (word & 0x0000ffff)>>>0;

    switch(dmode) {
        case 0:
            this.powerl = data;
            break;
        case 1:
            this.powerl |= (data << 16);
            this.powerl = this.powerl >>> 0;
            break;
        case 2:
            this.powerh = data;
            break;
        case 3:
            this.powerh |= data << 16;
            this.powerh = this.powerh >>> 0;
            this.gotPower();
            break;

        default:
            console.log("UNKNOWN POWER RESULT in rb callback");
            break;
    }
  }


  // return true if we handled the ringbus
  handleRingbus(x) {

    let superHandled = super.handleRingbus(x);

    const type = x & 0xff000000;
    if( (type) === hc.POWER_RESULT_PCCMD ) {
      this.handlePowerRing(x);
      return true;
    }

    return false | superHandled;
  }

  setupDebugBeamform() {
    this.cached_ideal = {};
  }

  updateDbTime() {
    // console.log('updateDbTime');
    let now = Date.now();
    this.sjs.db.set('timeAlive', now).write();
  }

  setupDb() {
    let now = Date.now();
    
    let lastRun = this.sjs.db.get('timeAlive');

    let deltaSeconds = (now-lastRun)/1000;

    console.log("App was asleep for " + deltaSeconds + " seconds");

    // fixme move hotstart code into individual radio
    let canHotStart = false;
    // if( deltaSeconds < 60*2 ) {
    //   let cfo = this.sjs.db.get('history.dsp.cfo.peers.t0').value();
    //   let sfo = this.sjs.db.get('history.dsp.sfo.peers.t0').value();
    //   if( Math.abs(cfo) > 0.001 && Math.abs(sfo) > 0.001 ) {
    //       canHotStart = true;
    //     }
    // }

    if(canHotStart) {
      console.log("attempting hot start");
      this.sjs.settings.setBool('runtime.dsp.hotstart', true);
    } else {
      console.log("resetting last hot data");
      this.sjs.db.set('history.dsp.cfo.peers.t'+'0', 0).write();
      this.sjs.db.set('history.dsp.sfo.peers.t'+'0', 0).write();
    }

    this.updateDbTimeHandle = setInterval(this.updateDbTime.bind(this), 10000);

  }

  setup() {

    let shouldCatchPrnData = this.sjs.getBoolDefault(false, 'exp.js.pseudo_random_pattern.rx_catch');

    this.shouldCatchBeamformData = this.sjs.getBoolDefault(false, 'exp.js.beamform_userdata_pattern.rx_catch');

    if( shouldCatchPrnData ) {
      this.sjs.enableDataToJs();
      this.grabTimer = setInterval(()=>{this.grabData()}, 500);
      this.setupRng();
    } else if ( this.shouldCatchBeamformData ) {
      this.sjs.enableDataToJs();
      this.grabTimer = setInterval(()=>{this.grabData()}, 500);
    }
  }

  setupRng() {

    this.rngSettings = {params:{}};
    this.rngSettings.params.bytes = 16;
    this.rngSettings.seq = 44;

    this.rng = new Prando(runtimeRandom.randomSeed + this.rngSettings.seq);
  }

  advanceRandom() {
    this.rngSettings.seq++;
    this.rng = new Prando(runtimeRandom.randomSeed + this.rngSettings.seq);
  }

  grabData() {
    // console.log(this.name);
    let res = this.sjs.getRxData();
    if(typeof res !== 'undefined') {
      // console.log(res);
      if( this.shouldCatchBeamformData ) {
        this.gotBeamformPacket(mutil.streamableToIShort(res[0]), res[1], res[2], res[3], res[4]);
      } else {
        this.gotPacket(mutil.streamableToIShort(res[0]));
      }
    }

    // this.debugGetPacket();
  }

  // writen to debug the flow, call from grabData() to verify code
  // not for production
  debugGetPacket() {
    this.advanceRandom();

    let randbytes = this.rngSettings.params.bytes - 8;

    let seq = this.rngSettings.seq;

    let res2 = runtimeRandom.idealPacket3(this.rng, randbytes, seq);

    console.log(res2);

    let asWords = mutil.streamableToIShort(res2);
    for(let i = 0; i < asWords.length; i++) {
      console.log(asWords[i].toString(16));
    }

    asWords[2] &= 0xffffefff;

    this.gotPacket(asWords);
  }

  gotPacket(words) {
    let [seq, len] = runtimeRandom.getSettings(words);
    console.log("got seq " + seq);
    console.log("got len " + len);

    const max_valid_len = 2000000;
    if( len > max_valid_len ) {
      console.log("Illegal length of " + len);
      console.log("Will not check bytes");
      return;
    }

    let expected = runtimeRandom.idealPacket3(new Prando(runtimeRandom.randomSeed + seq), len, seq);

    // console.log("expected:");
    // console.log(expected);

    let gotBytes = mutil.IShortToStreamable(words);
    // console.log("got:");
    // console.log(gotBytes);

    let badBytesIndices = [];
    for(let i = 0; i < expected.length; i++) {
      if(expected[i] != gotBytes[i]) {
        badBytesIndices.push(i);
      }
    }

    if( badBytesIndices.length != 0 ) {

      let printBadIndices = badBytesIndices;
      if( printBadIndices.length > 4096 ) {
        printBadIndices = printBadIndices.slice(0,4096);
      }

      console.log("Found " + badBytesIndices.length + " corrupted bytes at " + printBadIndices);
      let saveCorrupt = false;
      if( saveCorrupt ) {
        let o = {settings:{}};
        o.settings.seq = seq;
        o.settings.len = len;
        o.bad = badBytesIndices;
        o.got = JSON.parse(JSON.stringify(Buffer.from(gotBytes))).data;
        o.expected = expected;
        let fileContents = JSON.stringify(o);
        let fileName = 'capture/bad_packet_' + Date.now() + '.json';
        console.log(fileName);
        fs.writeFileSync(fileName, fileContents);

      }
    } else {
      console.log("All correct for seq " + seq);
    }
  }

  /// returns true for cache miss (ie we ran something)
  /// returns false if cache value was already there and we didn't run anything
  /// first argument must be a number
  updateIdealBeamformCache(words_length) {
    
    let found = ''+words_length in this.cached_ideal;

    if( found ) {
      return false;
    }

    console.log('Running initial updateIdealBeamformCache() of debug words with size ' + words_length);

    let initial = 0xdead0000;
    let ideal = mutil.xorshift32_sequence(initial, 350);

    // unsure why this is here, something to do with packet boundaries
    ideal = ideal.concat([0xfffff000]);

    while(ideal.length < words_length) {
      ideal = ideal.concat(ideal);
      // console.log('ideal length ' + ideal.length);
    }

    this.cached_ideal[''+words_length] = ideal;


    return true;
  }

  getIdealBeamform(words_length) {
    this.updateIdealBeamformCache(words_length);
    return this.cached_ideal[''+words_length];

  }

  gotBeamformPacket(words, got_at, len, seq, flags) {
    console.log("got " + words.length + " words " + got_at + " " + len + " " + seq + " " + flags);

    let ideal = this.getIdealBeamform(words.length);

    let ok = true;
    let failedAt = -1;
    for( let i = 0; i < words.length; i++) {
      if( ideal[i] !== words[i] ) {
        failedAt = i;
        ok = false;
        break;
      }
    }

    // let b = 0;

    // console.log(words[b+349].toString(16) + ' ' + ideal[b+349].toString(16));
    // console.log(words[b+350].toString(16) + ' ' + ideal[b+350].toString(16));
    // console.log(words[b+351].toString(16) + ' ' + ideal[b+351].toString(16));
    // console.log(words[b+352].toString(16) + ' ' + ideal[b+352].toString(16));

    // return;
    if( ok ) {
      console.log("got " + words.length + " words CORRECT");
    } else {
      console.log("only got first " + failedAt + " words");
    }


    let result2 = [this.our_id, ok?1:0, got_at, len, seq, flags];

    this.sjs.dsp.sendVectorTypeToPartnerPC(this.peer_arb, hc.FEEDBACK_VEC_WORDS_CORRECT, result2);
  }
}

module.exports = {
    RxBehavior
}
