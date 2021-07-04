'use strict';

// const util = require('util');
const hc = require('../../js/mock/hconst.js');


function _sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

class HandleOOK {
  constructor(sjs) {
    this.sjs = sjs;

    this.setupHandleOOK();
  }


  setupHandleOOK() {
    console.log('foo');

    this.sjs.debugOOK = this.debugOOK.bind(this);

    this.sjs.hrb.notify.removeAllListeners(''+hc.GENERIC_READBACK_PCCMD);

    this.sjs.hrb.on(hc.GENERIC_READBACK_PCCMD, this.gotGenericOpRingbus.bind(this));

    this.schedule_epoc_storage = 0;
    this.schedule_epoc_storage_op = -1;

    this.oneFilterUpper = 0;
    this.oneFilterLower = 0;
    this.zeroFilterUpper = 0;
    this.zeroFilterLower = 0;

    this.waitingCb = ()=>{};

    this.ookRingbusCb = (x)=>{};

    this.sjs.hrb.on(hc.RX_COUNTER_SYNC_PCCMD, (x)=>{this.ookRingbusCb(x)});

    
    this.autoFixState = 0; // waiting
    this.autoFixPrevCount = 0;
    this.autoFixPrint = true;

    setTimeout(()=>{this.automaticFix()}, 4000);

    


  }

  async fix(force = false) {
    if( force ) {
      this.handleFix();
      return;
    }

    let fail = await this.counterFail();
    if( fail === 1 ) {
      this.handleFix();
    }
  }

  automaticFix() {
    if(!this.sjs.settings.getBoolDefault(true, 'data.ook.auto_fix')) {
      return;
    }
    if(this.sjs.role !== 'tx' ) {
      return;
    }
    if(!this.sjs.behavior) {
      // when HandleOOK constructor runs
      // behavior is not yet attached
      // if for some reason it's still not attached by first run
      // something is very wrong
      console.log("handle_ook:automaticFix() Will not run");
    }

    if( this.autoFixState < 3 ) {
      if( this.autoFixPrint ) {
        console.log("Auto fix OOK state " + this.autoFixState + " cnt: " + this.sjs.behavior.ook_values_received);
      }
    }

    let loopTime = 4000;

    switch(this.autoFixState) {
      case 0:
        if(this.sjs.attached.radio_state === hc.RE_FINESYNC) {
          this.autoFixState = 1;
          console.log("handle_ook:::::::::::::::::::::::");
          loopTime = 0;
        } else {

          loopTime = 1000;
        }
        break;
      case 1:
        this.autoFixPrevCount = this.sjs.behavior.ook_values_received;
        this.autoFixState = 2;
        break;
      case 2:
        let delta = this.sjs.behavior.ook_values_received - this.autoFixPrevCount;
        console.log("Delta: " + delta);
        if( delta < 13 ) {
          console.log("Delta is too low forcing a fix");
          setTimeout(()=>{this.fix(true);}, 0);
        } else {
        }
          this.autoFixState = 3;
        break;
      case 3:
        // parking
        loopTime = 20000;
        break;
    }

    setTimeout(()=>{this.automaticFix()}, loopTime);
  }


  // promise
  // returns 0 for counter ok
  // returns 1 for counter not running
  // returns 2 for timeout, ie board probably down
  counterFail() {
    return new Promise(function(resolve,reject) {

      let grab = this.sjs.behavior.ook_values_received;
      let grab2;

      setTimeout(()=>{
        grab2 = this.sjs.behavior.ook_values_received;

        console.log(grab);
        console.log(grab2);

        let delta = grab2 - grab;

        if( grab === grab2 ) {
          console.log("Looks like OOK is stuck. will fix...");
          resolve(1);
        } else {
          console.log("Looks like OOK running fine. got " + delta + " values in 2 seconds");
          resolve(0);
        }
      }, 1000);

      setTimeout(()=>{
        resolve(2);
      }, 2000);

    }.bind(this));
  }

  async handleFix() {
    console.log('fixing...');

    await this.grabValues();
  }

