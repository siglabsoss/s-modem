'use strict';
const mutil = require('./mockutils.js');
const hc = require('./hconst.js');
const _ = require('lodash');
const mt = require('mathjs');
const Stream = require("stream");
const crypto = require('crypto');
const util = require('util');
const EventEmitter = require('events').EventEmitter;

const streamers = require('../mock/streamers.js');
const Schedule = require('../mock/mockschedule.js');
const Bank = require('../mock/typebank.js');
const HiggsSocket = require('./mockhiggssocket.js');
const FBParse = require("../mock/fbparse.js");
const FBTag = require("../mock/fbtag.js");
const Cooked = require("../mock/cooked_data_unrotated.js");
const RateKeep = require('../mock/ratekeep.js');
const CoarseSync = require('../mock/coarsesync.js');
const nativeWrap = require('../mock/wrapnativestream.js');
const txChain1 = require('../build/Release/TxChain1.node');
const rxChain1 = require('../build/Release/RxChain1.node');
const bufferSplit = require('../mock/nullbuffersplit');
const parseDashTie = require('../mock/parsedashtie.js');
const workKeeper = require('../mock/workkeeper.js');

// cs20 #defines
const UD_NOT_ACTIVE = (0);
const UD_BUFFER = (1);
const UD_WAIT = (2);
const UD_SENDING = (3);
const UD_SENDING_TAIL = (4);
const UD_DUMPING = (5);

// cs00 #defines
const FSM_FLUSHING  =  (1);
const FSM_DO_SYNC   = (4);
const FSM_PING_PONG  =  (3);



// streams out 32bit IShort (0xIIIIRRRR)
class CS20DataOrigin extends Stream.Readable{
  
  constructor(options, _higgs){
    super(options); //Passing options to native class constructor. (REQUIRED)
    // this.count = 1; // number of chunks to readout
    this.chunk = 1024;

    this.higgs = _higgs;

    this.width = 4;

    this.limiter = new RateKeep.RateKeep({
        rate:this.higgs.opts.rateLimit, // how many ofdm frames per second
        mode:RateKeep.Mode.Queue // if running too fast, call it later for us
      });

    this.thebuffer = new ArrayBuffer(4*this.chunk); // in bytes
    this.uint8_view = new Uint8Array(this.thebuffer);
    this.uint32_view = new Uint32Array(this.thebuffer);
    // this.uint16_view = new Uint16Array(this.thebuffer);

    for(let i = 0; i < this.chunk; i++) {
      let im = 0;
      let re = 0;
      this.uint32_view[i] = im | re;
    }

  }
    
  _read(sz){

    // let ret = true;
    // while(ret && this.count > 0) {
    // while(ret) {

    let work = () => {
      let [buffer,offset] = this.higgs.cs20loop();

      if( typeof buffer === 'undefined' || typeof offset === 'undefined' ) {
        throw(new Error('cs20loop() returned undefined'));
      }

      if(!this.higgs.opts.disableTxProcessing) {
        setImmediate( () => { this.push(buffer.slice(offset,offset+(1024*4))); } );
      } else {
        // not sure how to avoid this but when we want to disable TX Processing
        // we still need to push something here
        // if we never call push, _read never gets called again
        this.push(new Uint8Array(16));
      }
    }

    if(this.higgs.opts.rateLimit !== 0 ) {
      this.limiter.do(work.bind(this));
    } else {

      if(this.higgs.opts.lockInputOutputRates) {
        this.higgs.keeper.do(0,work.bind(this));
      } else {
        work();
      }

    }

  }
}

function build_vmem_zeros(len = 1024) {
    let thebuffer = new ArrayBuffer(4*len); // in bytes
    let uint8_view = new Uint8Array(thebuffer);
    let uint32_view = new Uint32Array(thebuffer);

    for(let i = 0; i < len; i++) {
      let im = 0;
      let re = 0;
      uint32_view[i] = im | re;
    }

    return uint8_view;
}

class Higgs extends HiggsSocket.HiggsSocket {
  constructor() {
    super()

    this.keeper = new workKeeper.WorkKeeper({tolerance:1000,print:false,printFirstBlock:true});

    this.environment = undefined;
  }

  attachedSmodemInit() {
    // SMODEM_BOOT_CMD

    // console.log("registering "+ hc.RING_ENUM_CS20);
    // console.log("registering "+ hc.RING_ADDR_CS20);
    this.regRingbusCallback(this.attachedSmodemBootRing.bind(this), hc.RING_ADDR_CS20, hc.SMODEM_BOOT_CMD);
  }

  attachedSmodemBootRing(data) {
    let asWho = "who?"
    if(data == 2) {
      asWho = "tx";
    }
    if( data == 1) {
      asWho = "rx";
    }
    console.log("smodem boot as " + asWho);
    this.notify.emit('smodem.boot', 'tx');
  }


