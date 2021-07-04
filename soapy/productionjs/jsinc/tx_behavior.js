'use strict'

var Prando = require('prando');
let runtimeRandom = require('../../js/runtime/runtime_core.js');
const mutil = require('../../js/mock/mockutils.js');
const sharedFile = require('./txrx_shared_behavior');
const hc = require('../../js/mock/hconst.js');

class TxBehavior extends sharedFile.TxRxSharedBehavior {
  constructor(sjs) {
    super(sjs);
    this.name = 'TxBehavior';

    this.state = 0;

    this.setup();
    this.setupRepair();
    this.setupGrav3();
    this.setupFinal();
  }

  setupFinal() {
    // put setup junk in here

      this.debug_ook_lower = 0;
      this.previous_ook_value = -2;
      this.ook_values_received = 0;
      this.sjs.hrb.on(hc.RX_COUNTER_SYNC_PCCMD, this.handleOOKSync.bind(this));
  }

  setupGrav3() {
    let b1 = this.sjs.getBoolDefault(false, 'hardware.tx.bootload.after.config_dac.enable');
    let b2 = this.sjs.getBoolDefault(false, 'hardware.tx.bootload.after.tx_channel.enable');

    if( !b1 || !b2 ) {
      console.log("TxBehavior will not touch grav3 tx mode or current");
      return;
    }

    let chan = this.sjs.getStringDefault('', 'hardware.tx.bootload.after.tx_channel.channel');

    if( chan !== 'a' ) {
      console.log('------------------------------------------');
      console.log('-');
      console.log('- TxBehavior not setup for any channel other than A');
      console.log('-');
      console.log('------------------------------------------');
    }

    const cpath = 'hardware.tx.bootload.after.tx_channel.current';

    let current = this.sjs.getIntDefault(-1, cpath);

    const willSetCurrent = (current !== -1);

    if( !willSetCurrent ) {
      console.log('TxBehavior will not set current ' + cpath + ' was missing');
    }

    if( current > 15 ) {
      current = 15;
      console.log('TxBehavior capping current to 15');
    }

    if( current < 0 ) {
      current = 0;
      console.log('TxBehavior capping current to 0');
    }

    console.log("setting grav3 mode tx");

    this.sjs.tx();

    if( willSetCurrent ) {
      console.log("setting current to " + current);
      this.sjs.current(current);
    }

    // do not put any setup here as it will not run if
    // hardware does not

    // console.log("chan: " + chan);
    // process.exit(0);

  }

  setup() {
    let shouldInjectTxPrnData = this.sjs.getBoolDefault(false, 'exp.js.pseudo_random_pattern.tx_inject');

    this.setupRng();
    
    if( shouldInjectTxPrnData ) {
      this.startSendingRngPackets();
    }
  }

  startSendingRngPackets() {
    this.grabTimer = setInterval(()=>{this.stuffData()}, 1000);
  }

  setupRng() {

    this.rngSettings = {params:{}};
    this.rngSettings.params.bytes = 8000*4;
    this.rngSettings.seq = 44;

    this.rng = new Prando(runtimeRandom.randomSeed + this.rngSettings.seq);
  }

  advanceRandom() {
    this.rngSettings.seq++;
    this.rng = new Prando(runtimeRandom.randomSeed + this.rngSettings.seq);
  }

  sendSize(x) {
    this.rngSettings.params.bytes = x - (x%4);
    console.log("setting send size to quantized value of " + this.rngSettings.params.bytes + " bytes");
  }

  stuffData() {
    // console.log("stuffData");
    let linkUp = this.sjs.getBoolDefault(false, 'runtime.tx.link.up');
    if( linkUp ) {
      // console.log("LINK UP!!!!!!!!!");
      this.sendRandomPacket();
    }
    // console.log(this.name);
    // let res = this.sjs.getRxData();
    // if(typeof res !== 'undefined') {
    //   console.log(res);
    // }
  }

  handleOOKSync(word) {

    let type = (word & 0x00f0000) >>> 16;
    let value = (word & 0xffff) >>> 0;

    if( type == 0 ) {
      this.debug_ook_lower = value;
    } else if( type == 1 ) {
      const built = ((value << 16) | this.debug_ook_lower) >>> 0;

      if( built != this.previous_ook_value ) {
        console.log("OOK Frame Delta :              0x" + built.toString(16));
      }
      this.previous_ook_value = built;
      this.ook_values_received++;
    }

    // console.log(word);
  }

  sendRandomPacket() {
    this.advanceRandom();

    let randbytes = this.rngSettings.params.bytes - 8;

    let seq = this.rngSettings.seq;

    let res2 = runtimeRandom.idealPacket3(this.rng, randbytes, seq/2);

    // console.log(res2);

    let asWords = mutil.streamableToIShort(res2);

    console.log("sending random packet with " + randbytes + " bytes and seq " + seq);
    this.sjs.queueTxData(asWords);

    // console.log(asWords);

    // this.gotPacket(asWords);
  }

  setDrainRate(val) {
    // only works in 320 mode right now
    this.sjs.settings.setDouble('runtime.tx.flush_higgs.drain_rate', val);
  }



  setupRepair() {
    
  }


  // return true if we handled the ringbus
  handleRingbus(x) {

    let superHandled = super.handleRingbus(x);

    const type = x & 0xff000000;
    // put if to catch our own ringbus
    // if( (type) === hc.POWER_RESULT_PCCMD ) {
    //   this.handlePowerRing(x);
    //   return true;
    // }

    return false | superHandled;
  }
}

module.exports = {
    TxBehavior
}
