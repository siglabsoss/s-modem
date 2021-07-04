'use strict'

const mutil = require('../../js/mock/mockutils.js');
const hc = require('../../js/mock/hconst.js');
const schedule = require('./schedule.js');

class ArbBehavior {
  constructor(sjs, zmq, options={}) {
    this.sjs = sjs;
    this.zmq = zmq;

    const {attachedMode=false, singleMode=true, startDelay=1500} = options;
    this.attachedMode = attachedMode;
    this.singleMode = singleMode;
    this.startDelay = startDelay;



    this.name = 'ArbBehavior';

    if( this.attachedMode ) {
      this.txp = [sjs.id];
    } else {
      this.txp = sjs.txPeers();
    }

    console.log(this.txp);

    this.setup();

    this.setupAir();

    this.counters = [];
    this.countersAt = [];
    this.arb_seq = 0;

    this.tun_packets = 0;

    this.allow_tx = true;

    this.setupAck();

    this.word_size = 40000;

    // this.singleMode = true;

    this.ts = 12;

    this.track = [];

    this.print_tx_time_updates = false;

    this.group_sent = 0;
    this.group_found = 0;

    this.g = {};
    this.g.start = 7;
    this.g.end = 50;
    this.g.step = 10;
    this.g.size = 1000;
  }

  gotJointAck(header, body) {
    let from = body[0];
    let packet_epoc = body[1];
    let packet_ts = body[2];
    let seq = body[3];
    let rx_epoc = body[4];
    let rx_frame = body[5];

    if( rx_frame > 1500 ) {
      console.log('gotJointAck was late:');
      console.log(body);
    }
  }

  setupAck() {
    this.zmq.notify.on(''+hc.FEEDBACK_VEC_JOINT_ACK, this.gotJointAck.bind(this));
    this.sjs.dsp.zmqCallbackCatchType(hc.FEEDBACK_VEC_JOINT_ACK);

    
    this.zmq.notify.on(''+hc.FEEDBACK_VEC_WORDS_CORRECT, this.gotDebugVec.bind(this));
    this.sjs.dsp.zmqCallbackCatchType(hc.FEEDBACK_VEC_WORDS_CORRECT);
  }

  setupAir() {
    this.sjs.airTx.set_modulation_schema(2);//hc.FEEDBACK_MAPMOV_QAM16);
    this.sjs.airTx.set_subcarrier_allocation(7);//hc.MAPMOV_SUBCARRIER_320);

    let code_length = 255;
    let fec_length = 32;

    //hc.AIRPACKET_CODE_REED_SOLOMON
    let code_bad = this.sjs.airTx.set_code_type(1, code_length, fec_length);

    if( code_bad != 0 ) {
      console.log('code_bad: ' + code_bad);
    }
  }


  watchTunSize() {
    this.tun_packets = this.sjs.txTunBufferLength();

    if( this.tun_packets != 0 ) {
      console.log('arb tun has ' + this.tun_packets);
    }
  }

  watchCountersAttached() {

    // only 1 radio, it's attached
    let p = this.txp[0];

    let now = Date.now();
    this.countersAt[p] = now;
    this.counters[p] = this.sjs.attached.predictScheduleFrame();
    if(this.print_tx_time_updates) {
      console.log('attached radio id ' + ' got ' + x);
    }
  }

  watchCounters() {
    let cmd = 'sjs.attached.predictScheduleFrame()';
    // this.zmq.id(1).eval('1').then((x)=>{ console.log('found id ' + x); });

    for( let p of this.txp ) {

      let gotRes = (x) => {
        let now = Date.now();
        this.countersAt[p] = now;
        this.counters[p] = x;
        if(this.print_tx_time_updates) {
          console.log('radio id ' + p + ' got ' + x);
        }
      }

      // console.log("p " + p);
      // console.log(this.zmq);
      this.zmq.id(p).eval(cmd).then(gotRes);
      // this.zmq.id(2).eval('sjs.attached.predictScheduleFrame()').then(console.log);
    }

    this.watchTunSize();
  }

  predictTxFrame(p) {
    let now = Date.now();

    if( this.countersAt[p] && this.counters[p] ) {
    } else {
      console.log('predictTxFrame() missing information');
      return;
    }

    let elapsed_time = now - this.countersAt[p];

    let frames_since_est = (elapsed_time / 1E3) * hc.SCHEDULE_FRAMES_PER_SECOND;

    // console.log(frames_since_est);

    // console.log(this.counters[p]);

    let res = schedule.add_frames_to_schedule(this.counters[p], frames_since_est);
    // console.log(res);

    return res;
  }


  /// This function tries to wake up when the transmitter is at frame 0
  /// Every time we wakeup, we use predictTxFrame() as our best guess of when it is
  /// for the transmitter
  /// We adjust our next setTimeout() so that we will wakeup near frame 0
  /// if we wake up > 24000, we simply busy wait until it's time to transmit
  /// if we wakeup < 100, we call it good and go
  groupTriggerAdjust() {
    let ok = true;

    // for every active peer, we try to falsify this flag by checking member of
    // this.counters
    for( let p of this.txp ) {
      // console.log('p ' + p + ' ok ' + ok);
      ok = !this.counters[p] ? false : ok;
    }

    if(!ok) {
      setTimeout(this.groupTriggerAdjust.bind(this), 1000);
      return;
    }

    // only use one radio for now.....
    let onep = this.txp[0];

    let est = this.predictTxFrame(onep);
    // console.log(est);

    let frame = est[1];

    let delta = (hc.SCHEDULE_FRAMES - frame) / hc.SCHEDULE_FRAMES;

    // console.log('delta ' + delta);

    let fudge = 0.14453 * 1000;

    setTimeout(this.groupTriggerAdjust.bind(this), 3000+(delta*1000)+fudge);

    if( est[1] > 24000 ) {
      let est2;
      do {
        est2 = this.predictTxFrame(onep);
      } while(est2[1] >= 100 );
      // console.log('  ' + est2);
      this.groupTrigger();
    } else if( est[1] < 100 ) {
      this.groupTrigger();
    } else {
      console.log('groupTriggerAdjust() skipping this time');
    }

  }