  cs00Init() {
    this.cs00uint = new Bank.TypeBankUint32().access();


    this.regRingbusCallback(this.cs00_stream_callback.bind(this), hc.RING_ADDR_CS00, hc.STREAM_CMD);
    this.regRingbusCallback(this.cs00_turnstile_advance_callback.bind(this), hc.RING_ADDR_CS00, hc.TURNSTILE_CMD);
    this.regRingbusCallback(this.cs00_sync_callback.bind(this), hc.RING_ADDR_CS00, hc.SYNCHRONIZATION_CMD);

  }


  cs00_stream_callback(x) {
    console.log('cs00_stream_callback called ' + x.toString(16));
}
  cs00_turnstile_advance_callback() {
    console.log('cs00_turnstile_advance_callback called');
}
  cs00_sync_callback() {
    console.log('cs00_sync_callback called');
    this.rx.stream.coarse.triggerCoarse();
}

  cs00_coarse_results(val, cplx) {
    this.ring_block_send_eth( (hc.COARSE_SYNC_PCCMD | (val&hc.DATA_MASK))>>>0 );
    console.log('coarse sync finished ' + val);
    this.notify.emit('cs00.coarseResult', val, cplx);
  }

  cs10Init() {
    this.cs10uint = new Bank.TypeBankUint32().access();
    const uint = this.cs10uint;

    uint.ringbus_sfo_adjustment_temp = 0;

    

    this.regRingbusCallback(this.cs10_sfo_adjustment_callback.bind(this), hc.RING_ADDR_CS10, hc.SFO_PERIODIC_ADJ_CMD);
    this.regRingbusCallback(this.cs10_sfo_sign_callback.bind(this), hc.RING_ADDR_CS10, hc.SFO_PERIODIC_SIGN_CMD);
  }

  cs10_sfo_adjustment_callback(x) {
    const uint = this.cs10uint;
    console.log('cs10_sfo_adjustment_callback called ' + x.toString(16));
    uint.ringbus_sfo_adjustment_temp = x;
  }
  cs10_sfo_sign_callback(data) {
    const uint = this.cs10uint;
    console.log('cs10_sfo_sign_callback called ' + data.toString(16));

    this.notify.emit('cs10.sfoSignCallback', uint.ringbus_sfo_adjustment_temp, data);

    if( data === 3 || data === 2 || data === 1 || data === 0 ) {
      console.log("UNSUPPORTED Mode " + data + " in cs10_sfo_sign_callback()");
      return;
    }

    if( data === 4 ) {
      if( this.opts.turnstileAppliesToRx ) {
        console.log("adjusting rx turnstile because this.opts.turnstileAppliesToRx is set");
        this.rx.stream.turnstile.advance(uint.ringbus_sfo_adjustment_temp*8*2); // advance is in bytes so we mul by 4
        return;
      } else {
        console.log("UNSUPPORTED TX CHAIN FEATURE cs10_sfo_sign_callback()");
        return;
      }
    }


  }

  cs20Init() {
    this.cs20schedule = Schedule.schedule_t();
    Schedule.schedule_all_on(this.cs20schedule);

    this.cs20uint = new Bank.TypeBankUint32().access();
    this.cs20uint.progress = 0;
    this.cs20uint.accumulated_progress = 0;
    this.cs20uint.timeslot = 0;
    this.cs20uint.epoc = 0;
    this.cs20uint.can_tx = 0;
    this.cs20uint.frame_counter = 0;
    this.cs20uint.lifetime_32 = 0;
    this.cs20uint.lifetime_32_measure_speed = 0;

    this.cs20uint.tdma_mode = 0;
    this.cs20uint.tdma_mode_pending = 0;


    this.cs20uint.ud_timeslot = 0;
    this.cs20uint.ud_epoc = 0;

    this.cs20uint.ud_state = UD_NOT_ACTIVE;

    // this object contains a header and a buffer which are passed to us as a result of
    // calling regUserdataCallback with a function
    this.cs20_mapmov = {buffer:undefined, header:undefined};

    // if the function passed to regUserdataCallback gets called too frequently, we buffer the objects
    // on this end
    this.cs20_buffered_mapmov = [];

    this.cs20_epoc_needs_latch = false; // flag set when s-modem wants to know current timers
    this.cs20_userdata_needs_fill_report = false; // flag set when userdata comes

    // uint8 byte arrays
    // these were previously vmem data
    // annoying, but we store them as byte array which means all code that treats them as vmem
    // will need to multilpy 4 for indicing
    this.vmem_zeros = build_vmem_zeros();

    let [junk,uview] = mutil.I32ToBufMulti(Cooked.vmem_counter);
    this.vmem_counter = uview;

    this.regRingbusCallback(
      this.cs20_epoc_was_requested_callback.bind(this),
      hc.RING_ADDR_CS20,
      hc.REQUEST_EPOC_CMD );

    this.regRingbusCallback(
      this.cs20ResetEq.bind(this),
      hc.RING_ADDR_CS20,
      hc.EQUALIZER_CMD );



    this.logTxAveragePeriod = 2000; // in ms
    this.logTxSpeedInterval = setInterval(this.logTxSpeed.bind(this), this.logTxAveragePeriod);

    // for verifyMapMov function
    this.storeMapMov = []
  }

