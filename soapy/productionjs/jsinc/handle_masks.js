'use strict';

const util = require('util');


class HandleMasks {
  constructor(sjs) {
    this.sjs = sjs;

    // Event Emitter ringbus
    // this.notify = new DispatchRingbusEE();

    this.sjs.masks = {};
    this.setupMasks();
  }


  setupMasks() {

    let mm = this.sjs.masks;

    mm.m0 = [];
    for(let i = 0; i<1024; i++) {
      let c = i < 64 || i > (1024-64) || (i == 21) ;
      mm.m0.push(c?0:1);
    }

    mm.m0_neg = [];
    for(let i = 0; i<1024; i++) {
      let c = i > (1024-64) || (i == 21);
      mm.m0_neg.push(c?0:1);
    }

    mm.fine_sync = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-128) && (i % 4) == 2 ) || (i == 21);
      mm.fine_sync.push(c?0:1);
    }

    mm.fine_sync_debug = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-64) ) || (i == 21);
      mm.fine_sync_debug.push(c?0:1);
    }

    mm.fine_sync_debug2 = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-32) );
      mm.fine_sync_debug2.push(c?0:1);
    }

    mm.fine_sync_debug3 = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-16) );
      mm.fine_sync_debug3.push(c?0:1);
    }


    mm.m1 = [];
    for(let i = 0; i<1024; i++) {
      let c = i < 16 || i > (1024-16);
      mm.m1.push(c?0:1);
    }

    mm.m2 = [];
    for(let i = 0; i<1024; i++) {
      let c = i < 16 || i > (1024-16); // true meens keep
      if( (i % 2 == 0) && (i < 32 || i > (1024-32)) ) {
        c |= true;
      }
      mm.m2.push(c?0:1);
    }

    mm.m3 = [];
    for(let i = 0; i<1024; i++) {
      let c = i < 4 || i > (1024-4);
      mm.m3.push(c?0:1);
    }

    mm.m4 = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-128)) && ((i % 4) == 2) ;
      mm.m4.push(c?0:1);
    }

    mm.m4all = [];
    for(let i = 0; i<1024; i++) {
      let c = (i > (1024-128)) ;
      mm.m4all.push(c?0:1);
    }

    mm.m5 = [];
    for(let i = 0; i<1024; i++) {
      let c = ( ((i < (128)) || (i > (1024-128))) && ((i % 4) == 2) ) ;
      mm.m5.push(c?0:1);
    }

    // non zero deletes the subcarrier
    // c means "keep" where zero deletes the subcarrier
    mm.narrow_1 = [];
    for(let i = 0; i<1024; i++) {
      let c = 0;
      if(i === 1022 || (i>=16 && i < 80) ) {
        c = 1;
      }
      mm.narrow_1.push(c?0:1);
    }
  }
}


module.exports = {
    HandleMasks
}
