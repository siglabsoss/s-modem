'use strict'


const hc = require('../../js/mock/hconst.js');
const fs = require('fs');
const { DateTime } = require('luxon');
const listenGrc = require('../../productionjs/jsinc/listen_grc_port.js');

function listenGrcCb(sjs, data) {
  console.log("Tuning to custom subcarrier " + data);

  let sc = data % 1024;

  sjs.ringbus(hc.RING_ADDR_CS20, hc.CS21_CHOOSE_CUSTOM_SC_CMD | sc );
}

class TxRxSharedBehavior {
  constructor(sjs) {
    this.sjs = sjs;
    this.setupUart();
    console.log('TxRxSharedBehavior boot');
    this.setupTestRing();
    this.handleSpam = true;
    if( this.handleSpam ) {
      this.setupSpam();
    }
    this.setupFinalBase();
    this.setupOthersBase();
  }

  setupFinalBase() {
    // other setup junk
    console.log("setup final base");
    this.sjs.hrb.on(hc.DEBUG_OTA_FRAME_PCCMD, this.handleDebugFrameSc.bind(this));

    this.printUart = false;
    this.sjs.hrb.on(hc.UART_READOUT_PCCMD, this.handleUartRing.bind(this));

    this.sjs.hrb.on(hc.GENERIC_PCCMD, this.handleTestRing.bind(this));
  }

  setupTestRing() {
    this.test_ring_res = [];
    
    this.ring_targets = [
     hc.RING_ADDR_CS11
    ,hc.RING_ADDR_CS01
    ,hc.RING_ADDR_CS02
    ,hc.RING_ADDR_CS12
    ,hc.RING_ADDR_CS22
    ,hc.RING_ADDR_CS32
    ,hc.RING_ADDR_CS31
    ,hc.RING_ADDR_CS21
    ,hc.RING_ADDR_CS20
    ];

    const msg = 0x00dead00;
    this.ring_msgs = [];

    for(let x of this.ring_targets) {
      this.ring_msgs.push( msg | (hc.RING_BUS_LENGTH - 2 - x) ) ;
    }
  }

   setupOthersBase() {
    this.listenGrc = new listenGrc.ListenGrcPort({printConnections:false, cb:(d)=>listenGrcCb(this.sjs,d)});
  }



  handleDebugFrameSc(word) {

    let frame = (word & 0x0f00000) >>> 20;
    let mag = (word & 0xfffff) >>> 0;



    console.log("frame: " + frame.toString(16) + " mag: " + mag.toString(16));
    if( frame == 0xe ) {
      console.log('');
      console.log('');
    }
  }

  setupUart() {
    this.uart_chars = [];
  }


  handleUartRing(word) {
    const code =  (word & 0x0000ffff)>>>0;
    const c = String.fromCharCode(code);

    let save = true;
    let flush = false;

    if(  c == "\r" ) {
      // console.log('newline R');
      save = false;
    }

    if( c == "\n" || c == "\r" ) {
      save = false;
      flush = true;
      // console.log('newline N');
    }

    if( save ) {
      this.uart_chars.push(c);
    }

    if( flush ) {
      if( this.uart_chars.length > 0 ) {
        if( this.printUart ) {
          let msg = this.uart_chars.join('');
          console.log('UART: ' + msg);
        }
        // console.log(this.uart_chars);
        this.uart_chars = [];
      }
    }


    // console.log("Uart: " + c);
  }

  test_ring() {
    this.test_ring_res = [];

    const len = this.ring_targets.length;

    for(let i = 0; i < len; i++) {
      let f = this.ring_targets[i];
      let m = this.ring_msgs[i];
      this.sjs.t_ringbus(f, m);
      // console.log('Packet sent: ' + f.toString(16) + ', ' + m.toString(16));
    }

    setTimeout(this.handleTestRingResults.bind(this), 300);
  }

  handleTestRing(x) {
    this.test_ring_res.push(x);
  }

  handleTestRingResults() {
    console.log("Test Ring Got " + this.test_ring_res.length);
    let pass = true;

    const len = this.ring_targets.length;

    for(let i = 0; i < len; i++) {
      // console.log("i: " + i);
      let fpga = this.ring_targets[i];
      let expected = this.ring_msgs[i];
      // let got = this.test_ring_res[i];

      // look for expected in got and not the other way around
      let found = this.test_ring_res.indexOf(expected) !== -1;

      let name = this.sjs.ringAddrToName(fpga);

      // console.log(name);

      if( !found ) {
        console.log(name + " fail!!!!!!");
        pass = false;
      } else {
        console.log(name + " passed");
      }
      // console.log(expected.toString(16));
    }

    if( pass ) {
      console.log("Test Ring - All FPGA passed");
    } else {
      console.log("Test Ring - SOME FAILED");
    }

  }


  handleRingbus(x) {
    // const type = x & 0xff000000;
    // if( type === hc.UART_READOUT_PCCMD ) {
    //   this.handleUartRing(x);
    //   return true;
    // }
    return false;
  }


  gotSpamRb(x) {
    this.spamCount += 1;
  }

  spamTimer() {
    if( this.spamCount != this.lastSpamCount ) {
      let d = this.spamCount - this.lastSpamCount;

      console.log("Getting spam at rate of: " + d );

      this.lastSpamCount = this.spamCount;
    } else {

      if( this.spamCount != 0 ) {
        // see https://moment.github.io/luxon/docs/manual/tour.html
        let time = DateTime.local().toLocaleString(DateTime.DATETIME_MED_WITH_SECONDS);

        console.log("Seems rx is down at " + time);
        this.lastSpamCount = 0;
        this.spamCount = 0;
      }
    }
  }


  spamSendTimer() {
    if( this.spamSendRate == 0 ) {
      return;
    }


  }


  setupSpam() {
    this.lastSpamCount = 0;
    this.spamCount = 0;
    this.spamInterval = setInterval(this.spamTimer.bind(this), 1000);
    this.sjs.hrb.on(hc.DEBUG_18_PCCMD, this.gotSpamRb.bind(this));

    // this.spamSendRate = 0;
    // this.spamSendInterval = setInterval(this.spamSendTimer.bind(this), 50);
  }
 


}

module.exports = {
    TxRxSharedBehavior
}