  logTxSpeed() {
    if( this.opts.log.tx_speed ) {
      let delta_frames = this.cs20uint.lifetime_32 - this.cs20uint.lifetime_32_measure_speed;

      this.cs20uint.lifetime_32_measure_speed = this.cs20uint.lifetime_32;

      let fps = delta_frames / (2.0);

      console.log('Average OFDM frames/s: ' + fps);

          
    }
  }

  setupVerifyMapMov() {
    this.notify.on('smodem.boot', () => this.storeMapMov = []);
  }

  verifyMapMov(header, data) {
    this.storeMapMov.push([header,data]);

    let correct = '04f986';
    let countCorrect = 0;

    for(let x of this.storeMapMov) {
        let hheader = crypto.createHash('sha1').update(x[0].toString()).digest('hex');
        let hdata = crypto.createHash('sha1').update(x[1].toString()).digest('hex');
        console.log("head " + hheader.slice(0,6) + " bod " + hdata.slice(0,6));
        if( hdata.slice(0,6) === correct ) {
          countCorrect++;
        }
    }

    if( countCorrect > 4 ) {
      
      this.notify.emit('verify.mapmov.pass', true);
      console.log('-----------------------------------')
      console.log('-')
      console.log('- Verify MapMov pass')
      console.log('-')
      console.log('-----------------------------------')
    }

    // console.log(this.storeMapMov);

  }

  cs20_buffer_mapmov(header, data) {
    this.cs20_buffered_mapmov.push({buffer:data, header:header});
  }

  cs20_unbuffer_if_needed() {
    if( this.cs20_buffered_mapmov.length > 0 ) {
      let o = this.cs20_buffered_mapmov.shift();
      console.log('feeding a saved mapmov, there are ' + this.cs20_buffered_mapmov.length + ' more');
      this.cs20_mapmov_userdata(o.header, o.buffer);
    }
  }

  // called when new mapmov data has been received
  cs20_mapmov_userdata(header, data) {
    if( this.opts.verifyMapMovDebug ) {
      this.verifyMapMov(header, data);
    }
    // console.log('cs20_mapmov_userdata ' + header + '\n' + data);
    const uint = this.cs20uint;

    let timeslot = FBParse.g_seq(header);
    let epoc = FBParse.g_seq2(header);

    if( typeof this.cs20_mapmov.header !== 'undefined' ) {
      this.cs20_buffer_mapmov(header, data);
      console.log('mapmov data is coming too fast, buffering (' + this.cs20_buffered_mapmov.length + ')');
      return;
    }

    this.cs20_mapmov.header = header;

    uint.ud_total_chunks = data.length / 1024;
    uint.ud_chunk_index_consumed = 0;

    console.log('ud came in ' + uint.ud_total_chunks);
    console.log('ud had len ' + data.length);

    uint.ud_timeslot = timeslot; // timeslot
    uint.ud_epoc = epoc; // epoc

    console.log('got userdata scheudled for epoc: ' + uint.ud_epoc + ' timeslot: ' + uint.ud_timeslot);
    console.log('now is                     epoc: ' + uint.epoc + ' timeslot: ' + uint.timeslot);

    this.cs20_userdata_needs_fill_report = true;


    // this.cs20_mapmov.buffer;

    let b = new ArrayBuffer(data.length*4);
    let uint8_view = new Uint8Array(b);
    let writeto = 0;
    let parts;
    for(let i = 0; i < data.length; i++) {
      parts = mutil.I32ToBuf(data[i]);
      // console.log(parts);
      uint8_view[writeto+0] = parts[0];
      uint8_view[writeto+1] = parts[1];
      uint8_view[writeto+2] = parts[2];
      uint8_view[writeto+3] = parts[3];
      writeto+=4;
    }

    this.cs20_mapmov.buffer = uint8_view;

    uint.ud_state = UD_WAIT;

  }

  // called by cs20loop() if we are done with the current mapmov data
  user_data_finished() {
    const uint = this.cs20uint;

    uint.ud_chunk_index_consumed = 0;

    uint.ud_timeslot = 0;

    uint.ud_state = UD_NOT_ACTIVE;

    this.cs20_mapmov.buffer = undefined;
    this.cs20_mapmov.header = undefined;

    // if there is another mapmov buffer waiting, grab it now
    this.cs20_unbuffer_if_needed();

  }

