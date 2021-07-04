'use strict';

const hc = require('../../js/mock/hconst.js');
const util = require('util');
const Prando = require('prando');

class TortureFeedbackBus {
  constructor(sjs) {
    this.sjs = sjs;

 
    this.sjs.torture_fb = this.torture.bind(this);
    this.mode = 0;

    this.tmr = null;
    this.rng = null;
    this.rngSeed = null;
    this.rngHistory = [];
    this.wakes = 0;
    this.work = [];
    this.on_exit = [];
    this.setupModes();
    this.error_count = 0;
    this.automatic_exit_with_error = true;
    this.will_auto_exit = false; // (due to error)
    this.setupCatch();
  }


  exit() {
    this.torture(0); // want to self kill
  }

  dispatchOnExit(old_mode_copy) {
    if(this.on_exit[old_mode_copy]) {
      this.on_exit[old_mode_copy]();
    }
  }

  setupModes() {

    let sjs = this.sjs;

    this.work[1] = () => {
      console.log("in mode " + this.mode);

      if( this.wakes === 0) {
        this.changeTimer(600);

        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 0);
        sjs.sendZerosToHiggs(64)

      }

      if( this.wakes === 1) {
        this.changeTimer(200);
        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 2);
      }

      if( this.wakes >= 2) {
        sjs.sendZerosToHiggs(64);
      }

