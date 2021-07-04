// const c = require('./hconst.js');
// console.log(c.FEEDBACK_TYPE_UNJAM);

'use strict';

const _ = require('lodash');
// const mutil = require('./mockutils.js');
const util = require('util');
const EventEmitter = require('events').EventEmitter;




// Object.assign(global, c);

// require('./hconst.js');

// (1, eval) = ('const FEEDBACK_TYPE_UNJAM = (4);')

// eval.apply(null, 'const FEEDBACK_TYPE_UNJAM = (4);');
// const FEEDBACK_TYPE_UNJAM = (4);
// global.FEEDBACK_TYPE_UNJAM = 4;
// console.log(FEEDBACK_TYPE_UNJAM);



// a = 0xff000000;
// console.log(a.toString(16));
// b = 0xff << 24;
// console.log(b.toString(16));


// var arr = new Uint32Array([0xff<<24]);
// console.log(arr[0]); // 31


// console.log(a);

// https://nodejs.org/api/stream.html#stream_implementing_a_transform_stream

const Stream = require('stream');
const readable = new Stream.Readable({
    read:function(){}
});

let _storage = null;

const upperCaseTr = new Stream.Transform({
  transform(chunk, encoding, callback) {
    console.log(chunk.length);

    if( _storage !== null ) {
      this.push(_storage);
      _storage = null;
    }

    if( Math.random() < 0.2 ) {
      this.push(chunk.toString().toUpperCase());
    } else {
      // hold
      _storage = chunk;
    }
    callback();
  }
});

readable.pipe(upperCaseTr).pipe(process.stdout);

let foo = 0;

function fill() {
  const items = Uint8Array.from([0x61+foo,0x20,0x20,0x20]);

  foo = (foo+1) % 26;

  readable.push(items);
}

function fillTime() {
  fill();
  setTimeout(fillTime, 100);
}

fillTime();

// setTimeout(function() {
//   fill();
// }, 10);




// no more data
// readable.push(null);




// function BufNotify() {
//   EventEmitter.call(this);
//   this.setMaxListeners(Infinity);
// }

// util.inherits(BufNotify, EventEmitter);


// let _

// setTimeout(function(){

// }, 1000);