  cs20loop() {
    const uint = this.cs20uint;

    let output_buffer = undefined;
    let output_offset = undefined;

    // console.log('entering cs20loop() with lifetime_32:' + uint.lifetime_32 );

    [ uint.progress,
      uint.accumulated_progress,
      uint.timeslot,
      uint.epoc,
      uint.can_tx ] = Schedule.schedule_get_timeslot2(this.cs20schedule, uint.lifetime_32);

    uint.frame_phase = (uint.progress % 5);
    uint.input_frame_phase = (uint.progress & (16-1) ); // same as % 16


    if(this.cs20_userdata_needs_fill_report) {
      this.cs20_userdata_needs_fill_report = false;
      this.cs20_send_fill_report(); // don't pass args, this function pulls them from `this`
    }

    switch(uint.ud_state) {
          // These first 2 states
          // this means we came in JUST EARLY ENOUGH
          // this should be a warning at least
          // possibly error condition

          // this.ring_block_send_eth(CS02_USERDATA_ERROR | 1); // warning, technically might still be ok to run
          // NO BREAK
      case UD_BUFFER:
      case UD_WAIT:
          /// if requested epoc second is in the past
          if( uint.ud_epoc != 0 && (uint.ud_epoc < uint.epoc) ) {
              uint.ud_state = UD_DUMPING;
              this.ring_block_send_eth(hc.CS02_USERDATA_ERROR | 2);
          } else if( (uint.ud_epoc == uint.epoc) ) {
              // it is currently the epoc second
              if( (uint.ud_timeslot < uint.timeslot) ) {
                  // if requested timeslot is in the past
                  uint.ud_state = UD_DUMPING;
                  this.ring_block_send_eth(hc.CS02_USERDATA_ERROR | 7);
              } else if (uint.ud_timeslot == uint.timeslot && uint.progress == 0) {
                  if( uint.can_tx ) {
                      // timeslot is ok, we are allowed to transmit, go time
                      uint.ud_state = UD_SENDING;
                      // fb_unpause_mapmov(); // we were either paused, or came in with super low buffer, so unpause
                      // this.ring_block_send_eth(0xdead000a);
                  } else {
                      // timeslot is ok for us to go, but schedule says it is illegal to transmit
                      uint.ud_state = UD_DUMPING;
                      this.ring_block_send_eth(hc.CS02_USERDATA_ERROR | 4);
                  }
                  // technically we could check for buffer fill level
                  // but it should be impossible to get into this state with a bad fill level?
              } else if (uint.ud_timeslot > uint.timeslot) {
                  // do nothing,
                  // we are in the right second, but not the right timeslot
              } else {
                  // fail, we got the packet in the right epoc second, but timeslot was too late
                  // even if timeslot was correct, progress was not zero, so still fail
                  uint.ud_state = UD_DUMPING;
                  this.ring_block_send_eth(hc.CS02_USERDATA_ERROR | 5);
              }
          } else {
              // do nothing, epoc is in the future
          }
          break;

      case UD_DUMPING:
      case UD_SENDING:
      case UD_SENDING_TAIL:
      case UD_NOT_ACTIVE:
          break;
      default:
          break;
    }


    if(uint.can_tx) {
        if( uint.ud_state == UD_SENDING || uint.ud_state == UD_SENDING_TAIL ) {
          output_buffer = this.cs20_mapmov.buffer;
          output_offset = uint.ud_chunk_index_consumed*1024;

          if( this.opts.log.successful_mapmov_frame ) {
            console.log('outputting mapmov chunk ' + uint.ud_chunk_index_consumed);
          }
          uint.ud_chunk_index_consumed++; // bump chunk we are consuming
        } else {
            if( uint.tdma_mode == 7 ) {
                // tdma_mode is controlled by rb
                // see setPartnerTDMA() in HiggsPartner.cpp
                uint.frame_phase = 1;
            }
            // which phase of the canned output are we on?
            // this resets to 0 at the start of a timeslot
            // input_dma_offset = (input_frame_phase*1024);
            output_buffer = this.vmem_counter;
            output_offset = (uint.frame_phase*1024);
            // input_offset_row = input_dma_offset/16;
            // output_dma_offset = (frame_phase*1024);
            // output_offset_row = output_dma_offset/16;
        //   output_dma_offset = input_dma_offset = (frame_phase*1024);
        // output_offset_row = input_offset_row = input_dma_offset/16; 
            // these are cpu pointers to base of a 5 buffer long array
            // input_base = (uint32_t*)vmem_counter;
            // output_base = (uint32_t*)work_area;
        }
    } else {
      output_buffer = this.vmem_zeros;
      output_offset = 0;
    }

    // #ifdef OVERWRITE_COUNTER

    // eq multiply

    // run sfo

    if(
           (uint.ud_state == UD_SENDING || uint.ud_state == UD_SENDING_TAIL)
        && uint.ud_chunk_index_consumed == uint.ud_total_chunks
         ) {
        this.user_data_finished();
    }




    // report counters via ringbus if requested
    if( this.cs20_epoc_needs_latch ) {
      this.cs20_send_epoc(uint.accumulated_progress, uint.epoc);
      this.cs20_epoc_needs_latch = false;
    }

    if(typeof this.delayPrintLifetime === 'undefined') {
      this.delayPrintLifetime = 0;
    }

    if( this.delayPrintLifetime++ % 100 == 0) {
      // console.log(uint.lifetime_32);
    }

    // counters
    uint.frame_counter++;
    uint.lifetime_frame_counter++;
    uint.lifetime_frame_counter &= 0xffffff;
    uint.lifetime_32++; // runs at same rate as frame_counter but overflows less frequently

    if(uint.frame_counter == 5) {
        uint.frame_counter = 0;
    }

    // convert to bytes at the last hour
    return [output_buffer, output_offset*4];
  }

