const Mode = {
  Drop: 0,
  Queue: 1
}

// This Class allows you to keep a constant "rate" from a fixed starting point
// This can be used to emit samples at a constant rate.
// If there is a glitch, and the samples "get behind", this class will allow you to "catch up"
// by tracking total runs against the system timer
//
// There are 2 modes
//  Drop: This will drop calls that run too fast, and return false to notify you
//  Queue: This will queue calls with a setTimeout, (return false to let you know it was queued),
//         and run at a later time
class RateKeep {
  constructor(options) {
    this.startTime = Date.now();
    Object.assign(this, options);
    this.sent = 0;
    this.blockedCalls = [];
    this.outstandingFutureTimers = 0;

    if( typeof this.rate === 'undefined' ) {
      throw(new Error('must pass `rate` to RateKeep'));
    }

    // because the clock we are using is in ms
    this.rate /= 1000;

    if( typeof this.mode === 'undefined' ) {
      this.mode = Mode.Drop;
    }

    if( this.mode === Mode.Drop ) {
      this.do = this._doDrop;
    }
    if( this.mode === Mode.Queue ) {
      this.do = this._doQueue;
    }
  }

  // optional, resets call count and start time
  reset() {
    this.startTime = Date.now();
    this.sent = 0;
  }

  // returns true if cb was run, false if it was dropped
  _doDrop(cb) {
    let now = Date.now();
    let target = (now - this.startTime) * this.rate;
    if( (target - this.sent) >= 1 ) {
      this.sent++;
      cb();
      return true;
    } else {
      return false;
    }
  }

  // pass now
  // this function calculates the ms time that the next callback will run
  // and then schedules a call to _serviceAll
  _scheduleFutureService(now) {
      // time on clock when we will fire next
      let nextFire = ((1+this.sent) / this.rate) + this.startTime;
      let timeTillThen = nextFire - now;

      // round and bound
      let timeBounded = Math.max(0,Math.round(timeTillThen));

      // crude way to track ountstaning callbacks (but does it work?)
      this.outstandingFutureTimers++;
      // schedule a later check
      // even if many multiples of these are queued, we will not call cb() more times than asked for 
      // by the user (This could technically be optimized by checking for an existing timeout)
      setTimeout(() => {this.outstandingFutureTimers--; this._serviceAll();}, timeBounded);
      // console.log('nextFire: ' + nextFire + ', '  + timeTillThen + ', ' + timeBounded + ', ' + now);
      // console.log('nextFire: ' + nextFire + ', ' + timeBounded + ', ' + now);
  }

  // returns true if cb was run, false if it was queued
  _doQueue(cb) {

    // if there are calls in line, we favor them...
    // we queue our callback, and then call _servieAll which does it's rate check
    // removing this if will cause out of order callback execution, but why would you want that?
    if(this.callsWaiting() > 0) {
      this.blockedCalls.push(cb);
      this._serviceAll();
      return false;
    }

    let now = Date.now();
    let target = (now - this.startTime) * this.rate;
    if( (target - this.sent) >= 1 ) {
      this.sent++;
      // console.log("did run: " + now);
      cb();
      return true;
    } else {

      // add to queue to run later
      this.blockedCalls.push(cb);

      // console.log('saved with ' + this.blockedCalls.length + ' in line');

      // schedule a run check for the future
      this._scheduleFutureService(cb, now);
      
      return false;
    }
  }

  // mostly a copy of _doQueue() but for 2 aspects:
  // 1) pulls cb from the internal queue, not an argument, instead returns false in the case that
  // 2) in case that we are too fast to call, returns false and does not run (so we can run cb later)
  // do not call this manually from outside class
  _serviceBlockedCall() {
    // if there are no callbacks, we just exit
    if( this.blockedCalls.length === 0) {
      return false;
    }


    let now = Date.now();
    let target = (now - this.startTime) * this.rate;
    if( (target - this.sent) >= 1 ) {
      this.sent++;

      // grab 1 that was blocked
      let cb = this.blockedCalls.shift();
      
      // console.log("did run later: " + now);
      cb();
      return true;
    } else {
      return false;
    }
  }

  // called with a timer.  this will call as many callbacks as are allowed due to the time frame
  // a signle call to this fn() can "catch up" after a long time of inactivity
  // Also, due to the fact that we re-enter _serviceBlockedCall(), which calculates a new Date() each time,
  // in the situation where callback()'s take a long time to run, we continue to meet the rate as close as possible
  // by checking the data with each call
  _serviceAll() {
    // console.log('enter _serviceAll');
    let ret = true;
    let runs = 0;
    while(ret) {
      ret = this._serviceBlockedCall();
      if( ret ) {
        runs++;
      }
    }

    // console.log('ran ' + runs + ' callbacks and there were ' + this.outstandingFutureTimers + ' callbacks alive ' + this.callsWaiting() + ' waiting');


    // if there are callbacks waiting to run, and we dont have any timers setup
    // set them up
    if((this.callsWaiting()) > this.outstandingFutureTimers ) {
      let now = Date.now();
      this._scheduleFutureService(now);
      // console.log('scheduled a future check');
    } else {
      // console.log('did NOT schedule a future check');
    }

  }

  callsWaiting() {
    return this.blockedCalls.length;
  }

} // class



module.exports = {
    RateKeep
  , Mode
}
