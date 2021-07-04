'use strict';

const hc = require('../../js/mock/hconst.js');
const g3ctrl = require('./grav3_ctrl.js');

class WrapNativeSmodem {
  constructor(sjs, role) {
    this.sjs = sjs;
    this.sjs.settings = {};

    // these need bind
    this.sjs.settings.setBool = this.setBool.bind(this);
    this.sjs.settings.setInt = this.setInt.bind(this);
    this.sjs.settings.setDouble = this.setDouble.bind(this);
    this.sjs.settings.setString = this.setString.bind(this);

    if( role != "arb") {
      this.sjs.ringbus = this.ringbus.bind(this);
      this.sjs.t_ringbus = this.t_ringbus.bind(this);

      this.sjs.enableDataToJs = this.enableDataToJs.bind(this);
      this.sjs.disableDataToJs = this.disableDataToJs.bind(this);
    }

    this.sjs.customEvent.send = this.sendEvent.bind(this);
    this.sjs.debugTxFill = this.debugTxFill.bind(this);



    // mirror these under settings for consistency
    this.sjs.settings.getDoubleDefault = this.sjs.getDoubleDefault;
    this.sjs.settings.getIntDefault = this.sjs.getIntDefault;
    this.sjs.settings.getBoolDefault = this.sjs.getBoolDefault;
    this.sjs.settings.getStringDefault = this.sjs.getStringDefault;

    // helpers etc
    this.sjs.allPeers = this.allPeers.bind(this);
    this.sjs.txPeers = this.allPeers.bind(this);
    this.sjs.ringAddrToName = this.ringAddrToName.bind(this);

    // duplex
    this.sjs.cookedDataType = this.cookedDataType.bind(this);
    this.sjs.duplexFeedback = this.duplexFeedback.bind(this);
    this.sjs.duplexFilters = this.duplexFilters.bind(this);
    this.sjs.duplex = this.duplex.bind(this);

    this.sjs.pickBs = this.pickBs.bind(this);
    this.sjs.bsUnprotectedSingle = this.bsUnprotectedSingle.bind(this);
    this.sjs.bs = this.bs.bind(this);

    // grav3 helpers
    this.sjs.tx = this.tx.bind(this);
    this.sjs.rx = this.rx.bind(this);
    this.sjs.safe = this.safe.bind(this);
    this.sjs.dsa = this.dsa.bind(this);
    this.sjs.current = this.current.bind(this);

    // attached radio
    this.sjs.attached = this.sjs.r[99]; // 99 is ATTACHED_INDEX from BindRadioEstimate.cpp


    // Feedback Bus
    this.sjs.test_fb = this.sjs.dsp.test_fb;

    // other
    this.sjs.fixedGrc = this.fixedGrc.bind(this);
    this.sjs.clog = this.clog.bind(this);

    this.sjs.ddd = this.ddd.bind(this);


  }

