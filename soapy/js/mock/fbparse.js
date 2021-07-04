"use strict";


const _ = require('lodash');
const mutil = require('./mockutils.js');
const util = require('util');
const EventEmitter = require('events').EventEmitter;
const Stream = require("stream");

const hc = require('./hconst.js');

const MapMov = require('./mapmov.js');


class FeedbackFrame extends Array {
  constructor(options = 16){
    super(options);
    for(let i = 0; i < options; i++) {
      this[i] = 0;
    }
  }

  foo() {
    console.log("foo: " + this);
  }

  get type() { return this[0]; }
  set type(v) { this[0] = v; }
  get fblength() { return this[1]; }
  set fblength(v) { this[1] = v; }
  get destination0() { return this[2]; }
  set destination0(v) { this[2] = v; }
  get destination1() { return this[3]; }
  set destination1(v) { this[3] = v; }
  get vtype() { return this[4]; }
  set vtype(v) { this[4] = v; }
  get stype() { return this[4]; }
  set stype(v) { this[4] = v; }
  get seq() { return this[5]; }
  set seq(v) { this[5] = v; }
  get seq2() { return this[6]; }
  set seq2(v) { this[6] = v; }
}

// things shared by vector and stream
class FeedbackVectorOrStream extends FeedbackFrame{
  constructor(options = 16) {
    super(options);
  }

  setLength(length) {
    if( length < 0 ) {
      throw new(Error('length cannot be negative'));
    }
    this.fblength = hc.FEEDBACK_HEADER_WORDS + length; // set member
    // this.length = this.fblength; // resize the array
  }
}

class FeedbackVector extends FeedbackVectorOrStream {
  // se init_feedback_vector
  constructor(destinations, deliver_higgs, deliver_pc, vtype) {
    super(16);

    this.type = hc.FEEDBACK_TYPE_VECTOR;
    this.vtype = vtype;
    this.destination0 = destinations;

    this.destination1 = 0;
    if( deliver_higgs ) {
        this.destination1 = hc.FEEDBACK_DST_HIGGS;
    }
    if( deliver_pc ) {
        this.destination1 = hc.FEEDBACK_DST_PC;
    }
  }
}

class FeedbackStream extends FeedbackVectorOrStream {
  // se init_feedback_stream
  constructor(destinations, deliver_higgs, deliver_pc, stype) {
    super(16);

    this.type = hc.FEEDBACK_TYPE_STREAM;
    this.stype = stype;
    this.destination0 = destinations;

    this.destination1 = 0;
    if( deliver_higgs ) {
        this.destination1 = hc.FEEDBACK_DST_HIGGS;
    }
    if( deliver_pc ) {
        this.destination1 = hc.FEEDBACK_DST_PC;
    }
  }
}




function g_type(b) {
  return b[0];
}

function g_length(b) {
  return b[1];
}

function g_destination0(b) {
  return b[2];
}

function g_destinatino1(b) {
  return b[3];
}

function g_vtype(b) {
  return b[4];
}

function g_seq(b) {
  return b[5];
}

function g_seq2(b) {
  return b[6];
}


function strForType(t1, t2) {
  // return t1.toString() + "_" + t2.toString();
  return `${t1}_${t2}`;
}

// helper class
function FBNotify() {
  EventEmitter.call(this);
  this.setMaxListeners(Infinity);
}
util.inherits(FBNotify, EventEmitter);






