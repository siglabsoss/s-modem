'use strict';
const StateMachine = require('javascript-state-machine');
const hc = require('../../js/mock/hconst.js');
const mutil = require('../../js/mock/mockutils.js');

class RadioEstimate {
  constructor(sjs, idx, notify, options={}) {

    const {printTransitions = false, printGraphviz = true, disable = false} = options;

    this.printTransitions = printTransitions;
    this.printGraphviz = printGraphviz;
    this.disable = disable;

    // Any this.xxx you add here must be mirrord down in setupFsm
    this.sjs = sjs;
    this.idx = idx;
    this.notify = notify;
    this.r = sjs.r[idx];

    this.bindNative();

    let msg = "JS RadioEstimate " + this.idx + " is ";
    if(this.disable) {
      console.log(msg + "disabled");
      return;
    }
    console.log(msg + "enabled");

    this.boot();
    this.setupGoAfter();
    this.setupFsm();
    this.setupEvents();


    // force bad transaction on purpose
    // this.goAfter('transitionTdma0', 3000);

  }

  boot() {
    console.log("r" + this.r.array_index + " js wrapper boot");
  }

  bindNative() {
    console.log("Bind Native on rjs" + this.idx);
    this.sjs.r[this.idx].getAllScBlocking = this.getAllScBlockingBound.bind(this);
  }

  getAllScBlockingBound() {
    this.r.save_next_all_sc = true;
    while(this.r.save_next_all_sc) {
      // FIXME: rewrite this as non blocking if we are going to use in runtime
      // console.log('blocking');
    }
    let asBuffer = this.r.getAllScSaved();
    let ishort = mutil.streamableToIShort(asBuffer);
    // console.log(ishort);
    return ishort;
  }


  setupEvents() {

    // filter events for only us
    let gotFineSyncRequest = (x, y) => {
      if( y == this.idx ) {
        console.log('rjs' + this.idx + ' gotFineSyncRequest ' + y);
        this.fsm.gotEvent(x, y);
      }
    };

    let gotTdmaRequest = (x, y) => {
      if( y == this.idx ) {
        console.log('rjs' + this.idx + ' got REQUEST_TDMA_EV ' + y);
        this.fsm.gotEvent(x, y);
      }
    };

    this.notify.on(hc.REQUEST_FINE_SYNC_EV, gotFineSyncRequest);
    this.notify.on(hc.REQUEST_TDMA_EV, gotTdmaRequest);
  }

  setupGoAfter() {
    this.goAfterCounter = 0;
    this.pendingGoCalls = {};
  }

  getGoAfterHandle() {
    let handle = this.goAfterCounter;
    this.goAfterCounter++;
    return handle;
  }

  goAfterFinished(idx) {
    delete this.pendingGoCalls[idx];
  }

  insertGoAfterHandle(handle, idx) {
    this.pendingGoCalls[idx] = handle;
  }

  clearGoAfter() {
    for(let idx in this.pendingGoCalls) {
      let handle = this.pendingGoCalls[idx];
      clearTimeout(handle);
    }
    this.pendingGoCalls = {};
  }

  goAfter(state, time) {
    let later = (h) => {
      // console.log('goAfter fire ' + state);
      try {
        this.fsm[state]();
      } catch(err) {
        console.log('');
        console.log('=============================================================');
        console.log('== State Machine Exception ==================================');
        console.log(err);
        console.log('=============================================================');
      }
      this.goAfterFinished(h);
    };
    let idx = this.getGoAfterHandle();
    let handle = setTimeout(later, time, idx);
    this.insertGoAfterHandle(handle, idx);
  }

  goNow(state) {
    this.goAfter(state, 0);
  }