  // this reports to s-modem how early or late the scheduled userdata packet was
  cs20_send_fill_report() {
    const uint = this.cs20uint;

    // these are used as stack variables, but end up getting left in `this.cs20uint`, no big deal:
    // e_delta, frame_delta, fill_report_masked


    // int8_t e_delta = (int8_t)(ud_epoc - epoc);
    uint.e_delta = (uint.ud_epoc - uint.epoc);
    uint.e_delta &= 0xff;

    // int16_t frame_delta = (int16_t)((ud_timeslot<<SCHEDULE_LENGTH_BITS) - accumulated_progress);
    uint.frame_delta = (uint.ud_timeslot<<Schedule.SCHEDULE_LENGTH_BITS);
    uint.frame_delta -= uint.accumulated_progress;
    uint.frame_delta &= 0xffff;

    // uint32_t masked = ((e_delta<<16)&0xff0000) | (frame_delta&0xffff);
    uint.fill_report_masked = ((uint.e_delta<<16)&0xff0000) | (uint.frame_delta&0xffff);
    
    this.sendRb(hc.CS02_FILL_LEVEL_PCCMD | uint.fill_report_masked );
  }

  // see cs20/main.c: pet_epoc_readback2()
  cs20_send_epoc(epoc_latch_frame, epoc_latch_second) {
    let base;
    let dmode;

    console.log('sending epoc back of ' + epoc_latch_frame + ', ' + epoc_latch_second);

    base = (epoc_latch_frame & 0xffff);
    dmode = 4 << 16;
    this.sendRb(hc.EPOC_REPLY_PCCMD | base | dmode);

    base = ( (epoc_latch_frame>>16) & 0xffff);
    dmode = 3 << 16;
    this.sendRb(hc.EPOC_REPLY_PCCMD | base | dmode);

    base = (epoc_latch_second & 0xffff);
    dmode = 2 << 16;
    this.sendRb(hc.EPOC_REPLY_PCCMD | base | dmode);

    base = ( (epoc_latch_second>>16) & 0xffff);
    dmode = 1 << 16;
    this.sendRb(hc.EPOC_REPLY_PCCMD | base | dmode);
  }

  // this function gets called by s-modem when it wants to know the current values of the counters
  // see cs20_send_epoc()
  cs20_epoc_was_requested_callback() {
    console.log('cs20_epoc_was_requested_callback ');
    this.cs20_epoc_needs_latch = true;
  }

  cs20UpdateSchedule(header, data) {
    console.log('cs20UpdateSchedule');

    if(this.opts.useNativeTxChain) {
      console.log("FIXME add eq to native TX chain");
      return;
    }


    if( data.length !== Schedule.SCHEDULE_SLOTS ) {
      throw(new Error('cs20UpdateSchedule with incorrect size ' + Schedule.SCHEDULE_SLOTS + ', ' + data.length));
    }

    for(let i = 0; i < data.length; i++) {
      this.cs20schedule.s[i] = data[i];
    }
  }

  cs20UpdateEq(header, data) {
    console.log('cs20UpdateEq');

    if(this.opts.useNativeTxChain) {
      console.log("FIXME add eq to native TX chain");
      return;
    }

    // console.log(data);

    this.tx.stream.eq.loadEqIShort(data);

    // this.tx.stream.eq

  }

  cs20ResetEq(data) {
    console.log('cs20ResetEq');

    if(this.opts.useNativeTxChain) {
      console.log("FIXME add eq to native TX chain");
      return;
    }

    if( data === 0 ) {
      let eq = [];
      for(let i = 0; i < 1024; i++) {
        eq.push(0x7fff);
      }
      this.tx.stream.eq.loadEqIShort(eq);
    } else {
      console.log('cs20ResetEq illegal value ' + data);
    }
  }

  extraInit() {
    this.splitSmodemRpc = new bufferSplit.NullBufferSplit(this.ethGotRpcFromSmodem.bind(this));

    this.dash = new parseDashTie.ParseDashTie();

    this.regRingbusCallback(
      (x)=>{
        let l = Array(3);
        l[0] = (x>>16)&0xff;
        l[1] = (x>>8)&0xff;
        l[2] = x&0xff;
        this.splitSmodemRpc.addData(Buffer.from(l));
      },
      hc.RING_ADDR_ETH,
      hc.NODEJS_RPC_CMD );
  }