class Parser {
  constructor(options={}) {

    const {printUnhandled=true, printPackets=false} = options;

    Object.assign(this, {printUnhandled, printPackets});


    this._buf = [];
    this._notify = undefined;
    this._jamming = -1;
  }

// warning: if this function sees a mapmov packet
// it will advance the non-expanded count (where as the count on the wire assumes expansion)
// this lets us expand it later
 feedback_word_length(v) {
    let error = false;
    let t = g_type(v);
    switch(t) {
        case hc.FEEDBACK_TYPE_STREAM:
        case hc.FEEDBACK_TYPE_VECTOR:
            if(g_length(v) < hc.FEEDBACK_HEADER_WORDS) {
              return [true, 1, false];
            }
            // this clause is not upheld in the new mapmov paradigm
            // if(g_length(v) > hc.FEEDBACK_MAX_LENGTH) {
            //   return [true, 1];
            // }

            // console.log("FEEDBACK_TYPE_VECTOR: " + g_length(v));

            if( t === hc.FEEDBACK_TYPE_VECTOR && g_vtype(v) === hc.FEEDBACK_VEC_TX_USER_DATA ) {
              // this is a mapmov packet
              // we will expand it later. for now clip out the unmapmov'd length

              // the 16 here is because this function gives us the lenght of the data but we need the header as well
              let pre_length = this.mapmov.getPreMapMovCount(g_length(v)) + 16;

              // console.log("got feedbackbus type, with length " + g_length(v) + ' and calculated: ' + pre_length);

              return [error, pre_length, true];
            } else {
              
              return [error, g_length(v), false];
            }

            break;

        case hc.FEEDBACK_TYPE_RINGBUS:
            if(g_length(v) > hc.FEEDBACK_HEADER_WORDS) {
                return [true, 1, false];
            }
            if(g_length(v) < hc.FRAME_PRIMITIVE_WORDS) {
                // *error = true; // a length of 0,1,2,3 are illegal
                // techincally a rb could be empty but why?
                return [true, 1, false];
            }
            // smaller values are OK and used to parse last useful byte of struct
            return [error, hc.FEEDBACK_HEADER_WORDS]; // length for ringbus is ALWAYS 16
            break;
        case hc.FEEDBACK_TYPE_FLUSH:
            // in the case of a flush, we return 1 to force caller
            // to check every byte 1 at a time
            return [error, 1, false];
            break;
        default:
            // unknown type
            return [true, 1, false];
            break;
    } // switch
} // feedback_word_length


  gotPacket(buf) {
    // console.log(buf.length);
    if(buf.length % 4 !== 0) {
      throw(new Error('gotPacket got data not multiple of 4'));
      return;
    }

    // const wordlen = buf.length / 4;
    // console.log(Sugar.Number.round(3.1415));
    // console.log(Sugar.Number.range(1,wordlen));

    // create a range of multiples of 4 which we mark the start of chunks of the buffer
    const range = _.range(0, buf.length, 4);

    // using the range, we convert to words and get out as an array
    const aswords = range.map(x => mutil.bufToI32(mutil.sliceBuf4(buf, x)) );

    // add to the end of the gobal object
    this._buf = this._buf.concat(aswords);

    // console.log('_buf has ' + _buf.length);

    // console.log('aswords was: ' + g_type(aswords));

    this._notify.emit('newp');
  }




  regCallback(t1, t2, cb) {
    this._notify.on(strForType(t1,t2), cb);
  }

  regUserdataCallback(cb) {
    this._notify.on('ud', cb);
  }

  // as is, jamming only detects zeros if the are at the lead of packet
  // this means this function behaves differently based on chunk size (bad)
  bufGotLonger() {
    if(this._buf.length < 16 ) {
      return;
    }
    // console.log('bufGotLonger2 ' + _buf);

    // trim zeros first cuz why not
    for(let i = 0; i < this._buf.length; i++) {
      let word = this._buf[i];
      if( word != 0 ) {
        // if this isn't the first loop
        if( i != 0 ) {
          // trim buf
          this._buf = this._buf.slice(i);

          if( this._jamming !== -1 )
          {
            if( this.printPackets ) {
              console.log('jamming for ' + this._jamming);
              this._jamming = -1;
            }
          }

        }
        break; // break if we trimmed or not
      } else {
        if( this._jamming === -1 ) {
          this._jamming = 1;
        } else {
          this._jamming++;
        }
      }
    } // for

    // console.log('bufGotLonger2a ' + this._buf);

    for(let i = 0; i < this._buf.length; i++) {

      let word = this._buf[i];

      // console.log('bufGotLonger3 i:' + i + ' ' + word);

      if( word != 0 ) {
        let [adv_error, advance, is_mapmov] = this.feedback_word_length(this._buf);

        if( advance > this._buf.length ) {
          return;
        } else {

          if( this.printPackets ) {
            if( adv_error && advance !== 1 ) {
              console.log('advance: ' + advance + ' error: ' + adv_error + ' mapmov: ' + is_mapmov);
            }
          }


          if( !adv_error ) {
            // we got a match, emit the event
            let t0 = g_type(this._buf);
            let t1 = g_vtype(this._buf);

            // fixme this could be done without entirePacket temporary value
            let entirePacket = this._buf.slice(i, advance);
            let header = entirePacket.slice(0,16);
            let buf = entirePacket.slice(16);
            const tag = strForType(t0,t1);
            this._notify.emit(tag, header, buf);

            if( this.printUnhandled ) {
              let listners = this._notify.listenerCount(tag);
              if( listners === 0 ) {
                console.log("UNHANDLED feedbackbus to " + tag);
              }
            }
            
            // console.log("t0: " + t0 + " t1: " + t1);
            // console.log('bufGotLonger4a ' + this._buf);

            this._buf = this._buf.slice(i + advance);

            // console.log('bufGotLonger4b ' + this._buf);

            if( this._buf.length >= 16 ) {
              // console.log('will recurse for remaining ' + this._buf.length);
              this._notify.emit('newp'); // recurse back in
            }

            break;
          }
        }
      }

    } // for
  } // bufGotLonger


