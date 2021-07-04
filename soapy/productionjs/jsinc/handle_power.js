'use strict';

const hc = require('../../js/mock/hconst.js');
const mutil = require('../../js/mock/mockutils.js');
// const safeEval = require('safe-eval');
// const Rpc = require('grain-rpc/dist/lib');
// const util = require('util');
// const EventEmitter = require('events').EventEmitter;


// function VecNotify() {
//   EventEmitter.call(this);
//   this.setMaxListeners(Infinity);
// }
// util.inherits(VecNotify, EventEmitter);

class HandlePower {
  constructor(sjs, role) {
    this.sjs = sjs;
    this.seq = 0;
    this.role = role;
    this.doNextAdjust = true;


    this.our_id = this.sjs.settings.getIntDefault(-1,"exp.our.id");
    // this.master_id = this.sjs.settings.getIntDefault(-1,"exp.rx_master_peer");

    if( this.our_id === -1 ) {
      console.log("HandlePower got something very wrong from settings: exp.our.id was missing!");
      process.exit(1);
    }

    this.isServer = (this.role === "rx") || (this.role === "arb");

    // Event Emitter for vector types
    // this.notify = new VecNotify();

    this.setupFoo();
    this.setupAll();
  }
  setupFoo() {
    // this.sjs.dsp.zmqCallbackCatchType(hc.FEEDBACK_VEC_JS_ARB);
    // this.sjs.dsp.registerCallback(this.gotZmq.bind(this));
    this.table = [];
    this.activePowerIndex = -1;
  }

  gotPower(db, mag, mag2) {
    console.log("          mag2: " + mag2);
    console.log("          mag: " + mag);
    console.log("          db : " + db + "db");
    if( this.doNextAdjust ) {
      this.adjustPower(db);
    }
  }


  adjustPower(db) {
    let match_index = -1;
    let val_p = -9999; // in db thresh

    for(let i = 0; i < this.table.length; i++) {
      let x = this.table[i];
      const val = x.thresh;

      if( val_p < db && db <= val ) {
        match_index = i;
        break;
      }

      val_p = val;
    }

    if( match_index == -1 ) {
      console.log("warning, using maximum value");
      match_index = this.table.length-1;
    }

    if( match_index === this.activePowerIndex ) {
      console.log("do nothing, already in correct bracket");
      this.powMessage(match_index, false);
      return -1;
    }

    this.injectRingbus(match_index);
    this.powMessage(match_index, true);

    this.activePowerIndex = match_index;

    return match_index;
  }

  powMessage(index, changed) {
    const x = this.table[index];
    let message;
    if(changed) {
      message = 'choose';
    } else {
      message = 'already using';
    }

    console.log('-----------------------------');
    console.log('    Power ' + message + ' thresh: ' + x.thresh);
    console.log('-----------------------------');
  }

  injectRingbus(index) {
    const x = this.table[index];
    
    let niceName = "";

    if( x.cs31 ) {
      niceName = "cs31";
      this.injectRingbusSub(hc.RING_ADDR_CS31, x.cs31);
    }

    
    // console.log("injected into " + niceName);

  }


  injectRingbusSub(fpga, list) {
    let later = 1000;
    for(const x of list) {
      console.log(x.toString(16));
      this.sjs.ringbusLater(fpga, x, later);
      later += 10000;
    }
  }

  check(adjust = true) {
    this.doNextAdjust = adjust;
    this.sjs.ringbus(hc.RING_ADDR_CS31, hc.POWER_ESTIMATION_CMD | 0 );
  }

  force(db = 0) {
    this.adjustPower(db);
  }

  addEntry(obj) {
    // console.log(obj);

    this.table.push(obj);

    this.table.sort(function(a, b){
      var keyA = a.thresh,
          keyB = b.thresh;
      // Compare the 2 dates
      if(keyA < keyB) return -1;
      if(keyA > keyB) return 1;
      return 0;
    });

    // console.log('-----');
    // console.log(this.table);

    // process.exit(0);
  }


  setupAll() {
    this.addEntry( {
      thresh:10,
      cs31:[
             hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0f
            ,hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f

            ,hc.FFT_BARREL_SHIFT_CMD | 0x000000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x010000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x020000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x030000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x040000 | 0x0f
      ]
    });




    this.addEntry({
      thresh:0,
      cs31:[
             hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0f
            ,hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f

            ,hc.FFT_BARREL_SHIFT_CMD | 0x000000 | 0x0b
            ,hc.FFT_BARREL_SHIFT_CMD | 0x010000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x020000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x030000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x040000 | 0x0f
      ]
    });



    this.addEntry({
      thresh:-3,
      cs31:[
             hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0e
            ,hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f

            ,hc.FFT_BARREL_SHIFT_CMD | 0x000000 | 0x0a
            ,hc.FFT_BARREL_SHIFT_CMD | 0x010000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x020000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x030000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x040000 | 0x0f
      ]
    });

    this.addEntry({
      thresh:-8,
      cs31:[
             hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0c
            ,hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f

            ,hc.FFT_BARREL_SHIFT_CMD | 0x000000 | 0x0b
            ,hc.FFT_BARREL_SHIFT_CMD | 0x010000 | 0x0e
            ,hc.FFT_BARREL_SHIFT_CMD | 0x020000 | 0x0e
            ,hc.FFT_BARREL_SHIFT_CMD | 0x030000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x040000 | 0x0f
      ]
    });

    this.addEntry({
      thresh:-12,
      cs31:[
             hc.APP_BARREL_SHIFT_CMD | 0x000000 | 0x0b
            ,hc.APP_BARREL_SHIFT_CMD | 0x010000 | 0x0f

            ,hc.FFT_BARREL_SHIFT_CMD | 0x000000 | 0x09
            ,hc.FFT_BARREL_SHIFT_CMD | 0x010000 | 0x0e
            ,hc.FFT_BARREL_SHIFT_CMD | 0x020000 | 0x0e
            ,hc.FFT_BARREL_SHIFT_CMD | 0x030000 | 0x0f
            ,hc.FFT_BARREL_SHIFT_CMD | 0x040000 | 0x0f
      ]
    });






  }
}


module.exports = {
    HandlePower
}