  // Tx mode always on channel A
  tx() {
    this.sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.tx_state('a') );
  }

  // Rx mode always on channel B
  rx() {
    this.sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.rx_state('b', 0) );
  }

  safe() {
    this.sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | g3ctrl.all_safe() );
  }

  dsa(x) {
    const words = g3ctrl.channel_b({state:'rx',dsa:x});
    for( let x of words ) {
      this.sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | x );
    }
    // g3ctrl.channel_b('b', 0)
  }

  current(x) {
    const words = g3ctrl.channel_a({state:'tx',current:x});
    for( let x of words ) {
      this.sjs.ringbus(hc.RING_ADDR_ETH, hc.UART_PUT_CHAR_CMD | x );
    }
  }


  // alias with default value of 0
  ringbus(fpga, word) {
    this.sjs.ringbusLater(fpga, word, 0 );
    const type = (word & 0xff000000);
    if( type == 0 ) {
      console.log("!!!!!!!!!!!!!!!!!!!!!!!!! Warning: Sending Ringbus with no type !!!!!!!!!!!!!!!!!!!!!!!!!");
    }
  }

  // for test_eth and test_ring
  t_ringbus(fpga, word) {
    this.sjs.ringbusLater(fpga, word, 0 );
  }

  setBool(path, value) {
    let pathStrArray = path.split('.');

    let o = [];
    o[0]={u:1000};
    o[1]={'k':pathStrArray,'t':'bool',v:!!value};

    this.sjs.dispatchMockRpc(JSON.stringify(o));
  }

  setInt(path, value) {
    let pathStrArray = path.split('.');

    let o = [];
    o[0]={u:1000};
    o[1]={'k':pathStrArray,'t':'int',v:Math.trunc(value)};

    this.sjs.dispatchMockRpc(JSON.stringify(o));
  }

  setDouble(path, value) {
    let pathStrArray = path.split('.');

    let o = [];
    o[0]={u:1000};
    o[1]={'k':pathStrArray,'t':'double',v:parseFloat(value)};

    this.sjs.dispatchMockRpc(JSON.stringify(o));
  }

  setString(path, value) {
    let pathStrArray = path.split('.');

    let o = [];
    o[0]={u:1000};
    o[1]={'k':pathStrArray,'t':'string',v:""+value};

    this.sjs.dispatchMockRpc(JSON.stringify(o));
  }

  enableDataToJs() {
    this.setBool('runtime.data.to_js', true);
  }

  disableDataToJs() {
    this.setBool('runtime.data.to_js', false);
  }

  // unsure if this works
  sendEvent(u1, u2) {
    let o = [];
    o[0]={u:2};
    o[1]={'v1':u1,'v2':u2};

    this.sjs.dispatchMockRpc(JSON.stringify(o));
  }

  debugTxFill() {
    this.sjs.settings.setBool('runtime.tx.trigger_warmup', true);
  }

  allPeers() {
    return this.sjs.getTxPeers();
  }

  ringAddrToName(x) {
    switch(x) {
      case hc.RING_ADDR_CS11:
        return 'CS11';
      case hc.RING_ADDR_CS01:
        return 'CS01';
      case hc.RING_ADDR_CS02:
        return 'CS02';
      case hc.RING_ADDR_CS12:
        return 'CS12';
      case hc.RING_ADDR_CS22:
        return 'CS22';
      case hc.RING_ADDR_CS32:
        return 'CS32';
      case hc.RING_ADDR_CS31:
        return 'CS31';
      case hc.RING_ADDR_CS21:
        return 'CS21';
      case hc.RING_ADDR_CS20:
        return 'CS20';
      default:
        return '?';
    }
  }

  cookedDataType(x) {
    this.sjs.ringbus(hc.RING_ADDR_RX_0,  hc.COOKED_DATA_TYPE_CMD | x);
    this.sjs.ringbus(hc.RING_ADDR_RX_1,  hc.COOKED_DATA_TYPE_CMD | x);
    this.sjs.ringbus(hc.RING_ADDR_RX_2,  hc.COOKED_DATA_TYPE_CMD | x);
    this.sjs.ringbus(hc.RING_ADDR_RX_4,  hc.COOKED_DATA_TYPE_CMD | x);
    this.sjs.ringbus(hc.RING_ADDR_TX_0,  hc.COOKED_DATA_TYPE_CMD | x);
    this.sjs.ringbus(hc.RING_ADDR_TX_1,  hc.COOKED_DATA_TYPE_CMD | x);
  }

  duplexFeedback() {
    let sjs = this.sjs;
    if( sjs.role === 'rx' ) {
      this.sjs.ringbus(hc.RING_ADDR_TX_0, hc.APP_BARREL_SHIFT_CMD | 0x30000 | 0x10) // rx analog bs
    } else if ( sjs.role === 'tx' ) {
      this.sjs.ringbus(hc.RING_ADDR_TX_0, hc.APP_BARREL_SHIFT_CMD | 0x20000 | 0x10) // tx correction bs
    }
  }

  duplexFilters() {
    if( sjs.role === 'rx' ) {
      sjs.ringbus(hc.RING_ADDR_CS22, hc.IIR_COEFF_STATE_RX_CMD|0x7a00); // Filter for pp

      // rx 
      sjs.ringbus(hc.RING_ADDR_CS32, hc.IIR_COEFF_STATE_RX_CMD|0x7a00);
      sjs.ringbus(hc.RING_ADDR_CS32, hc.RX_EQ_FOR_DATA_CMD|1);
    } else if ( sjs.role === 'tx' ) {
      // tx 
      sjs.ringbus(hc.RING_ADDR_CS22, hc.RX_PHASE_CORRECTION_CMD | 0x00010001 );
      sjs.ringbus(hc.RING_ADDR_CS22, hc.IIR_COEFF_STATE_TX_CMD|0x7a00); // Filter for FF
      sjs.ringbus(hc.RING_ADDR_CS32, hc.IIR_COEFF_STATE_TX_CMD|0x7a00); // Filter for PP
    }
  }

  duplex(x) {
    let sjs = this.sjs;
    if( !x ) {
      sjs.cookedDataType(0);
      return;
    }

    if( sjs.role === 'rx' ) {

    } else if ( sjs.role === 'tx' ) {
      sjs.attached.sfo_sto_use_duplex = true;
    }

    sjs.duplexFeedback();

    sjs.cookedDataType(2);



    if( sjs.role === 'rx' ) {
      sjs.ringbus(hc.RING_ADDR_RX_2, hc.RX_PHASE_CORRECTION_CMD | 0 );
    } else if ( sjs.role === 'tx' ) {
    }
  }



