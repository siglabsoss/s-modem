'use strict';

const hc = require('../../js/mock/hconst.js');
const util = require('util');

class HandleFeedbackBusRecovery {
  constructor(native) {
    this.sjs = native;
    // console.log('HandleCS20Tx boot');

    this.capCount = 0;
    this.captures = [{}];
    this.rbDuringCapture = [0];
  
    this.checkAlive = [0x00000002,0x00000020,0x00000000,0xdeadbeef,0x00000007,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000];

    this.setupMonitorRequest();

  }


  setupMonitorRequest() {
    this.monitorTimer = setInterval(()=>{this.monitorCleanRequest()}, 10);
    this.recovering = false;
  }

  monitorCleanRequest() {
    let go = this.sjs.getBoolDefault(false, 'runtime.js.recover_fb.request');
    if( go && !this.recovering ) {
      console.log("monitorCleanRequest triggered");
      this.recovering = true;
      this.sjs.settings.setBool('runtime.js.recover_fb.request', false);
      this.sjs.settings.setBool('runtime.js.recover_fb.result', false);

      this.check();

      setTimeout(()=>{

        if( this.sjs.isDebugMode ) {
          this.sjs.settings.setBool('runtime.js.recover_fb.result', true);
          this.recovering = false;
          return;
        }

        let o = this.captures[this.capCount];
        this.printCapture();
        if( o.cs11.ud.state !== 0 ) {
          let total = o.cs11.ud.total_chunks;
          let at = o.cs11.ud.chunk_index_consumed;
          let free = total - at;


          let total2 = o.fb.status.mapmov.total_chunks;
          let at2    = o.fb.status.mapmov.this_chunk;
          let sendZeros = (total2 - at2) * 1024;

          console.log("js found: ");
          console.log(total2);
          console.log(at2);
          console.log(sendZeros);
          console.log('lllllllllllllllllllllllllll');
          console.log('lllllllllllllllllllllllllll');
          console.log('lllllllllllllllllllllllllll');

          this.sjs.sendZerosToHiggs(sendZeros);

          let afterFlush = () => {
            let level = this.sjs.flushHiggs.getNormalLen();
            console.log("level: " + level);

            if( level > 0 ) {
              setTimeout(afterFlush, 100);
            } else {
              this.oldStyleReset();
            }

          };

          setTimeout(afterFlush, 100);



          // setTimeout(()=> {
          //   console.log("blind ok");
          //   this.sjs.settings.setBool('runtime.js.recover_fb.result', true);
          //   this.recovering = false;
          // }, 7000);

          

          // console.log('hanging');


        } else {

          console.log("js: resetting mapmov");
          this.sjs.ringbus(hc.RING_ADDR_ETH, hc.MAPMOV_RESET_CMD);
          this.sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.MAPMOV_RESET_CMD);



          let markOk = () => {
            console.log("all ok");
            this.sjs.settings.setBool('runtime.js.recover_fb.result', true);
            this.recovering = false;
          };

          let zrsAfterReset = () => {
            console.log("js: zrsAfterReset");

            this.sjs.sendZerosToHiggs(1024*5);
            setTimeout(markOk, 1);
          };

          setTimeout(zrsAfterReset, 1000);


        }

      },300);
    }
  }

  oldStyleReset() {
    this.sjs.flushHiggs.dumpLow();

    this.sjs.ringbus(hc.RING_ADDR_ETH, hc.MAPMOV_RESET_CMD);
    this.sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.MAPMOV_RESET_CMD);

    this.sjs.sendZerosToHiggs(1024);

    this.last_alive_count = this.sjs.attached.feedback_alive_count;

    let doCheckAlive = () => {
      let alive_count = this.sjs.attached.feedback_alive_count;

      console.log("");
      if( alive_count > this.last_alive_count ) {
        console.log("-------------------------------------------");
        console.log("Attached Feedback Bus is Alive");
        console.log("-------------------------------------------");
        this.sjs.settings.setBool('runtime.js.recover_fb.result', true);
        this.recovering = false;
      } else {
        console.log("-------------------------------------------");
        console.log("js did not get check alive reply");
        console.log("-------------------------------------------");
      }

    };

    let sendStatusReply = () => {
      this.sjs.sendToHiggs(this.checkAlive);
      setTimeout(doCheckAlive, 100);
    };

    setTimeout(sendStatusReply, 100);

  }

  register() {
    this.sjs.ringbusCallbackCatchType(hc.CS02_FB_REPORT_PCCMD);
    this.sjs.ringbusCallbackCatchType(hc.CS02_FB_REPORT2_PCCMD);
    this.sjs.ringbusCallbackCatchType(hc.CS02_REPORT_STATUS_PCCMD);
    this.sjs.ringbusCallbackCatchType(hc.ETH_STATUS_PCCMD);
  }

  handle1(word) {
    let o = this.captures[this.capCount];
    let type = (word & 0x00f00000) >>> 20;
    let value = (word & 0xfffff) >>> 0;

    let e = o.fb.errors;
    let c = o.fb.counters;

    switch(type) {
      case 0:
        e.incorrect_types = value;
        break;
      case 1:
        e.odd_rb_length = value;
        break;
      case 2:
        e.small_vector_length = value;
        break;
      case 3:
        e.large_vector_length = value;
        break;
      case 4:
        e.unhandled_vtype = value;
        break;
      case 5:
        e.unhandled_stype = value;
        break;
      case 6:
        e.unset_callback = value;
        break;
      case 7:
        c.stream = value;
        break;
      case 8:
        c.vector = value;
        break;
      case 9:
        c.ringbus = value;
        break;
      case 10:
        e.ringbus_queue_full = value;
        break;
      default:
        console.log('HandleCS02Tx.handle1 illegal type ' + type);
        break;
    }
  }

  handle2(word) {
    let o = this.captures[this.capCount];
    let type = (word & 0x00ff0000) >>> 16;
    let value = (word & 0xffff) >>> 0;
    let valueSigned = ((word & 0xffff) << 16) >> 16;

    // console.log("type: " + type + " value: " + valueSigned);

    let mapmov = o.fb.status.mapmov;
    let dma = o.fb.status.dma_in;
    let w1 = o.fb.status.dma_in_work;
    let w2 = o.fb.status.parse_work;

    // if( typeof o.mapmpv === 'undefined' )

    switch(type) {
      case 0:
        mapmov.total_size = value;
        break;
      case 1:
        mapmov.total_chunks = value;
        break;
      case 2:
        mapmov.this_chunk = value;
        break;
      case 3:
        mapmov.chunk_enqueued = value;
        break;
      case 4:
        mapmov.fft_size = valueSigned;
        break;
      case 5:
        mapmov.pause_operations = valueSigned;
        break;
      case 6:
        dma.body_signal = valueSigned;
        break;
      case 7:
        dma.mapmov_signal = valueSigned;
        break;
      case 8:
        dma.mapmov_header_signal = valueSigned;
        break;
      case 9:
        dma.allowed_additional_occupancy = valueSigned;
        break;
      case 10:
        dma.expected_occupancy = valueSigned;
        break;
      case 11:
        o.fb.status.dumping_zeros_mode = valueSigned;
        break;
      case 12:
        o.fb.status.dumping_zeros_counted = valueSigned;
        break;
      case 13:
        dma.schedule_occupancy = valueSigned;
        break;
      case 14:
        w1.state = valueSigned;
        break;
      case 15:
        w1.a_empty = valueSigned;
        break;
      case 16:
        w1.b_empty = valueSigned;
        break;
      case 17:
        w2.state = valueSigned;
        break;
      case 18:
        w2.a_empty = valueSigned;
        break;
      case 19:
        w2.b_empty = valueSigned;
        break;
      default:
        console.log('HandleCS02Tx.handle2 illegal type ' + type);
        break;
    }

  }

  handle3(word) {
    let o = this.captures[this.capCount];
    let type = (word & 0x00ff0000) >>> 16;
    let value = (word & 0xffff) >>> 0;
    let valueSigned = ((word & 0xffff) << 16) >> 16;

    // console.log("type: " + type + " value: " + valueSigned);

    let cs = o.cs11;
    let ud = o.cs11.ud;
    let td = o.cs11.tdma;

    // if( typeof o.mapmpv === 'undefined' )

    switch(type) {
      case 0:
        cs.frame_counter = valueSigned;
        break;
      case 1:
        cs.lifetime_frame_counter = value;
        break;
      case 2:
        cs.litetime_32_l = value;
        break;
      case 3:
        cs.litetime_32_u = value;
        break;
      case 4:
        cs.pending_data = valueSigned;
        break;
      case 5:
        cs.pending_timeslot = valueSigned;
        break;
      case 6:
        cs.pending_length = valueSigned;
        break;
      case 7:
        cs.output_frame_count = valueSigned;
        break;
      case 8:
        cs.epoc_needs_latch = valueSigned;
        break;
      case 9:
        cs.epoc_was_latched = valueSigned;
        break;
      case 10:
        cs.progress_report_remaining = valueSigned;
        break;
      case 11:
        ud.callbacks = value;
        break;
      case 12:
        ud.state = valueSigned;
        break;
      case 13:
        ud.timeslot = valueSigned;
        break;
      case 14:
        ud.epoc = valueSigned;
        break;
      case 15:
        ud.total_chunks = value;
        break;
      case 16:
        ud.chunk_index_consumed = value;
        break;
      case 17:
        ud.chunk_length = value;
        break;
      case 18:
        ud.asked_for_pause = valueSigned;
        break;
      case 19:
        cs.pending_lifetime_update = valueSigned;
        break;
      case 20:
        cs.incoming_future_lifetime_counter = value;
        break;
      case 21:
        ud.needs_fill_report = valueSigned;
        break;
      case 22:
        cs.run_sfo = valueSigned;
        break;
      case 23:
        td.mode = valueSigned;
        break;
      case 24:
        td.mode_pending = valueSigned;
        break;
      default:
        console.log('HandleCS11Tx.handle3 illegal type ' + type);
        break;
    }
  }

  handle_eth(word) {
    let o = this.captures[this.capCount];
    let type = (word & 0x00ff0000) >>> 16;
    let value = (word & 0xffff) >>> 0;
    let valueSigned = ((word & 0xffff) << 16) >> 16;

    let e = o.eth;
    let mac = e.mac;
    let unsup = e.unsupported;
    let cs11 = e.cs11;
    let cs20 = e.cs20;
    let rb = e.rb;
    // let c = o.fb.counters;

    // eth sends high low on the same value
    // so handle a bit differently

    // let count = this.eth_count[type];

    if( typeof this.eth_count[type] === 'undefined' ) {
      this.eth_count[type] = 0;
    }

    if( this.eth_count[type] === 0 ) {
      this.eth_value[type] = value<<16;
      this.eth_count[type] = 1;
    } else if ( this.eth_count[type] === 1 ) {
      this.eth_value[type] = (this.eth_value[type] | value) >>> 0;
      this.eth_count[type] = 2;
    } else if ( this.eth_count[type] === 2) {
      console.log("this.eth_count[type] was too large: " << this.eth_count[type]);
      this.eth_count[type] = 3;
    } else {
      console.log("this.eth_count[type] was too large: " << this.eth_count[type]);
    }

    // this.eth_value = [];
    // this.eth_count = [];

    if( this.eth_count[type] !== 2 ) {
      return;
    }

    let value32 = this.eth_value[type];


    switch(type) {
      case 1:
        mac.rx_err = value32;
        break;
      case 2:
        mac.rx_crc_err = value32;
        break;
      case 3:
        mac.rx_len_chk_err = value32;
        break;
      case 4:
        mac.rx_pkt_rx_ok = value32;
        break;
      case 5:
        unsup.eth_type_err = value32;
        break;
      case 6:
        unsup.ipv4_protocol = value32;
        break;
      case 7:
        unsup.dest_port = value32;
        break;
      case 8:
        cs11.data_buf_overflow = value32;
        break;
      case 9:
        cs11.data_buf_underflow = value32;
        break;
      case 0xa:
        rb.in_buf_underflow = value32;
        break;
      case 0xb:
        rb.in_buf_overflow = value32;
        break;
      case 0xc:
        cs20.data_buf_overflow = value32;
        break;
      case 0xd:
        rb.out_buf_overflow = value32;
        break;

        
        break;
      default:
        console.log('HandleCS11Tx.handle_eth illegal type ' + type);
        break;
    }
  }

  rbcb(word) {
    let found = false;
    let cmd_type = (word & 0xff000000) >>> 0;
    switch(cmd_type) {
      case hc.CS11_FB_REPORT_PCCMD:
        this.handle1(word);
        found = true;
        break;
      case hc.CS11_FB_REPORT2_PCCMD:
        this.handle2(word);
        found = true;
        break;
      case hc.CS11_REPORT_STATUS_PCCMD:
        this.handle3(word);
        found = true;
        break;
      case hc.ETH_STATUS_PCCMD:
        this.handle_eth(word);
        found = true;
        break;
    }

    // if( found ) {
    //   console.log("FOUND: 0x" + word.toString(16));
    // }

    return found;
  }

  pushDefault() {
    let o = {
      fb:{
        status:{
          mapmov:{}
          ,dma_in:{}
          ,dma_in_work:{}
          ,parse_work:{}
        }
        ,errors:{}
        ,counters:{}
      }
      ,cs11:{
        ud:{}
        ,tdma:{}
      }
      ,eth:{
        mac:{}
        ,unsupported:{}
        ,cs11:{}
        ,rb:{}
        ,cs20:{}
      }
    };
    this.captures.push(o);
    this.rbDuringCapture.push(0);

    this.eth_value = [];
    this.eth_count = [];
  }

  printCapture() {
    let o = this.captures[this.capCount];

    console.log(util.inspect(o, {showHidden: false, depth: null}));
  }


  check() {
    this.capCount++;
    this.pushDefault();

    //console.log(this.captures[this.capCount]);

    this.sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.CS02_REPORT_ERRORS_CMD );

    setTimeout(()=>{
      this.sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.FB_REPORT_STATUS_CMD );
    }, 50);

    setTimeout(()=>{
      this.sjs.ringbus(hc.RING_ADDR_TX_PARSE, hc.CS02_MAIN_REPORT_STATUS_CMD );
    }, 100);

    setTimeout(()=>{
      this.sjs.ringbus(hc.RING_ADDR_ETH, hc.ETH_GET_STATUS_CMD )
    }, 150);
  }

  checkPrint() {
    this.check();
    setTimeout(()=>{this.printCapture()}, 500);
  }


  resetMapMov() {
    this.sjs.ringbus(hc.RING_ADDR_ETH, hc.MAPMOV_RESET_CMD );
  }
}

module.exports = {
    HandleFeedbackBusRecovery
}