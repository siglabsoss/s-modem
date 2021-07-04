'use strict';


const hc = require('../../js/mock/hconst.js');
// const util = require('util');


class HandleDuplex {
  constructor(sjs) {
    this.sjs = sjs;

    this.sjs.mode2 = this.mode2.bind(this);
  }










  // do not rename this without updating getTxString()
  asTx() {
    const sjs = this.sjs;

// tx
sjs.attached.sfo_sto_use_duplex = true
sjs.ringbus(hc.RING_ADDR_RX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_1,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_2,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_4,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_TX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_TX_1,  hc.COOKED_DATA_TYPE_CMD | 2);


// tx
// sjs.dsp._rx_should_insert_sfo_cfo = false;  // stop tx from updating
// sjs.attached.radio_state_pending = hc.DEBUG_STALL // stop tx from updating
sjs.ringbus(hc.RING_ADDR_CS32,  hc.EQ_DATA_RX_CMD | 2);
sjs.current(3)
sjs.ringbus(hc.RING_ADDR_TX_0, hc.APP_BARREL_SHIFT_CMD | 0x20000 | 0x10) // tx correction bs
sjs.dsp.op(hc.RING_ADDR_TX_0, "set", 13, 10000) // enable tx eq correction
//sjs.ringbus(hc.RING_ADDR_CS32, hc.APP_BARREL_SHIFT_CMD | 0x20000 | 0x3); // "add" for fine sync


// tx 
sjs.ringbus(hc.RING_ADDR_CS22, hc.RX_PHASE_CORRECTION_CMD | 0x0 );
// sjs.ringbus(hc.RING_ADDR_CS22, hc.RX_PHASE_CORRECTION_CMD | 0x00010001 ); // worse performance with this on
sjs.ringbus(hc.RING_ADDR_CS22, hc.IIR_COEFF_STATE_TX_CMD|0x7a00); // Filter for FF
sjs.ringbus(hc.RING_ADDR_CS32, hc.IIR_COEFF_STATE_TX_CMD|0x7a00); // Filter for PP
sjs.attached.setAllEqMask(sjs.masks.narrow_1);
sjs.attached.updatePartnerEq(false, false);

// tx
sjs.ringbus(hc.RING_ADDR_CS32, hc.APP_BARREL_SHIFT_CMD | 0x60000 | 0xf);

sjs.ringbus(hc.RING_ADDR_CS32, hc.APP_BARREL_SHIFT_CMD | 0x10000 | 0x13);
sjs.ringbus(hc.RING_ADDR_CS32, hc.APP_BARREL_SHIFT_CMD | 0x20000 | 0x1);

}


  asRx() {
    const sjs = this.sjs;

    // rx
sjs.ringbus(hc.RING_ADDR_RX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_1,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_2,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_4,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_TX_0,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_TX_1,  hc.COOKED_DATA_TYPE_CMD | 2);
sjs.ringbus(hc.RING_ADDR_RX_2, hc.RX_PHASE_CORRECTION_CMD | 0 );
sjs.ringbus(hc.RING_ADDR_TX_0, hc.APP_BARREL_SHIFT_CMD | 0x30000 | 0x10) // rx analog bs


// rx 
sjs.ringbus(hc.RING_ADDR_CS22, hc.IIR_COEFF_STATE_RX_CMD|0x7a00); // Filter for pp
sjs.ringbus(hc.RING_ADDR_CS32, hc.IIR_COEFF_STATE_RX_CMD|0x7a00);
sjs.ringbus(hc.RING_ADDR_CS32, hc.RX_EQ_FOR_DATA_CMD|0);
sjs.attached.setAllEqMask(sjs.masks.narrow_1);
sjs.attached.updatePartnerEq(false, false);

// Rx node for mode 2: i,d
sjs.fixedGrc([2,987-2,1007,987])
sjs.dsp.op(hc.RING_ADDR_CS20, "set", 1, 18)


}






  mode2() {
    const sjs = this.sjs;
    let error = '';
    if( sjs.role !== 'rx') {
      error = 'This is only setup to run from the rx side at the moment';
      return error;
    }

    // dispatch this to each tx side
    for(let peer_id of sjs.allPeers() ) {
      sjs.zmq.id(peer_id).eval(this.getTxString()).then(()=>{});
    }

    // do the rx side
    this.asRx();

    return;
  }


  // right now assume this runs from the rx side
  // this means that the tx side needs to be sent through 
  // zmq.r().eval
  // because of this, we need to convert the function into a string
  // and then send it over
  // when we use javascript to convert it to a string it has some extra
  // characters that we want to delete
  // this function does that
  getTxString() {
    let r = this.asTx.toString();

    // pull off the 'asTx() '
    // at the beginning
    r = r.slice(7);

    // wrap and call
    r = '(()=>' + r + ')()';

    return r;
  }

}


module.exports = {
    HandleDuplex
}