  handleMapMovPacket(header, buf) {

    // let header = b.slice(0,16);
    // let buf = b.slice(16);
    // console.log('got mapmov ' + buf);

    let r = this.mapmov.accept(buf);

    // console.log(" " + r);

    this._notify.emit('ud', header, r);
  }


  reset() {
    this._buf = [];
    this._notify = new FBNotify();
    this._notify.on('newp', this.bufGotLonger.bind(this));
    this._notify.on(strForType(hc.FEEDBACK_TYPE_VECTOR, hc.FEEDBACK_VEC_TX_USER_DATA), this.handleMapMovPacket.bind(this));
    this.mapmov = new MapMov.MapMov();
    this.mapmov.reset();
  }

} // class Parser


// Parser class was written before I knew how to do stream.s
// this wrapper should fix that and be as good as if it were originally a stream
class ParserStream extends Stream.Writable {
  constructor(options, options2){
    super(options) //Passing options to native constructor (REQUIRED)
    // Object.assign(this, opt);
    this._wrap = new Parser(options2);
    this._wrap.reset(); // do this for free
  }

  regCallback(x,y,z) {
    return this._wrap.regCallback(x,y,z);
  }

  reset() {
    return this._wrap.reset();
  }

  regUserdataCallback(x) {
    return this._wrap.regUserdataCallback(x);
  }

  gotPacket(x) {
    throw(new Error('Illegal to call gotPacket on ParserStream class. See Parser'));
  }
  
  _write(chunk,encoding,cb) {
    this._wrap.gotPacket(chunk);
    // // sadly this does a buffer copy accorind to 
    // // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    // const uint8_view = new Uint8Array(chunk, 0, chunk.length);
    // var dataView = new DataView(uint8_view.buffer);

    // // in place modify of the copy
    // for(let i = 0; i < chunk.length/8; i++) {
    //   let tmp = dataView.getFloat64(i*8, true) * this.gain;
    //   dataView.setFloat64(i*8, tmp, true);
    //   // this.capture.push();
    // }
    // this.push(uint8_view);

    cb();
  }
}




// returns true if the feedback type is valid
function feedback_type_valid(v) {
    switch(v.type) {
        case hc.FEEDBACK_TYPE_STREAM:
        case hc.FEEDBACK_TYPE_VECTOR:
        case hc.FEEDBACK_TYPE_RINGBUS:
        case hc.FEEDBACK_TYPE_FLUSH:
            return true;
            break;
        default:
            return false;
            break;
    }
}


function feedback_peer_to_mask(peer_id) {
    return (0x1<<(peer_id+1))>>>0;
}





module.exports = {
    Parser
  , ParserStream
  , g_type
  , g_length
  , g_destination0
  , g_destinatino1
  , g_vtype
  , g_seq
  , g_seq2
  , feedback_type_valid
  , feedback_peer_to_mask
  , FeedbackFrame
  , FeedbackVector
  , FeedbackStream
  , FeedbackVectorOrStream  // not sure if outside needs this
};