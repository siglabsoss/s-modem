'use strict';

const hc = require('../../js/mock/hconst.js');
const util = require('util');


class EqReadback {
  constructor(sjs) {
    this.sjs = sjs;
    // console.log('HandleCS20Tx boot');

    this.capCount = 0;
    this.captures = [];

    this.register();

  }

  register() {
    this.sjs.ringbusCallbackCatchType(hc.CS02_USERDATA_ERROR);
  }

  handle1(word) {
    let type = (word & 0x000000ff) >>> 0;
    let funny_dmode =      (word & 0xffff00) >>> 8;

    if( type === 0x16 || type === 0x17 ) {
      // ok
    } else {
      return;
    }

    this.captures.push(funny_dmode);

    if( this.captures.length === 2048 ) {
      let build = 0;
      for(let i = 0; i < 2048; i++) {
        if( i % 2 == 0 ) {
          build = this.captures[i] << 16;
        } else {
          build |= this.captures[i];
          build = build >>> 0;
          console.log(build.toString(16).padStart(8,'0'));
        }
      }
      this.captures = [];
    }

  }

  rbcb(word) {
    let found = false;
    let cmd_type = (word & 0xff000000) >>> 0;
    switch(cmd_type) {
      case hc.CS02_USERDATA_ERROR:
        this.handle1(word);
        found = true;
        break;
    }
    return found;
  }
}

module.exports = {
    EqReadback
}