  ethInit() {
    this.regRingbusCallback(
      this.eth_self_test.bind(this),
      hc.RING_ADDR_ETH,
      hc.ETH_TEST_CMD );

    // do nothing for DSA gain
    this.regRingbusCallback(
      ()=>{},
      hc.RING_ADDR_ETH,
      hc.DSA_GAIN_CMD );


    this._eth_fill_level = 0;
    this.ethFillInterval = setInterval(this.ethReportFillLevel.bind(this), 100);
  }

  ethGotRpcFromSmodem(x) {
    if( this.opts.log.rpc ) {
      console.log(" got: " + x);
    }
    this.dash.addData(x);
  }

  sendToHiggsRpc(o) {
    return this.sendToHiggsRpcRaw(JSON.stringify(o));
  }

  sendToHiggsRpcRaw(str) {
    let word = 0;
    for (var i = 0; i < str.length; i++) {
      let c = str.charCodeAt(i);
      let mod = i%3;
      switch(mod) {
        case 0:
          word = word | (((c)&0xff)<<16);
          break;
        case 1:
          word = word | (((c)&0xff)<<8);
          break;
        case 2:
          word = word | (((c)&0xff)<<0);
          break;
        default:
          console.log("sendToHiggsRpc() broken");
          break;
      }

      if( mod == 2 || (i+1) == str.length ) {
        // cout << HEX32_STRING(word) << endl;
        this.ring_block_send_eth((hc.NODEJS_RCP_PCCMD | word)>>>0);
        word = 0;
      }
    }
    this.ring_block_send_eth((hc.NODEJS_RCP_PCCMD | 0)>>>0);
  }

  ethReportFillLevel() {
    // for now just use some junk values
    // this._eth_fill_level++;// = 0;
    // console.log(this._eth_fill_level)
    this._eth_fill_level = (this._eth_fill_level+1) % 3;

    // choose a 'random' value but always on the empty side
    let fill_level  = 1024 + (this._eth_fill_level*512);

    // console.log('reporting fill level ' + fill_level);

    this.sendRb(hc.FILL_REPLY_PCCMD | fill_level );

  }

  eth_self_test(word) {
    let data = word & 0x00ffffff;
    let cmd_type = word & 0xff000000;

    // if( this.opts.log.rb ) {
    //   console.log('eth got rb with data: ' + data);
    // }

    if( data === 1 ) {
      for(let i = 0; i < 10; i++ ) {
        // let buf = Buffer.from( mutil.I32ToBuf(ETH_TEST_CMD | i) );
        this.sendRb(hc.ETH_TEST_CMD | i);
      }
    }

    if( data === 2 ) {
      this.sendRb(0xdeadbeef);
    }

  }


  fb_flush(header, body) {
    // seems we do nothing here
  }

  fb_status_reply(header, body) {
    console.log('Feedback Bus callback fb_status_reply() fired');
    this.sendRb(hc.FEEDBACK_ALIVE);
  }

  // when this fires, this.fbparse is fully constructed
  // and now needs hooks
  dsockListeningFinished() {
    // bind functions for each type of feedback bus message you wish to handle
    this.fbparse.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_FLUSH,
      this.fb_flush.bind(this) );

