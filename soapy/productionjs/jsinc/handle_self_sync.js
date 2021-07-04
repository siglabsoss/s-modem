'use strict';

const hc = require('../../js/mock/hconst.js');

///
/// This Class handles waking up all the FPGA's after a bootload
/// The new behavior is that the bootload enteres the board in a "sleeping" state
/// We use a board-wide shared counter to wake up deterministiaclly from a starting point
///
/// Usage:
///   call the onDone() with a callback
///   call run() to start
///
class HandleSelfSync {
  constructor(sjs) {
    this.sjs = sjs;

    this.sjs.hrb.on(hc.CHECK_BOOTLOAD_PCCMD, this.gotRbOne.bind(this));

    this.onDoneCb = null;
    this.checkBootLoadPending = false;

    this.syncPending = 0;
    this.timerStorage = {};

    this.sjs.hrb.on(hc.TIMER_RESULT_PCCMD, this.gotTimers.bind(this));

    // this number should be hand tuned to put us on the edge of the fft / cp
    // See TestSelfCoarseSync.md to tune this value
    this.txVsRxChainAdjustment = 473;

    // this number is our tolerance into the cp. It should be as close to 0
    // as the riscv code wakeup allows us
    // This is NOT `coarse.tweak`
    this.txVsRxChainAdjustment -= 8;

    // HARD SET TO ZERO, see TestSelfCoarseSync.md
    // this.txVsRxChainAdjustment = 0;
  }


  /// we've asked ONE fpga if it has been released from it's self-sync boot lock
  /// it's replied
  ///
  /// next function is selfSync() in the ideal case
  ///
  gotRbOne(word) {
    const syncDone = (word & 0x00000f00)>>>8;
    const who      = (word & 0x000000ff)>>>0;

    if( !this.checkBootLoadPending ) {
      console.log("HandleSelfSync.gotRbOne doing nothing");
      return;
    }
    this.checkBootLoadPending = false;

    if( who !== 2 ) {
      console.log("HandleSelfSync.gotRbOne did not understand 0x" + word.toString(16));
      return;
    }

    console.log("HandleSelfSync.gotRbOne got word " + word.toString(16) + "    " + syncDone);

    if( syncDone ) {
      console.log("HandleSelfSync was already performed, continuing");
      if( this.onDoneCb ) {
        this.onDoneCb();
      } else {
        console.log("HandleSelfSync CALLBACK WAS NOT SET.  Your javascript is broken!");
      }
    } else {
      console.log("HandleSelfSync needs to unblock Higgs");
      this.selfSync();
    }
  }

  ///////

  /// This gets called by gotTimers()
  ///
  /// The next function called is: performFinalCorrection()
  ///
  /// Seems like riscv code can only handle timers where the value + then will not overflow
  /// if we detect a timer above a thresh, we just call selfSync() later
  ///
  ///
  performSelfSync(timer) {
    console.log('performSelfSync: ' + timer.toString(16));

    const timerLockout = 0xf4000000;
    // const timerLockout = 0x10000000; // debug value to forst multiple retrys

    if( timer > timerLockout ) {
      console.log("performSelfSync got a timer larger than " + timerLockout.toString(16) + " PLEASE WAIT!");
      setTimeout(this.selfSync.bind(this), 800);
      return;
    }

    let deltaMs = 150;
    let delta = parseInt((deltaMs/1000.0) * 125E6);

    let wakeup = timer + delta;

    console.log('performSelfSync wakeup:    ' + wakeup.toString(16));

    let wakeupRb = (wakeup>>>8);

    this.sjs.ringbus(hc.RING_ADDR_CS01, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS02, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS11, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS12, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS20, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS21, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS22, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS31, hc.SELF_SYNC_CMD | wakeupRb );
    this.sjs.ringbus(hc.RING_ADDR_CS32, hc.SELF_SYNC_CMD | wakeupRb );

    setTimeout(()=>{
      this.performFinalCorrection(this.txVsRxChainAdjustment)
    }, 2000);

    // sjs.ringbus(hc.RING_ADDR_CS11, hc.SELF_SYNC_CMD | wakeupRb );
    // sjs.ringbus(hc.RING_ADDR_CS31, hc.SELF_SYNC_CMD | wakeupRb );

  };

  /// The next function called is: 
  ///
  ///  setPartnerSfoAdvance()
  ///  setPartnerSfoAdvance()
  ///  ...
  ///  the user's callback
  /// 
  /// We do a loop and fire off a bunch of setTimeout()'s
  ///
  performFinalCorrection(amount) {

    console.log("entering performFinalCorrection()");

    let who = this.sjs.id;

    let bumpMs = 300;

    let limit = 128;

    let acc = 0;

    let curMs = 0;

    while(acc < amount) {
      let thisOne = Math.min(limit,amount-acc);

      // if( (amount-limit) > 0 ) {
      // amount -= thisOne;
      // }

      setTimeout(()=>{
        console.log("run: " + thisOne);
        this.sjs.dsp.setPartnerSfoAdvance(who, thisOne, 4, false);
      },curMs);

      curMs += bumpMs;
      acc += thisOne;
    }

    setTimeout(()=>{
      if( this.onDoneCb ) {
        this.onDoneCb();
      }
    }, curMs);

  }

  /// the next function to run is: performSelfSync()
  /// This function gets called two times with timer values
  ///
  /// we only move forward if we've already set our own member this.syncPending
  /// this prevents errant runs of performSelfSync() from rouge ringbus
  gotTimers(word) {
      let who   = (word & 0x00f00000)>>>20;
      let upper = (word & 0x000f0000)>>>16;
      let data  = (word & 0x0000ffff)>>>0;

      if( upper ) {
        let res = (this.timerStorage[who] | (data<<16)) >>> 0;
        console.log("res:                                         " + res.toString(16));
        // this prevents us from running this code if we get this ringbus at a later date
        if( this.syncPending ) {
          this.syncPending = 0;
          this.performSelfSync(res);
        }
      } else {
        this.timerStorage[who] = (data) >>> 0;
        // console.log(who.toString(16) + ' storing ' + this.timerStorage[who].toString(16));
      }

    // console.log("timer: " + word.toString(16));
  }

  onDone(cb) {
    this.onDoneCb = cb;
  }

  /// Initial entry point
  /// 
  /// the next function to run is: gotRbOne() 
  ///
  run() {
    console.log('HandleSelfSync will check board');
    this.checkBootLoadPending = true;
    this.sjs.ringbus(hc.RING_ADDR_CS11, hc.CHECK_BOOTLOAD_CMD | 2);
  }


  /// the next function to run is: gotTimers
  /// 
  selfSync() {
    // this allows a check in gotTimers to continue, without this, gotTimers does nothing
    this.syncPending = 1;
    this.sjs.ringbus(hc.RING_ADDR_CS11, hc.GET_TIMER_CMD);
    // sjs.ringbus(hc.RING_ADDR_CS31, hc.GET_TIMER_CMD);
  }

}


module.exports = {
    HandleSelfSync
}
