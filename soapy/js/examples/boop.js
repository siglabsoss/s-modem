var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('./mockutils.js');
const mapmov = require('./mapmov.js');

var assert = require('assert');

let in0 = mutil.readHexFile('test/data/mapmov_in0.hex');
let ideal_out0 = mutil.readHexFile('test/data/mapmov_out0.hex');




// let piece = ideal_out0.slice(0,8);


// let buf = Uint32Array.from(piece);


// var bytes = new Uint8Array(buf);


// console.log(buf);
// console.log(bytes);

// var kindOf = require('kind-of');


var buffer = new ArrayBuffer(4);
var dataView = new DataView(buffer);
var int8View = new Int8Array(buffer);
var uint32View = new Uint32Array(buffer);
// dataView.setInt32(0, 0x1234ABCD);
uint32View[0] = 0x1234ABCD;
console.log(dataView.getInt32(0).toString(16)); 
console.log(dataView.getInt8(0).toString(16)); 
console.log(int8View[0].toString(16));

console.log('uint32View ' + uint32View[0].toString(16));

int8View[0] = 0xff;

console.log('uint32View ' + uint32View[0].toString(16));

// console.log('t1 ' + kindOf(int8View));

const items = Uint8Array.from([0x61,0x20,0x20,0x20]);

// console.log('t2 ' + kindOf(items));
