var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const enumerate = require('pythonic').enumerate;
const bufferSplit = require('../mock/nullbuffersplit');

var assert = require('assert');



describe('Null Buffer Split', function () {

it('Null Buffer Prepare', function() {

    let expected = [0x68,
      0x65,
      0x6c,
      0x6c,
      0x6f,
      0x20,
      0x77,
      0x6f,
      0x72,
      0x6c,
      0x64,
      0x00];


    let res = bufferSplit.prepare('hello world');

    let resArray = [];

    for( x of res ) {
      // console.log(x.toString(16));
      resArray.push(x);
    }

    assert.deepEqual(resArray, expected);
}); // it

it('Null Buffer Split1', function(done) {
  // this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let inputsRaw = ['hi','hello'];

    let inputsMessage = [];

    for( x of inputsRaw ) {
      inputsMessage.push(bufferSplit.prepare(x));
    }

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      try {

        let calls = 0;

        let cb = function(a) {
          if( calls === 0 ) {
            assert.equal(a,inputsRaw[0]);
          }
          if( calls === 1 ) {
            assert.equal(a,inputsRaw[1]);
            resolve(true);
          }

          calls++;
          // console.log(a);
        }

        const doSplit = new bufferSplit.NullBufferSplit(cb);
        doSplit.addData(inputsMessage[0]);
        doSplit.addData(inputsMessage[1]);

      
      }catch(e){reject(e)}
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it




it('Null Buffer Split2', function(done) {
  // this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let inputsRaw = [];
    let simpleAdditions = jsc.random(0,1);

    if( simpleAdditions ) {
      inputsRaw = ['hi','hello'];
    }

    let additions = jsc.random(1,50);
    // console.log('adding ' + additions + ' strings');

    for( let i = 0; i < additions; i++) {
      let length = jsc.random(1,2000);
      // console.log('picked string length ' + length);
      let str = '';
      for( let j = 0; j < length; j++ ) {
        let chrCode = jsc.random(0x20,0x7E); // inclusive
        str = str +  String.fromCharCode(chrCode);
      }

      inputsRaw.push(str);
    }

    // for( x of inputsRaw ) {
    //   console.log(x.length);
    // }

    let expectedCalls = inputsRaw.length;

    let inputsMessage = [];

    for( x of inputsRaw ) {
      inputsMessage.push(bufferSplit.prepare(x));
    }

    let inputsSingleBuffer = Buffer.concat(inputsMessage);

    // console.log(inputsSingleBuffer);

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      try {

        let calls = 0;

        let cb = function(a) {
          assert.equal(a, inputsRaw[calls]);

          if( (calls+1) === expectedCalls ) {
            resolve(true);
          }

          // console.log(calls + " exp " + expectedCalls);
          calls++;
        }

        const doSplit = new bufferSplit.NullBufferSplit(cb);

        let added = 0;
        while(added < inputsSingleBuffer.length) {
          let willAdd = jsc.random(1,10000);
          let slc = inputsSingleBuffer.slice(added,added+willAdd);
          doSplit.addData(slc);
          added += willAdd;
        }
      }catch(e){reject(e)}
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 100};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



it('Null Buffer Split3', function(done) {
  // this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let expected = [
      0x00,
      0x68,
      0x65,
      0x6c,
      0x6c,
      0x6f,
      0x20,
      0x77,
      0x6f,
      0x72,
      0x6c,
      0x64,
      0x00,
      0x00,
      0x00,
      0x00,
      0x68,
      0x65,
      0x6c,
      0x6c,
      0x6f,
      0x20,
      0x77,
      0x6f,
      0x72,
      0x6c,
      0x64,
      0x00
      ];

    let inputsSingleBuffer = Buffer.from(expected);

    // console.log(inputsSingleBuffer);

    let hell = 'hello world';

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      try {

        let calls = 0;

        let cb = function(a) {
          if( calls === 0 ) {
            assert.equal(a,hell);
          }
          if( calls === 1 ) {
            assert.equal(a,hell);
            resolve(true);
          }

          calls++;
          // console.log(a);
        }

        const doSplit = new bufferSplit.NullBufferSplit(cb);
        doSplit.addData(inputsSingleBuffer);

      
      }catch(e){reject(e)}
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it

}); // describe
