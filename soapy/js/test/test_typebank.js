var jsc = require("jsverify");
const _ = require('lodash');
var assert = require('assert');

const Bank = require('../mock/typebank.js');



describe("Bank tests", function () {


it('bank object', (done) => {
  let t = jsc.forall('bool', function (input) {

    return new Promise(function(resolve, reject) {
      setImmediate(() => {
        let uint = new Bank.TypeBankUint32().access();

        uint.aa = 10;

        assert(uint.aa == 10, "Can't assign and then get");

        uint.a = 0xff000000;

        assert(uint.a > 0, "fetching a large value should be positive");

        uint.b += 4;

        assert(uint.b == 4, " += from unset value should start at 0");

        // resolve( Math.random() > 0.0006 );
        // resolve( _.isEqual(input, cap.capture ) );
        resolve(true);
      });
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('int problems', (done) => {
  let t = jsc.forall('bool', function (input) {

    return new Promise(function(resolve, reject) {
      setImmediate(() => {
        let uint = new Bank.TypeBankUint32().access();

          // bad examples come from https://www.npmjs.com/package/uint32

          // this class passes if all examples are first written to a variable
          
          uint.x = 0xFFFFFFFF;

          uint.lhs = ~~uint.x;
          uint.rhs = uint.x

          let r0 = uint.lhs === uint.rhs;
          uint.lhs = (uint.x | 0) ;
          let r1 = uint.lhs === uint.x;  
          uint.lhs = (uint.x & uint.x) ;
          let r2 = uint.lhs === uint.x;  
          uint.lhs = (uint.x ^ 0) ;
          let r3 = uint.lhs === uint.x;  
          uint.lhs = (uint.x >> 0) ;
          let r4 = uint.lhs === uint.x; 
          uint.lhs = (uint.x << 0) ;
          let r5 = uint.lhs === uint.x; 

          assert(r0);
          assert(r1);
          assert(r2);
          assert(r3);
          assert(r4);
          assert(r5);

        // resolve( Math.random() > 0.0006 );
        // resolve( _.isEqual(input, cap.capture ) );
        resolve(true);
      });
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('bank as array and 64bit -> 32bit value behavior', (done) => {
  let t = jsc.forall('bool', function (input) {

    return new Promise(function(resolve, reject) {
      setImmediate(() => {
        let uint = new Bank.TypeBankUint32().access();

        // we would like to see how close to c++ we are.  in c++ I ran this:

        /*
        uint32_t x;
        uint64_t large = 1;

        for(auto i = 0; i < 64; i++) {
          x = large + i;
          std::cout << "i: " << i << "    "    << x << std::endl;

          large *= 2;
        }
        */

        // the purpose is to make a 64 bit value (remember we can't use shift past 32bits) (we use *= 2 instead)
        // that grows with each loop iteration, and then see what happens when we assign into the bank variable
        // turns out everything works exactly correctly except for double precision rounting which breaks at i = 53
        // without large (+ i) the lower result is always zero, which is also correct

        const expected = [1, 3, 6, 11, 20, 37, 70, 135, 264, 521, 1034, 2059, 4108, 8205, 16398, 32783, 65552, 131089, 262162, 524307, 1048596, 2097173, 4194326, 8388631, 16777240, 33554457, 67108890, 134217755, 268435484, 536870941, 1073741854, 2147483679, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63];

        let large = 1;

        for(let i = 0; i < 64; i++) {
          uint[i] = large + i;
          large *= 2;
          // console.log('' + i + ': ' + uint[i].toString(16));
        }

        // double precision rounding screws us at i=53
        for(let i = 0; i < 53; i++) {
          assert(uint[i] === expected[i], `TypeBankUint32 did not match c++ results when i = ${i}` );
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