    this.fbparse.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_STATUS_REPLY,
      this.fb_status_reply.bind(this) );

    this.fbparse.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_SCHEDULE,
      this.cs20UpdateSchedule.bind(this) );

    this.fbparse.regCallback(
      hc.FEEDBACK_TYPE_VECTOR,
      hc.FEEDBACK_VEC_TX_EQ,
      this.cs20UpdateEq.bind(this) );


    this.fbparse.regUserdataCallback(this.cs20_mapmov_userdata.bind(this));

    this.setupStreams();

  }


  setupTxStream() {
    this.tx = {stream:{}};
    this.tx.stream.origin = new CS20DataOrigin({}, this);

    this.txEndpoint;

    if( !this.opts.disableTxProcessing ) {
      if( this.opts.useNativeTxChain ) {
        let addOutputCp = 256;
        let removeInputCp = 0;

        let afterShutdown = () => {};
        let afterPipe = () => {console.log('rx did pipe')};

        this.tx.stream.native = new nativeWrap.WrapNativeTransformStream({},{print:false,args:[removeInputCp,addOutputCp]}, txChain1, afterPipe, afterShutdown);

        this.tx.stream.origin
          .pipe(this.tx.stream.native);

        this.unpipeTx = () => {
          console.log("unpipe native");
          this.tx.stream.origin.pause();
          this.tx.stream.native.pause();
          
          this.tx.stream.origin.unpipe(this.tx.stream.native);


          // console.log("env " + this.envId);

          let envRadio = this.environment.getRadio(this.envId);
          // console.log(envRadio);
          this.tx.stream.native.unpipe(envRadio.tx.ours);

        }

        this.txEndpoint = this.tx.stream.native;

      } else {
        this.tx.stream.convert = new streamers.IShortToCF64({}, {});
        this.tx.stream.eq = new streamers.IFloatEqStream({}, {chunk:1024});
        this.tx.stream.eq.defaultEq();
        this.tx.stream.fft = new streamers.CF64StreamFFT({}, {n:1024,cp:256,inverse:true});

        // pipe up, but not pipe to the environment
        this.tx.stream.origin
          .pipe(this.tx.stream.convert)
          .pipe(this.tx.stream.eq)
          .pipe(this.tx.stream.fft);

        this.unpipeTx = () => {
          console.log("unpipe js");
          this.tx.stream.origin.unpipe(this.tx.stream.convert);
          this.tx.stream.convert.unpipe(this.tx.stream.fft);
        }

        this.txEndpoint = this.tx.stream.fft;
      }
    } else {
      this.tx.stream.dummy = new streamers.DummyStream();

      this.tx.stream.origin
        .pipe(this.tx.stream.dummy);

      this.txEndpoint = this.tx.stream.origin;
    }
  }

  setupRxStream() {
    this.rx = {stream:{}};
    
    let disableRxCapture = true;
    let udpChunk = hc.UDP_PACKET_DATA_SIZE-4;

    if( typeof this.opts.streamHexFromFilename !== 'undefined' ) {
      this.rx.stream.capture = new streamers.F64CaptureStream({},{disable:disableRxCapture});
      this.rxEndpoint = this.rx.stream.capture;

      console.log('Streaming hex from ' + this.opts.streamHexFromFilename + ' to rx port of s-modem');

      this.rx.stream.hexFile = new streamers.RateStreamHexFile({},{chunk:1024,rate:1000,fname:this.opts.streamHexFromFilename,repeat:true});

      this.rx.stream.addCounter = new streamers.AddCounterStream({},{chunk:(udpChunk)});

      this.rx.stream.udp = new streamers.StreamToUdp({},{port:this.opts.RX_DATA_PORT,addr:this.opts.SMODEM_ADDR,sock:this.dsock});

      this.rx.stream.hexFile
        .pipe(this.rx.stream.addCounter)
        .pipe(this.rx.stream.udp);



    } else if( !this.opts.disableRxProcessing ) {
      if( this.opts.useNativeRxChain ) {

        let coarseCallback = function(val) {
          console.log('got coarse callback with 0x' + val.toString(16));
          // coarseResultsArray.push(val);
        }

        let afterShutdown = () => {};
        let afterPipe = () => {console.log('rx did pipe')};

        let addOutputCp = 0;
        let removeInputCp = 256;

        this.rx.stream.native = new nativeWrap.WrapRxChain1({},{print:false,args:[removeInputCp,addOutputCp]}, rxChain1, afterPipe, afterShutdown, coarseCallback);

        this.rx.stream.tagger = new FBTag.Tagger({},{});

        this.rx.stream.addCounter = new streamers.AddCounterStream({},{chunk:(udpChunk)});

        this.rx.stream.udp = new streamers.StreamToUdp({},{port:this.opts.RX_DATA_PORT,addr:this.opts.SMODEM_ADDR,sock:this.dsock});

        this.rx.stream.native
          .pipe(this.rx.stream.tagger)
          .pipe(this.rx.stream.addCounter)
          .pipe(this.rx.stream.udp);

        this.rxEndpoint = this.rx.stream.native; 
      } else {
        this.rx.stream.turnstile = new streamers.ByteTurnstileStream({},{chunk:(1024+256)*8*2});

        if( this.opts.lockInputOutputRates ) {
          this.rx.stream.toIShort = new streamers.IFloatToIShortMetered({}, {keeper:this.keeper, keeperId:1});
        } else {
          this.rx.stream.toIShort = new streamers.IFloatToIShort();
        }

        this.rx.stream.coarse = new CoarseSync.CoarseSync({},{cp:256,ofdmNum:20,print2:false,print1:false,print3:false,print4:false}, this.cs00_coarse_results.bind(this));

        this.rx.stream.toFloat = new streamers.IShortToCF64({}, {});

        this.rx.stream.fft = new streamers.CF64StreamFFT({}, {n:1024,cp:0,inverse:true});

        this.rx.stream.gain = new streamers.F64GainStream({}, {gain:1/100});

        this.rx.stream.tagger = new FBTag.Tagger({},{});

        this.rx.stream.addCounter = new streamers.AddCounterStream({},{chunk:(udpChunk)});

        this.rx.stream.udp = new streamers.StreamToUdp({},{port:this.opts.RX_DATA_PORT,addr:this.opts.SMODEM_ADDR,sock:this.dsock});

        this.rx.stream.turnstile
          .pipe(this.rx.stream.toIShort)
          .pipe(this.rx.stream.coarse)
          .pipe(this.rx.stream.toFloat)
          .pipe(this.rx.stream.fft)
          .pipe(this.rx.stream.gain)
          .pipe(this.rx.stream.tagger)
          .pipe(this.rx.stream.addCounter)
          .pipe(this.rx.stream.udp);

        this.rxEndpoint = this.rx.stream.turnstile;
      }

    } else {
      this.rx.stream.capture = new streamers.F64CaptureStream({},{disable:disableRxCapture});
      this.rxEndpoint = this.rx.stream.capture;
    }
  }

  setupStreams() {
    this.setupTxStream();
    this.notify.emit('streams.tx.setup');
    this.setupRxStream();

    this.notify.emit('streams.rx.setup');
    this.notify.emit('streams.all.setup');


    let attachResult = this.environment.attachRadio(this.txEndpoint, this.rxEndpoint);

    console.log('attached to environment with result ' + attachResult);

    this.envId = attachResult;

    this.notify.emit('environment.attached');
  }


  start(optsin, _env) {
    Object.assign(this.opts, optsin);

    this.environment = _env;

    this.cs20Init();
    this.cs10Init();
    this.ethInit();
    this.extraInit();
    this.cs00Init();
    this.attachedSmodemInit();
    this.setupVerifyMapMov();

    // call start on base class
    // this can only be called after options have been assign()'d to the class
    this.startSocket();

    if(this.opts.log.portNumbers) {
      console.log("Port Numbers");
      console.log(this.opts.RX_CMD_PORT);
      console.log(this.opts.TX_CMD_PORT);
      console.log(this.opts.TX_DATA_PORT);
      console.log(this.opts.RX_DATA_PORT);
    }

    if(this.opts.lockInputOutputRates) {
      this.keeper.peers(2);
    }

    // this.setupStreams();  // now called by dsockListeningFinished
  }

  // does not call unpipe, stops native
  stop2() {
    if( this.opts.useNativeRxChain ) {
      console.log('this.rx.stream.native.stop();');
      this.rx.stream.native.stop();
    }
    if( this.opts.useNativeTxChain ) {
      console.log('this.tx.stream.native.stop();');
      this.tx.stream.native.stop();
    }
  }


  // calls unpipe first
  stop() {
    this.unpipeTx(); // unpipe now
    if( !this.opts.useNativeRxChain && !this.opts.useNativeTxChain ) {
      console.log('will not stop any native chains');
      return Promise.resolve();
    } else {
      return new Promise((resolve,reject)=>{
        setTimeout(()=>{

          this.stop2()

          resolve();

        }, 500); // timeout
      }); // promise
    } // else
  } // stop

  // astop1() {
  //   this.unpipeTx();
  // }

  // astop2() {
  //   this.stop2();
  // }


} // class Higgs

