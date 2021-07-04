'use strict';

const util = require('util');
const EventEmitter = require('events').EventEmitter;


function DispatchRingbusEE() {
  EventEmitter.call(this);
  this.setMaxListeners(Infinity);
}
util.inherits(DispatchRingbusEE, EventEmitter);


class DispatchRingbus {
  constructor(sjs) {
    this.sjs = sjs;

    // Event Emitter ringbus
    this.notify = new DispatchRingbusEE();
  }


  // instead of calling hrb.notify.on
  // call this instead
  // this will tell smodem to send ringbus types up to javascript
  // as well as setup the event emitter on()
  on(ringbusMask, cb) {
    this.sjs.ringbusCallbackCatchType(ringbusMask);
    this.notify.on(''+ringbusMask, cb);
  }

}


module.exports = {
    DispatchRingbus
}
