var jsc = require("jsverify");
const _ = require('lodash');
var assert = require('assert');

const Schedule = require('../mock/mockschedule.js');
const Bank = require('../mock/typebank.js');


describe("Schedule object", function () {


it('make schedule', (done) => {
  let t = jsc.forall('bool', function (input) {

    return new Promise(function(resolve, reject) {
      setImmediate(() => {

        const print = false;

        let schedule = Schedule.schedule_t();

        Schedule.schedule_all_on(schedule);

        let stack = new Bank.TypeBankUint32().access();
        stack.progress = 0;
        stack.accumulated_progress = 0;
        stack.timeslot = 0;
        stack.epoc = 0;
        stack.can_tx = 0;

        let runs = 300; //100000;

        for( let i = 0; i < runs; i++) {
            // special
            stack.lifetime_32 = 0xffffff00 + i;

            [ stack.progress,
              stack.accumulated_progress,
              stack.timeslot,
              stack.epoc,
              stack.can_tx ] = Schedule.schedule_get_timeslot2(schedule, stack.lifetime_32);

              if( print ) {
                  console.log(`.lifetime_32 = ${stack.lifetime_32}, .progress = ${stack.progress}, .accumulated_progress = ${stack.accumulated_progress}, .timeslot = ${stack.timeslot}, .epoc = ${stack.epoc}, .can_tx = ${stack.can_tx}`);
              }
          }

        resolve(true);
      });
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


}); // describe