function portNumbers(idx) {
  let obj = {};
  let bump = idx*10;
  obj.RX_CMD_PORT = (10001+bump);
  obj.TX_CMD_PORT = (20000+bump);
  obj.TX_DATA_PORT = (30000+bump);
  obj.RX_DATA_PORT = (40001+bump);

  return obj;
}

function getDefaults(idx=0) {
  const defaults = {log:{}};

  const ports = portNumbers(idx);

  // defaults.RX_CMD_PORT = (10001);
  // defaults.TX_CMD_PORT = (20000);
  // defaults.TX_DATA_PORT = (30000);
  // defaults.RX_DATA_PORT = (40001);


  Object.assign(defaults, ports);

  defaults.SMODEM_ADDR = '127.0.0.1';

  defaults.log.rbsock      = false; // lock ringbus socket packets
  defaults.log.dsock       = false; // log data socket packets
  defaults.log.rb          = true; // turn on
  defaults.log.rbUnhandled = true; // log unhandled messages
  defaults.log.rbNoisy     = false; // if log rb is on, do we log noisy ones?
  defaults.log.tx_speed    = false; // log in OFDM frames per second
  defaults.log.successful_mapmov_frame = false; // log when s-modem sends us a mapmov frame
  defaults.log.rpc      = false;
  defaults.log.portNumbers = false;

  defaults.disableTxProcessing = false;
  defaults.disableRxProcessing = false;

  defaults.useNativeTxChain = true; // only valid if enabled (see disableTxProcessing)
  defaults.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

  defaults.streamHexFromFilename = undefined;
  // defaults.streamHexFromFilename = 'test/data/long_cs21_out.hex';

  defaults.verifyMapMovDebug = false;

  defaults.turnstileAppliesToRx = false;

  defaults.rateLimit = hc.SCHEDULE_FRAMES_PER_SECOND;

  defaults.lockInputOutputRates = false;

  return defaults;
}


module.exports = {
    Higgs
  , getDefaults
  , portNumbers
}