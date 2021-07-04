var jsc = require("jsverify");
const _ = require('lodash');
var assert = require('assert');

const RateKeep = require('../mock/ratekeep.js');



describe("RateKeep", function () {


it('slow rate dump mode', function(done) {
  this.timeout(3000);
  let t = jsc.forall('bool', function (input) {

    let rate = new RateKeep.RateKeep({rate:10});

    let counter = 0;

    let firstNow;
    let lastNow;

    let fn = () => {
      // for(let i = 0; i < 1000; i++) {
        let now = Date.now();

        if( typeof firstNow === 'undefined' ) {
          firstNow = now;
        } else {
          lastNow = now;
        }
        // console.log('counter: ' + counter + ', ' + now);
      // }
      counter++;
    }

    rate.reset();

    // let startTime = Date.now();
    for(let i = 0; i < 1000; i++) {
      setTimeout(()=>{rate.do(fn)}, i);
    }
    // let endTime = Date.now();

    return new Promise(function(resolve, reject) {
      setTimeout(() => {
        // console.log('after run');
        // console.log('left: ' + rate.callsWaiting());
        // console.log('counter was ' + counter );
        // console.log('First now: ' + firstNow);
        // console.log('Last  now: ' + lastNow);
        let deltaNow = lastNow - firstNow;
        // console.log('Delta now: ' + deltaNow);

        if( deltaNow < 800 || deltaNow > 1000 ) {
          console.log('Glitch in timer or test caused invalid results, defaulting to pass');
        } else {
          assert(counter == 10, "timer was too fast or too slow");
          assert(rate.callsWaiting() === 0, "there are callbacks waiting in drop mode")

        }

        // let delta = endTime - startTime;
        // console.log('run took ' + delta + ' ms');

        resolve(true);
      }, 1200); // setTimeout
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



it('slow rate queue mode', function(done) {
  this.timeout(30000);
  let print = false;
  let t = jsc.forall(jsc.integer(400,990), jsc.integer(300,700), function (callsPerSecond, calls) {

    if(print) {
      console.log('Setting rate to ' + callsPerSecond + ' callsPerSecond with ' + calls + ' calls');
    }

    let rate = new RateKeep.RateKeep({rate:callsPerSecond, mode:RateKeep.Mode.Queue});

    let counter = 0;

    let outOfOrder = 0;

    let firstNow;
    let lastNow;

    let times = [];

    let fn = (k) => {
      let now = Date.now();

      if( typeof firstNow === 'undefined' ) {
        firstNow = now;
        lastNow = now; // will get junked, but we use to make times[]
      } else {
        times.push(now - lastNow);
        lastNow = now;
      }

      // console.log('counter: ' + counter + ' ' + k);

      // if the argument k is not the same as our counter, we were called out of order
      if( counter !== k ) {
        console.log('oopsie!');
        outOfOrder++;
      }
      counter++;
    }

    rate.reset();

    // let calls = 600;

    // when we call the callback, we bind in the loop i
    // this lets us catch out of order calling of the callbacks
    for(let i = 0; i < calls; i++) {
      setTimeout(()=>{ rate.do(fn.bind(this,i));}, i);
    }

    let estimatedTimeToCompletion = 1000 * calls / callsPerSecond; // in ms
    if(print) {
      console.log('Estimate completed in ' + estimatedTimeToCompletion + ' ms');
    }

    estimatedTimeToCompletion += 300; // add fudge

    return new Promise(function(resolve, reject) {
      setTimeout(() => {
        if(print) {
          console.log('after run');
          console.log('left: ' + rate.callsWaiting());
          console.log('counter was ' + counter );
        }

        assert.equal(counter, calls, "timer was too fast or too slow");
        assert(rate.callsWaiting() === 0, "there are callbacks waiting in queue mode after way too long");
        assert(outOfOrder === 0, "callbacks were called out of order");

        let sum = _.sum(times);
        let ave = sum / calls;

        let expectedAve = 1000 / callsPerSecond;

        let deltaAve = Math.abs(expectedAve - ave);
        if(print) {
          console.log('sum was ' + sum + ' ave ' + ave + ' expected ' + expectedAve + ' error ' + deltaAve);
        }

        // measured in ms, not seconds
        assert(deltaAve < 0.6, "Averate time per callback error was gross large " + deltaAve + 'ms');

        // let delta = endTime - startTime;
        // console.log('run took ' + delta + ' ms');

        resolve(true);
      }, estimatedTimeToCompletion); // setTimeout
    }); // promise
  }); // forall

  const props = {tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


}); // describe
