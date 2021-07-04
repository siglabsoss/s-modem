'use strict'

const mutil = require('../../js/mock/mockutils.js');
const hc = require('../../js/mock/hconst.js');
const schedule = require('./schedule.js');
const scheduleFile = require('./fixed_schedule.js');

class ArbBehavior {
  constructor(sjs, zmq) {
    this.sjs = sjs;
    this.zmq = zmq;
    this.name = 'ArbBehavior';

    this.txp = sjs.txPeers();

    console.log(this.txp);

    this.epocWakeTime = 3000;
    this.use_object = true;

    if(this.use_object) {
      this.epocWakeTime = 3000;
    }

    this.setupTimers();

    this.setupAir();

    this.setupSchedule();

    this.counters = [];
    this.countersAt = [];
    this.arb_seq = 0;

    this.tun_packets = 0;

    this.allow_tx = true;

    this.setupAck();

    this.word_size = 40000;

    this.single_mode = true;

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
    let dropped = body[6];

    if( dropped ) {
      console.log('peer ' + from + ' dropped packet seq ' + seq);
    }

    if( rx_frame > 1500 ) {
      // console.log('gotJointAck was late:');
      // console.log(body);
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

  setupSchedule() {

    const onep = this.txp[0];

    // let est = this.predictTxFrame(onep);

    const api = {
      wordsInTun:()=>{return this.sjs.dsp.wordsInTun();}
      ,nowFrame:()=>{return this.predictTxFrame(onep);}
      ,dump:()=>{return this.sjs.dsp.dumpTunBuffer();}
      ,send:(epoc,ts,sz)=>{return this.trigger4(epoc,ts,sz);}
    };

    this.agent = new scheduleFile.FixedSchedule(api);
  }


  watchTunSize() {
    this.tun_packets = this.sjs.txTunBufferLength();

    if( this.tun_packets != 0 ) {
      console.log('arb tun has ' + this.tun_packets);
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

    // this.watchTunSize();
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
    // console.log('trying ' + est);

    let frame = est[1];

    let delta = (hc.SCHEDULE_FRAMES - frame) / hc.SCHEDULE_FRAMES;

    // console.log('delta ' + delta);

    let fudge = 0.14453 * 1000;

    if( this.use_object ) {
      fudge = 0;
      // if( est[1] > 100 && est[1] < 5000 ) {
      //   fudge = -100;
      // }
    }

    let final = this.epocWakeTime+(delta*1000)+fudge;

    // console.log('pre final    ' + final);

    if( this.use_object ) {
      while(final > 1000) {
        final -= 1000;
      }

      if( final < 500 ) {
        final += 1000;
      }
    }

    // console.log('final         ' + final);

    setTimeout(this.groupTriggerAdjust.bind(this), final);

    if( est[1] > 24000 ) {
      let est2;
      do {
        est2 = this.predictTxFrame(onep);
      } while(est2[1] >= 100 );
      // console.log('  ' + est2);
      this.epocStarted(est2);
    } else if( est[1] < 100 ) {
      this.epocStarted(est);
    } else {
      console.log('groupTriggerAdjust() skipping this time');
    }

  }

  eraseTrack() {
    this.track = [];
  }


  handleAgentEpocStarted(est) {

    const firstp = this.txp[0];

    if(!this.counters[firstp] ) {
      console.log("handleAgentEpocStarted counter missing");
      return;
    }

    // console.log('handleAgentEpocStarted ' + est);

    // this.agent_wake = [];

    this.agent_wake = this.agent.wakeupForEpoc(est[0]);
    // console.log(this.agent_wake);

    for(let i = 0; i < this.agent_wake.length; i++) {
      const ts = this.agent_wake[i];

      if( ts === 0 ) {
        continue;
      }

      

      let futWake = () => {
        this.agent.hitTimeslot(ts);
      }

      // FIXME adjust by wakeup time
      const ms = (ts * 512.0) / (25600) * 1000 * hc.SCHEDULES_PER_SECOND;

      setTimeout(futWake, ms);
    }

    this.agent.hitTimeslot(0);

  }

  handleAgent() {
    if(!this.counters[1] ) {
      return;
    }

    // this.agent
  }

  epocStarted(est) {
    // console.log('epocStarted');
    if(this.use_object) {
      this.handleAgentEpocStarted(est);
    } else {
      this.groupTrigger();
    }
  }

  groupTrigger() {

    if( this.counters[1] ) {

    } else {
      console.log('counter not set in groupTrigger()');
      return;
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
    if(!this.use_object) {
      this._gotDebugVec(header, body);
    }
  }

  _gotDebugVec(header, body) {
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



  setupTimers() {
    setTimeout( ()=>{
      this.handle = setInterval(this.watchCounters.bind(this), 1000);
    }, 1500);


    setTimeout( ()=>{
      setTimeout(this.groupTriggerAdjust.bind(this), this.epocWakeTime);
    //   this.handle2 = setInterval(this.groupTrigger.bind(this), 3000);
    }, 1500);

    // setTimeout( ()=>{
    //   this.handle3 = setInterval(this.predictTxFrame.bind(this), 333);
    // }, 3000);


  }

  getCounter() {
    let ca;
    let cb;
    if( this.single_mode ) {
      ca = this.counters[1][0];
      return ca;
    } else {
      let ca = this.counters[1][0];
      let cb = this.counters[2][0];
    }
    return Math.max(ca,cb);
  }


  trigger() {

    let epoc_go = this.getCounter() + 1;

    let res = this.sjs.dsp.scheduleJointFromTunData(this.txp, this.arb_seq, this.word_size, epoc_go, this.ts);

    console.log(res);

    this.arb_seq = (this.arb_seq+1) & 0xff;
  }

  trigger4(epoc_go,ts,sz) {
    let seq = this.arb_seq;
    let res = this.sjs.dsp.scheduleJointFromTunData(this.txp, seq, sz, epoc_go, ts);
    this.arb_seq = (this.arb_seq+1) & 0xff;
    return [seq,res];
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
