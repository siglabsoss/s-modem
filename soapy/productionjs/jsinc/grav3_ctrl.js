const CS_OP_CODE_0              = 0x00;
const CS_OP_CODE_1              = 0x01;
const CS_OP_CODE_2              = 0x02;
const CS_OP_CODE_3              = 0x03;
const CS_OP_CODE_4              = 0x04;
const CS_OP_CODE_5              = 0x05;
const CS_OP_CODE_6              = 0x06;
const CS_OP_CODE_7              = 0x07;
const CS_OP_CODE_8              = 0x08;
const CS_OP_CODE_9              = 0x09;
const CS_OP_CODE_10             = 0x0A;
const CS_OP_CODE_11             = 0x0B;
const CS_OP_CODE_12             = 0x0C;
const CS_OP_CODE_13             = 0x0D;
const CS_OP_CODE_14             = 0x0E;
const CS_OP_CODE_15             = 0x0F;
const CS_OP_CODE_16             = 0x10;
const CS_OP_CODE_17             = 0x11;
const CS_OP_CODE_18             = 0x12;
const CS_OP_CODE_19             = 0x13;
const CS_OP_CODE_20             = 0x14;
const CS_OP_CODE_21             = 0x15;
const CS_OP_CODE_22             = 0x16;
const CS_OP_CODE_23             = 0x17;
const CS_OP_CODE_24             = 0x18;
const CS_OP_CODE_25             = 0x19;
const CS_OP_CODE_63             = 0x3F;

const CS_AFE_MASK               = 0x80;
const CS_RX_FLAG_MASK           = 0x40;
const CS_OP_CODE_MASK           = 0x3F;
const CS_AFE_A                  = 0x00;
const CS_AFE_B                  = 0x80;
const CS_RX                     = 0x40;

//let exports = module.exports = {};

exports.channel_a = (args) => {
    return this.channel('a',args);
};

exports.channel_b = (args) => {
    return this.channel('b',args);
};

exports.channel = (ch=null, args={}) => {
  let {state=null, current=null, dsa=null} = args;
  let validState = ['tx','rx','off'];
  let validCh = ['a','b'];
  let codes = [];
  
  if((state != null || dsa != null)) {
    if(ch === null) {
        throw "Channel not specified";
    } else if(!validCh.includes(ch)) {
        throw "Invalid Channel specified";
    }
  }

  if(state != null && !validState.includes(state)) {
   throw "State Invalid";
  }

  if(state === 'tx') {
    codes.push(this.tx_state(ch, state));
  } else if(state === 'rx') {
    if(dsa == null) {
      console.log("Warning: no DSA specified. Will set DSA to 0");
      codes.push(this.rx_state(ch, 0));
    } else {
      if( dsa < 0 ) {
        console.log("Warning: negative DSA specified. Will set DSA to 0");
        dsa = 63;
      }

      if( dsa > 63 ) {
        console.log("Warning: DSA larger than 63 specified. Will set DSA to 63");
        dsa = 63;
      }

      codes.push(this.rx_state(ch, dsa));
    }
  } else if(state === 'off') {
    codes.push(this.safe_state(ch));
  } else {
    if(dsa != null) {
      console.log("Warning: Will set  Channel " + ch + " to rx");
      codes.push(this.rx_state(ch,dsa));
    }
  }

  if(current != null) {
    codes.push(this.dac_current(current));
  }

  return codes;
};

exports.tx_state = (ch) => {
    let code;
    if(ch === 'a') {
    code = CS_OP_CODE_3 | CS_AFE_A;
    }
    else if(ch === 'b') {
    code = CS_OP_CODE_3 | CS_AFE_B;
    }
    return code;
};

exports.safe_state = (ch) => {
    let code;
    if(ch === 'a') {
    code = CS_OP_CODE_22 | CS_AFE_A;
    }
    if(ch === 'b') {
    code = CS_OP_CODE_22 | CS_AFE_B;
    }
    return code;
};

exports.all_safe = () => {
    return CS_OP_CODE_0;
};

exports.dump_telem_fpga = () => {
    return CS_OP_CODE_2;
};

exports.dump_telem_pc = () => {
    return CS_OP_CODE_1;
};

exports.dac_current = (current) => {
    return  CS_OP_CODE_4+current;
};

exports.rx_state = (ch, dsa) => {
    let code;
    if(ch === 'a') {
    code = CS_RX | CS_AFE_A | dsa;
    } else {
    code = CS_RX | CS_AFE_B | dsa;
    }
    return code;
};