////////////////////// bs

  pickBs(value) {
    let bs = [0,0,0,0,0];
    for(let i = 0; i < value; i++) {
//         let foo = Math.floor(i/5);
        let bar = i % 5;
        bs[bar]++;
    }
    
//     console.log(bs);
    return bs;
  }


  bsUnprotectedSingle(chain, sset, values, print = true) {
    let sjs = this.sjs;
    if( print ) {
        let p = "Barrelshift: ";
        switch(sset) {
            case 0:
                console.log(p + "Default");
                break;
            case 1:
                console.log(p + "DL Pilot 'p'");
                break;
            case 2:
                console.log(p + "UL Pilot 'P'");
                break;
            case 3:
                console.log(p + "UL Feedback 'F'");
                break;
            case 4:
                console.log(p + "DL Beam Pilot 'i'");
                break;
            case 5:
                console.log(p + "DL Beam Data 'd'");
                break;
            default:
                console.log(p + "?????????");
                break;
        }
    }

    if( chain !== 'tx' && chain !== 'rx' ) {
      console.log("Argument of " + chain + " is wrong");
      return;
    }
    
    let setBump = sset * 5;
    let rb = hc.FFT_BARREL_SHIFT_CMD;
    
    let target = hc.RING_ADDR_TX_FFT;

    if( chain === 'rx' ) {
      target = hc.RING_ADDR_RX_FFT;
    }


    for(let i = 0; i < 5; i++) {
      let idx = ((i+setBump) << 16) >>> 0;
      let build = (((((rb | rb)>>>0)|idx) >>> 0) | values[i])>>>0;
      
      sjs.dsp.ringbusPeerLater(sjs.id, target, build, 1);

      if( print ) {
        console.log('rb: ' + build.toString(16));
      }
    }
  }




  bs(chain, sset, values, print = true) {
    let valuesOk = false;
    if( Array.isArray(values) ) {
//         console.log('is arr');
      if( values.length === 5 ) {
        valuesOk = true;
      }
    } else {
      if( Number.isInteger(values) ) {
//             if( print ) {
//                 console.log("Values is a number, will automatically pick bs");
//             }
        values = this.pickBs(values);
        valuesOk = true;
      }
    }
    
    if( !valuesOk ) {
      console.log("Values argument is not correct");
      return;
    }
    
    if( (!Number.isInteger(sset))&&(!Array.isArray(sset)) && (sset.toLowerCase() === 'all')) {
      sset = [0, 1, 2, 3, 4];
    }

    
    if( Array.isArray(sset) ) {
        let i = 0;
        for(let x of sset) {
                if( x > 5 ) {
                    console.log("Set argument[" + i + "] = " + x + " is too large, skipping");
                    continue;
                }
            this.bsUnprotectedSingle(chain,x,values,print);
            i++;
        }
    } else {
        if( !Number.isInteger(sset) ) {
            console.log("Set argument must be a number or array or the string 'all'");
            return;
        }
        if( sset > 5 ) {
            console.log("Set argument " + sset + " is too large");
            return;
        }
        this.bsUnprotectedSingle(chain,sset,values,print);
    }
  }


  fixedGrc(sset) {
    if( (!Array.isArray(sset)) || sset.length != 4 ) {
      console.log('fixedGrc disabling operation by sending all zeros');
      // default values
      sset = [0,0,0,0]
    }

    for(let i = 0; i < 4; i++) {

      const build = (((i << 16) >>> 0) | sset[i])>>>0;
      const build2 = ((hc.GRC_SUBCARRIER_SELECT_CMD) | build)>>>0;

      this.sjs.ringbus(hc.RING_ADDR_CS20, build2);
    }




    // sjs.ringbus(hc.RING_ADDR_CS20, hc.GRC_SUBCARRIER_SELECT_CMD | 0x040010)
  }

  clog(x) {
    console.log(x);
  }

  ddd(x) {
    x('hi');
  }

////////////////////// bs

}


module.exports = {
    WrapNativeSmodem
}