  async grabValues() {

    // this.waitingCb = ()=>{
    //   console.log('a');
    // };

    await this.grabOne(2);
    await this.grabOne(3);
    await this.grabOne(4);
    await this.grabOne(5);

    this.sjs.dsp.op(hc.RING_ADDR_RX_FINE_SYNC, "set", 6, 0);

    await _sleep(200);

    let working;

    let ranges = [10, 256, 1024, 2304, 4096, 65536, 262144, 589824, 1048576, 1638400, 2359296, 3211264, 4194304, 5308416, 6553600, 7929856, 9437184, 11075584, 12845056, 14745600,
    16777216, 67108864, 150994944, 268435456, 419430400, 603979776, 822083584, 1073741824, 1358954496, 1677721600, 2030043136, 2415919104, 2835349504, 3288334336, 3774873600];

    for(let x of ranges) {
      working = await this.tryAValue(x);
      if( working ) {
        console.log("Seems like 0x" + x.toString(16) + " worked!");
        break;
      }

      console.log("Failed");
    }

    this.ookRingbusCb = ()=>{};
    
    // working = await this.tryAValue(0x11000);
    // working = await this.tryAValue(0x51000);



  }

  tryAValue(vh) {
    return new Promise(function(resolve,reject) {

      console.log("OOK trying 0x" + vh.toString(16));

      this.ookRingbusCb = (x)=>{
        console.log('OOK counter ' + x);

        resolve(1);
      };

      setTimeout(()=>{
        resolve(0);
      }, 1000);

      // this.waitingCb = (_sel, _full_value) => {
      //   this.waitingCb = ()=>{};
      //   resolve(0);
      // };


      this.sjs.dsp.op(hc.RING_ADDR_RX_FINE_SYNC, "set", 5, vh);

    }.bind(this));
  }


  grabOne(sel) {
    return new Promise(function(resolve,reject) {

      this.waitingCb = (_sel, _full_value) => {
        // console.log('a');

        if( _sel !== sel ) {
          let e = 'got wrong value out in sel in generic op';
          console.log(e);
          reject(0);
        }

        switch(_sel) {
          case 2:
            this.zeroFilterUpper = _full_value;
            break;
          case 3:
            this.zeroFilterLower = _full_value;
            break;
          case 4:
            this.oneFilterUpper = _full_value;
            break;
          case 5:
            this.oneFilterLower = _full_value;
            break;
          default:
            console.log("grabOne callback does not know how to handle " + _sel);
            break;
        }

        this.waitingCb = ()=>{};
        resolve(0);
      };

      switch(sel) {
        case 2:
        case 3:
        case 4:
        case 5:
          this.sjs.dsp.op(hc.RING_ADDR_RX_FINE_SYNC, "get", sel, 0);
          break;
        default:
          console.log("grabOne does not know how to read " + sel);
          break;
      }

    }.bind(this));
  }

  // FIXME move to its own file for generics
  // and refactor this
  gotGenericOpCallback(sel, full_value) {
    console.log("sel: " + sel + " has " + full_value.toString(16));

    this.waitingCb(sel, full_value);
  }


  debugOOK() {
    console.log("include b");
  }

  

  // FIXME move to its own file for generics
  // and refactor this
  gotGenericOpRingbus(data) {
    // console.log('js2 ' + word.toString(16));
    let dmode = (data >>> 16) & 0xff; // 8 bits
    let data_16 = (data & 0xffff) >>> 0; // 16 bits

    let op   = dmode&0x7 >>> 0;   // 3 bits
    let high = (dmode&0x8) >>> 3; // 1 bit
    let sel  = (dmode&0xf0)>>>4;  // 4 bits

    let op_sel = (dmode&(0x7|0xf0)) >>> 0; // op and sel put together

    if(0) {
        console.log( "op    " + (op).toString(16));
        console.log( "high  " + (high).toString(16));
        console.log( "sel   " + (sel).toString(16));
    }

    if(high === 0) {
        this.schedule_epoc_storage = data_16;
        this.schedule_epoc_storage_op = op_sel;
        return;
    }

    if(this.schedule_epoc_storage_op !== op_sel) {
        // got back to back non matching ops. dropping
        this.schedule_epoc_storage_op = -1;
        return;
    }

    if( op !== 4 ) {
        console.log("Got called with op " + op + " which is illegal");
        this.schedule_epoc_storage_op = -1;
        return;
    }

    let full_value = ((((data_16<<16)&0xffff0000) >>> 0) | (this.schedule_epoc_storage)) >>> 0;

    // fixme refactor
    this.gotGenericOpCallback(sel, full_value);
  }



}


module.exports = {
    HandleOOK
}

/*

delete require.cache[require.resolve('./jsinc/handle_ook.js')];HandleOOKFile = require('./jsinc/handle_ook.js');handleOOK = new HandleOOKFile.HandleOOK(sjs);


*/



/*


sjs.dsp.op(hc.RING_ADDR_CS32, "get", 5, 0)

*/