  eraseTrack() {
    this.track = [];
  }

  groupTrigger() {
    let p0 = this.txp[0];
    let p1 = this.txp[1];

    if( !this.singleMode ) {
      if( this.counters[p1] ) {
      } else {
        console.log('counter not set in groupTrigger()');
        return;
      }
    }

    let sz = this.g.size;

    let epoc = this.getCounter() + 2;



    for(let i = this.g.start; i < this.g.end; i += this.g.step) {
      // console.log('schedule ' + epoc + ' ' + i);
      let seq = this.trigger3(epoc, i, sz);

      let [_bits, _sc_worth, _frames] = this.sjs.airTx.getSizeInfo(sz);

      // console.log('saving ' + seq);
      this.sentDebugVec(seq, sz, i, _frames);
    }

    this.sjs.dsp.dumpTunBuffer();
  }

  sentDebugVec(seq, len, ts, frames) {

    // search if existing sequence number is already there
    for(let i = 0; i < this.track.length; i++ ) {
      let [_seq,...junk] = this.track[i];
      if( _seq === seq ) {
        this.track.splice(i,1); // this will delete the ith element
        console.log('sentDebugVec() for seq ' + seq + ' erasing existing entry in track');
        break;
      }
    }


    let entry = [seq, Date.now(), ts, frames];
    this.track.push(entry);
    this.group_sent++;
  }

  gotDebugVec(header, body) {
    // console.log('gotDebugVec()');
    // console.log(body);
    let from = body[0];
    let ok = body[1];
    let at = body[2];
    let len = body[3];
    let seq = body[4];
    let flags = body[5];

    let now = Date.now();

    let found = false;

    for(let i = 0; i < this.track.length; i++ ) {
      let [_seq,_date,_ts,_frames] = this.track[i];
      if( _seq === seq ) {
        let m = (''+seq).padEnd(4) + '   ' + (''+_ts).padEnd(4) + '   ' + (''+_frames).padEnd(6) + '             ';
        console.log('found ' + m + this.groupStats());
        this.track.splice(i,1); // this will delete the ith element
        // console.log(this.track);
        found = true;
        break;
      }
    }

    if(!found) {
      console.log('gotDebugVec() was called with ' + seq + ' but this.track did not find');
      console.log(this.track);
    } else {
      this.group_found++;
    }
  }

  printSroupStats() {
    console.log(this.groupStats());
  }

  groupStats() {
    let r = this.group_found / this.group_sent;
    let s = "Total: " + this.group_found + '/' + this.group_sent + " and got " + r*100 + "%";
    return s;
  }



  setup() {
    setTimeout( ()=>{

      if( this.attachedMode ) {
        this.handle = setInterval(this.watchCountersAttached.bind(this), 1000);
      } else {
        this.handle = setInterval(this.watchCounters.bind(this), 1000);
      }

    }, this.startDelay);


    setTimeout( ()=>{
      setTimeout(this.groupTriggerAdjust.bind(this), 3000);
    //   this.handle2 = setInterval(this.groupTrigger.bind(this), 3000);
    }, this.startDelay);

    // setTimeout( ()=>{
    //   this.handle3 = setInterval(this.predictTxFrame.bind(this), 333);
    // }, 3000);


  }

  getCounter() {

    let p0 = this.txp[0];
    let p1 = this.txp[1];

    let ca;
    let cb;
    if( this.singleMode ) {
      ca = this.counters[p0][0];
      return ca;
    } else {
      let ca = this.counters[p0][0];
      let cb = this.counters[p1][0];
    }
    return Math.max(ca,cb);
  }


  trigger() {

    let epoc_go = this.getCounter() + 1;

    let res = this.sjs.dsp.scheduleJointFromTunData(this.txp, this.arb_seq, this.word_size, epoc_go, this.ts);

    console.log(res);

    this.arb_seq = (this.arb_seq+1) & 0xff;
  }

  trigger3(epoc_go, ts, sz) {
    let seq = this.arb_seq;
    this.sjs.dsp.debugJoint(this.txp, seq, sz, epoc_go, ts);
    this.arb_seq = (this.arb_seq+1) & 0xff;
    return seq;
  }

  trigger2() {
    console.log(this.counters[1]);
    console.log(this.counters[2]);

    let ca = this.counters[1][0];
    let cb = this.counters[2][0];

    let epoc_go = Math.max(ca,cb) + 1;

    this.sjs.dsp.debugJoint(this.txp, this.arb_seq, 20000, epoc_go, 12);

    this.arb_seq = (this.arb_seq+1) & 0xff;
  }
  

}

module.exports = {
    ArbBehavior
}