      // if( 0 ) {
      //   if( (this.wakes+1) >= 2 ) {  // will get 2 wakesup.. why
      //     this.exit();
      //   }
      // }
    }

    this.work[2] = () => {
      console.log("in mode " + this.mode);

      if( this.wakes === 0) {
        this.changeTimer(600);

        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 0);
        sjs.sendZerosToHiggs(64)

      }

      if( this.wakes === 1) {
        this.changeTimer(200);
        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 2);

        sjs.attached.setRandomRotationEq();
      }

      if( this.wakes >= 2) {
        sjs.attached.updatePartnerEq(false, false);


        if( this.rng.nextInt(0, 1024) < 90 ) {
          const new_speed = this.rng.nextInt(50, 444);
          console.log("changing speed to " + new_speed);
          this.changeTimer(new_speed);
        }
      }

    }


    this.work[3] = () => {
      console.log("in mode " + this.mode);

      if( this.wakes === 0) {
        this.changeTimer(600);

        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 0);
        sjs.sendZerosToHiggs(64)

      }

      if( this.wakes === 1) {
        this.changeTimer(200);
        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 2);
      }

      if( this.wakes >= 2) {
        sjs.test_fb();


        if( this.rng.nextInt(0, 1024) < 90 ) {
          const new_speed = this.rng.nextInt(50, 444);
          console.log("changing speed to " + new_speed);
          this.changeTimer(new_speed);
        }
      }

    }

    this.work[4] = () => {
      console.log("in mode " + this.mode);

      if( this.wakes === 0) {
        console.log("\n\n-------------------\nin mode " + this.mode + " This mode forces tx node, and does not cleanup, do not run if experimenting");
        this.changeTimer(1000);

        sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 0);
        sjs.sendZerosToHiggs(1024)

      }

      if( this.wakes === 1) {
        this.changeTimer(2000);
        // sjs.ringbus(hc.RING_ADDR_CS20, hc.COOKED_DATA_TYPE_CMD | 2);

        sjs.txSchedule.setupDuplexAsRole("tx")
        sjs.dsp.fsm_print_peers = false
        sjs.txSchedule.doWarmup = true;
        sjs.txSchedule.debug_extra_gap = 3000;

        sjs.ringbus(hc.RING_ADDR_TX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
        sjs.ringbus(hc.RING_ADDR_TX_1,  hc.COOKED_DATA_TYPE_CMD | 2);

        sjs.dsp.op(hc.RING_ADDR_TX_PARSE    , "set", 10, 1);
        sjs.dsp.op(hc.RING_ADDR_TX_FFT      , "set", 10, 1);
        sjs.dsp.op(hc.RING_ADDR_RX_FFT      , "set", 10, 1);
        sjs.dsp.op(hc.RING_ADDR_RX_FINE_SYNC, "set", 10, 1);
        sjs.dsp.op(hc.RING_ADDR_RX_EQ,        "set", 10, 1);
        sjs.dsp.op(hc.RING_ADDR_RX_MOVER,     "set", 10, 1);



      }

      if( this.wakes === 2) {
        this.changeTimer(4000);
        sjs.ringbus(hc.RING_ADDR_RX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
        sjs.ringbus(hc.RING_ADDR_RX_1,  hc.COOKED_DATA_TYPE_CMD | 2);
        sjs.ringbus(hc.RING_ADDR_RX_2,  hc.COOKED_DATA_TYPE_CMD | 2);
        sjs.ringbus(hc.RING_ADDR_RX_4,  hc.COOKED_DATA_TYPE_CMD | 2);
      }

      if( this.wakes === 3) {
        this.changeTimer(2000);
        sjs.txSchedule.debug_duplex = 4000;
      }


      if( this.wakes >= 4) {
        const choose_gap = this.rng.nextInt(1500, 24000); // in frames
        const choose_time = this.rng.nextInt(2000, 6000);  // in ms
        console.log("Choosing gap " + choose_gap + " choosing run time of " + (choose_time / 1000.0) );
        sjs.txSchedule.debug_extra_gap = choose_gap;
        this.changeTimer(choose_time);
        // sjs.test_fb();

        // fixme check cs11 counter



        // if( this.rng.nextInt(0, 1024) < 90 ) {
        //   const new_speed = this.rng.nextInt(50, 444);
        //   console.log("changing speed to " + new_speed);
        //   this.changeTimer(new_speed);
        // }
      }
    }
    this.on_exit[4] = () => {
      sjs.txSchedule.debug_duplex = 1;
    };

  }

  periodic() {
    // console.log(this.work);
    // console.log(this.mode);

    if( this.automatic_exit_with_error && this.will_auto_exit ) {
      this.exit();
      return;
    } else {
      this.work[this.mode]();
    }


    this.wakes++;
  }

  changeTimer(intv) {
    this.killTimer();
    this.beginTimer(intv);
  }

  beginTimer(intv=500) {
    this.tmr = setInterval(()=>{this.periodic()},intv);
  }

  killTimer() {
    if(this.tmr !== null) {
      clearInterval(this.tmr);
      this.tmr = null;
    }
  }

  pickRng(forced = null) {

    console.log("-------------------------");
    if( forced != null ) {
      this.rngSeed = forced;
      console.log("Using passed seed 0x" + this.rngSeed.toString(16));
    } else {
      this.rngSeed = parseInt(Math.random() * 0xffffffff);
      console.log("Using Random seed 0x" + this.rngSeed.toString(16));
    }
    console.log("-------------------------");

    this.rngHistory.push(this.rngSeed);

    this.rng = new Prando(this.rngSeed);
  }

  torture(mode) {
    if( mode != 0 && this.mode != 0 ) {
      console.log("You are already in mode " + this.mode + " you must exit first");
      return;
    }
    const old_mode_copy = this.mode;

    this.mode = mode;

    if( mode ) {

      // this is like a "Begin callback"
      this.pickRng();
      this.wakes = 0;
      this.error_count = 0;
      this.automatic_exit_with_error = true;
      this.will_auto_exit = false;
      this.beginTimer();

    } else {
      this.dispatchOnExit(old_mode_copy);
      this.exitStats(old_mode_copy);
      this.killTimer();
    }

  }


  exitStats(old_mode_copy) {
    if( old_mode_copy === 0 )  {
      console.log("Torture test was not running");
    } else {
      console.log("-------------------------");
      console.log("exiting from: " + old_mode_copy );
      console.log("-------------------------");
    }
  }

  gotUdError(w) {

    let serious_error = this.sjs.getErrorFeedbackBusParseCritical(hc.TX_USERDATA_ERROR | w);

    // console.log("serious error: " + serious_error);

    if( this.mode && serious_error == 3 ) {
      console.log("Torture got error\n");
      this.error_count ++;
      this.will_auto_exit = true;
    }
  }


  setupCatch() {
    this.sjs.hrb.on(hc.TX_USERDATA_ERROR, (x)=>{this.gotUdError(x)});
  }

}

module.exports = {
    TortureFeedbackBus
}
