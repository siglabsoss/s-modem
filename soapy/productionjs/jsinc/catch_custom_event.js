'use strict';

const util = require('util');
const EventEmitter = require('events').EventEmitter;

// private to this file
function CatchNotify() {
  EventEmitter.call(this);
  this.setMaxListeners(Infinity);
}
util.inherits(CatchNotify, EventEmitter);

class CatchCustomEvent {
  constructor(print=false) {
    this.state = null;
    this.notify = new CatchNotify();
    this.print = print;
  }

  cb(x) {
    // console.log('class ev: ' + x + ' state: ' + this.state);
    if(this.state === null) {
      this.state = x;
    } else {
      let a = this.state;
      let b = x;

      this.notify.emit((a).toString(), a, b);
      if(this.print) {
        console.log('class ev: ' + a + ', ' + b);
      }

      this.state = null;
    }
  }
}


module.exports = {
  CatchCustomEvent
}