  setupFsm() {

    // define these "sugar" accessors, so we don't need to be wordy below
    const r = this.r;
    const goAfter = this.goAfter.bind(this);
    const goNow = this.goNow.bind(this);
    const clearGoAfter = this.clearGoAfter.bind(this);
    const sjs = this.sjs;
    const dsp = this.sjs.dsp;
    const peer_id = this.r.peer_id;

    // do we need to keep this object for later?
    const fsmOptions = {
      init: 'boot'
      ,transitions: [
        {  name: 'gotFineSyncRequest',       from: 'boot',                          to: 'preSfo' }
        ,{ name: 'transitionSfo',            from: 'preSfo',                        to: 'sfo' }
        ,{ name: 'transitionHotSfo',         from: 'preSfo',                        to: 'hotSfo' }
        ,{ name: 'enterSfo',                 from: ['preSfo','sfo'],                to: 'sfo' }
        ,{ name: 'transitionSto0',           from: ['sfo','sto0','hotSfo'],         to: 'sto0' }
        ,{ name: 'transitionStoEq0',         from: 'sto0',                          to: 'stoEq0' }
        ,{ name: 'transitionCfo0',           from: ['stoEq0','cfo0'],               to: 'cfo0' }
        ,{ name: 'transitionHotCfo',         from: ['stoEq0'],                      to: 'hotCfo' }
        ,{ name: 'transitionBackgroundSync', from: ['cfo0','hotCfo'],               to: 'backgroundSync' }
        ,{ name: 'transitionWaitEvents',     from: ['backgroundSync',
                                                            'waitEvents','tdma3'],  to: 'waitEvents' }
        ,{ name: 'gotTdmaMessage',           from: ['waitEvents','tdma0'],          to: 'tdmaRequested' }
        ,{ name: 'transitionTdma0',          from: ['tdmaRequested','tdma0'],       to: 'tdma0' }
        ,{ name: 'tdma0Timeout',             from: ['tdma0'],                       to: 'tdma0' }
        ,{ name: 'transitionTdma1',          from: ['tdma0','tdma1','tdma2x6'],     to: 'tdma1' }
        ,{ name: 'transitionTdma2',          from: ['tdma1'],                       to: 'tdma2' }
        ,{ name: 'transitionTdma2x5',        from: ['tdma2'],                       to: 'tdma2x5' }
        ,{ name: 'transitionTdma2x6',        from: ['tdma2x5','tdma2x6'],           to: 'tdma2x6' }
        ,{ name: 'transitionTdma2x7',        from: ['tdma2x6'],                     to: 'tdma2x7' }
        ,{ name: 'transitionTdma3',          from: ['tdma2x7','tdma3'],             to: 'tdma3' }
      ]
      ,data: {
        sjs: this.sjs
        ,idx: this.idx
        ,notify: this.notify
        ,r: this.r
        // ,goAfter: this.goAfter.bind(this)
        // ,goNow: this.goNow.bind(this)
      }
      ,methods: {
        onInit1:     function() { console.log('I init1')     }
        ,onGotFineSyncRequest: function() {
          console.log('onGotFineSyncRequest');
          if( sjs.settings.getBoolDefault(false, 'runtime.dsp.hotstart') ) {
            // let sfo = sjs.settings.getDoubleDefault(0, 'db.history.dsp.sfo.peers.t0');
            // console.log("will hotstart with " + sfo);
            goAfter('transitionHotSfo', 500);
          } else {
            goAfter('transitionSfo', 1000);
          }
        }
        ,onTransitionSfo: function() {
          r._prev_sfo_est = r.times_sfo_estimated;
          // do not use this, we are already on our way to this state
          // goAfter('enterSfo', 1);
        }
        ,onHotSfo: function() {
          let sfo = sjs.settings.getDoubleDefault(0, 'db.history.dsp.sfo.peers.t0');
          console.log("will hotstart SFO with " + sfo);
          r.sfo_estimated = sfo;
          r.sfoState();
          goAfter('transitionSto0', 1);
        }
        ,onEnterSfo: function() {
          console.log('onEnterSfo');
          let SFO_ESTIMATE_WAIT = sjs.settings.getIntDefault(0, 'dsp.sfo.sfo_estimate_wait');

          if((r.times_sfo_estimated - r._prev_sfo_est) == SFO_ESTIMATE_WAIT) {
            console.log('rjs' + this.idx + " SFO_STATE_0 counter matched")
            let failure = r.sfoState();
            if(!failure) {
              goAfter('transitionSto0', 1);
              console.log('rjs' + this.idx + ' Exiting SFO_STATE_0');
              r._prev_sto_est = r.times_sto_estimated;
            } else {
              goAfter('enterSfo', 1);
            }
            r._prev_sfo_est = r.times_sfo_estimated;
          } else {
            goAfter('enterSfo', 1000);
          }
        }

        ,onTransitionSto0: function() {
          console.log('onTransitionSto0');
          if((r.times_sto_estimated - r._prev_sto_est) === 2) {
            console.log('onTransitionSto0 MATCHED');
            r.stoState();

            // note we don't check counter for this jump
            goAfter('transitionStoEq0', 1000);
            console.log('rjs' + this.idx + " Exiting STO_0");
          } else {
            // console.log(this.state + ' looping back to ' + 'transitionSto0');
            console.log('rjs' + this.idx + " Skipping update of STO: " + (r.sto_estimated/256.0) );
            goAfter('transitionSto0', 250);
          }
        }

        ,onTransitionStoEq0: function() {
          console.log('onTransitionStoEq0');

          // applies one time eq according to margin
          r.dspRunStoEq(1.0);

          // setting 2nd to true sort of skips logic due to
          // times_eq_sent. fixme re-look at this logic
          r.updatePartnerEq(false, false);

          r._prev_cfo_est = r.times_cfo_estimated;

          if( false && sjs.settings.getBoolDefault(false, 'runtime.dsp.hotstart') ) {
            goAfter('transitionHotCfo',1);
          } else {
            goAfter('transitionCfo0', 4000);
          }
          // console.log('ssssssssssssssssssssssssssssssstall');
        }

        ,onHotCfo: function() {
          let cfo = sjs.settings.getDoubleDefault(0, 'db.history.dsp.cfo.peers.t0');
          console.log("will hotstart CFO with " + cfo);
          r.cfo_estimated = cfo;
          r.cfoState();
          goAfter('transitionBackgroundSync', 250);
        }

        ,onTransitionCfo0: function() {
          if((r.times_cfo_estimated - r._prev_cfo_est) === 2) {
            console.log('rjs' + this.idx + " CFO_0 counter matched");
            let failure = r.cfoState();
            if(!failure) {
                console.log('rjs' + this.idx + " Exiting CFO_0");
                goAfter('transitionBackgroundSync', 1);
            } else {
                goAfter('transitionCfo0', 1);
            }
            r._prev_cfo_est = r.times_cfo_estimated;
          } else {
            goAfter('transitionCfo0', 250);
          }
        }

        ,onTransitionBackgroundSync: function() {
          console.log('rjs' + this.idx + " above startBackground()");
          r.startBackground();

          sjs.db.set('history.dsp.cfo.peers.t'+this.idx, r.cfo_estimated_sent).write();
          sjs.db.set('history.dsp.sfo.peers.t'+this.idx, r.sfo_estimated_sent).write();

          sjs.customEvent.send(hc.RADIO_ENTERED_BG_EV, this.idx);

          goNow('transitionWaitEvents');
        }

        ,onTransitionWaitEvents: function() {
          console.log('rjs' + this.idx + " onTransitionWaitEvents");
        }

        ,onWaitEvents: function() {
          console.log('rjs' + this.idx + " onWaitEvents");

          // goAfter('transitionWaitEvents',1000);
        }

        ,onGotTdmaMessage: function() {
          console.log('rjs' + this.idx + " entered GOT_TDMA_REQUEST");
          r.td.reset();
          r.resetDemodPhase();
          console.log('rjs' + this.idx + " after reset resetDemodPhase() and td.reset()");

          sjs.dsp.setPartnerTDMA(peer_id, 6, 0);
          r.track_demod_against_rx_counter = true;

          r.times_wait_tdma_state_0 = 0;

          goAfter('transitionTdma0', 1);
        }

        ,onTransitionTdma0: function() {
          console.log('rjs' + this.idx + " TDMA_STATE_0 " + r.times_wait_tdma_state_0);

          if(r.td.found_dead) {
            console.log('rjs' + this.idx + " found dead");
            sjs.dsp.setPartnerTDMA(peer_id, 4, 0);

            r.td.sent_tdma = true;
            r.td.lifetime_rx = 0;
            r.td.lifetime_tx = 0;
            r.td.reset();
            r.resetDemodPhase();

            goAfter('transitionTdma1', 1);


            
          } else if (r.times_wait_tdma_state_0 > ((1000/5)*30)) {
            console.log('rjs' + this.idx + " TDMA_STATE_0 timed out, going back to GOT_TDMA_REQUEST");
            goAfter('onGotTdmaMessage', 1);
          } else {
            goAfter('transitionTdma0', 5);
          }
          r.times_wait_tdma_state_0++;
        }


        ,onTransitionTdma1: function() {
          if(r.td.lifetime_rx !== 0 || r.td.lifetime_tx !== 0) {
            console.log('rjs' + this.idx + ` tx: ${r.td.lifetime_tx} rx: ${r.td.lifetime_rx}`);
            r.tdma_phase = -1;
            goAfter('transitionTdma2', 1);
          } else {
            goAfter('transitionTdma1', 1000);
          }
        }


        ,onTransitionTdma2: function() {
          console.log('rjs' + this.idx + " sending tdma mode 4 again to double check alignment");
          dsp.setPartnerTDMA(peer_id, 4, 0);
          r.resetTdmaAndLifetime();
          goAfter('transitionTdma2x5', 2000);
        }

        ,onTransitionTdma2x5: function() {
          console.log('rjs' + this.idx + " TDMA_STATE_2_DOT_5");
          dsp.setPartnerTDMA(peer_id, 6, 0);
          goAfter('transitionTdma2x6', 3000);
        }

        // ,onTransitionTdma2x55: function() {
        //   console.log('rjs' + this.idx + " TDMA_STATE_2_DOT_55");
        //   r.tdma_phase = -1;
        //   r.td.needs_fudge = false;
        //   dsp.setPartnerTDMA(peer_id, 6, 0);
        //   goAfter('transitionTdma2x6', 3000);
        // }

        ,onTransitionTdma2x6: function() {
          console.log('rjs' + this.idx + " TDMA_STATE_2_DOT_6");
          if( r.td.needs_fudge == false ) {
            console.log('rjs' + this.idx + " did not find TDMA mode 6");
            goAfter('transitionTdma1', 1);
          } else {
            r.alignSchedule3();
            goAfter('transitionTdma2x7', 1000);
          }
        }

        ,onTransitionTdma2x7: function() {
          console.log('rjs' + this.idx + " TDMA_STATE_2_DOT_7 reset");
          r.tdma_phase = -1;
          goAfter('transitionTdma3', 3000);
        }

        ,onTransitionTdma3: function() {
          console.log('rjs' + this.idx + " TDMA_STATE_3");
          if(r.td.lifetime_rx !== 0 || r.td.lifetime_tx !== 0) {
            console.log('rjs' + this.idx + ` Final tx: ${r.td.lifetime_tx} rx: ${r.td.lifetime_rx}`);
            console.log('rjs' + this.idx + ` Final tx mod: ${r.td.lifetime_tx % hc.SCHEDULE_FRAMES} rx mod : ${r.td.lifetime_rx % hc.SCHEDULE_FRAMES}`);

            console.log('rjs' + this.idx + " going to WAIT_EVENTS");
            goAfter('transitionWaitEvents', 1);

            dsp.setPartnerTDMA(peer_id, 20, 0);

            sjs.customEvent.send(hc.FINISHED_TDMA_EV, this.idx);

            r.setTdmaSyncFinished(true);
          } else {
            goAfter('transitionTdma3', 1000);
          }
        }



        // x is the custom event type, y is the index of the desired radio
        // this function is only called if y is our index, no need to check here
        ,gotEvent:   function(x, y) {
          // within this context, `this` is the fsm
          console.log('fsm got event ' + x + ' ' + y);
          // console.log(this);
           
          switch(x) {
            case hc.REQUEST_FINE_SYNC_EV:
              // setTimeout(this.gotFineSyncRequest.bind(this), 500);
              this.gotFineSyncRequest();
              // setTimeout(this.enterSfo.bind(this), 1500);
              break;
            case hc.REQUEST_TDMA_EV:
              this.gotTdmaMessage();
              break;
            default:
              console.log('rjs' + this.idx + ' got unknown event ' + x);
              break;
          }
        }
      }
  };
  if(this.printTransitions) {
    fsmOptions.methods.onAfterTransition = function(lifecycle, arg0, arg1) {
      console.log('//////////////////////////////////////////////////////////////////');
      console.log('// ' + lifecycle.from.padEnd(15) + '->' + lifecycle.to.padStart(15) + '    (' + lifecycle.transition + ')'); // 'step'
      // console.log(lifecycle.transition); // 'step'
      // console.log(lifecycle.from);       // 'A'
      // console.log(lifecycle.to);         // 'B'
    };
  }

  this.fsm = new StateMachine(fsmOptions);


  // paste output into
  //   https://dreampuf.github.io/GraphvizOnline/
  if( this.printGraphviz ) {
    const visualize = require('javascript-state-machine/lib/visualize');
    let dotCode = visualize(this.fsm);
    console.log('');
    console.log('------------------------------------------------------------------');
    console.log(dotCode);
    console.log('------------------------------------------------------------------');
    console.log('');
  }


  }
}

module.exports = {
  RadioEstimate
}
