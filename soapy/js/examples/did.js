'use strict';
const hc = require('../mock/hconst.js');
const mutil = require('../mock/mockutils.js');
// const sjs = require('./build/Release/sjs.node');
// const wrapNative = require('./jsinc/wrap_native.js');
// const wrapHandle = new wrapNative.WrapNativeSmodem(sjs);
const HandleCS20Tx = require('../../productionjs/jsinc/handle_cs20_tx.js');
const handleTx = new HandleCS20Tx.HandleCS20Tx({});
const fs = require('fs');



let w = [
0x46000003,
0x4b008000,
0x4b010060,
0x4b020029,
0x4b03002d,
0x4b040400,
0x4b050000,
0x4b06ffff,
0x4b07ffff,
0x4b080000,
0x4b090000,
0x4b0a0004,
0x4b0b0000,
0x4b0c0000,
0x4b0d0004,
0x4b0e0000,
0x4b0f0000,
0x4b100001,
0x4b110000,
0x4b120000,
0x4b130001
];

w = [
0x46000003,
0x4b008000,
0x4b010060,
0x4b020060,
0x4b030060,
0x4b040400,
0x4b050000,
0x4b06ffff,
0x4b07ffff,
0x4b08ffff,
0x4b090000,
0x4b0a0002,
0x4b0b0000,
0x4b0c0000,
0x4b0d0002,
0x4b0e0000,
0x4b0f0001,
0x4b100000,
0x4b110001,
0x4b120001,
0x4b130000
]

w = [
0x4b004800,
0x4b010012,
0x4b020012,
0x4b030012,
0x4b040400,
0x4b050000,
0x4b06ffff,
0x4b07ffff,
0x4b08ffff,
0x4b090000,
0x4b0a0002,
0x4b0b0000,
0x4b0c0000,
0x4b0d0002,
0x4b0e0000,
0x4b0f0001,
0x4b100000,
0x4b110001,
0x4b120001,
0x4b130000


]

handleTx.capCount++;
handleTx.pushDefault();

for( let x of w ) {
  handleTx.rbcb(x);
}

handleTx.printCapture();

w = [
0x48000000,
0x48100000,
0x48200001,
0x48300000,
0x48400000,
0x48500000,
0x48600000,
0x48700000,
0x48800000,
0x48900000,
0x48a00000,
0x4b000000,
0x4b010000,
0x4b020000,
0x4b030000,
0x4b040000,
0x4b050000,
0x4b06ffff,
0x4b07ffff,
0x4b08ffff,
0x4b090000,
0x4b0a0000,
0x4b0b0000,
0x4b0c0000,
0x4b0d0000,
0x4b0e0001,
0x4b0f0000,
0x4b100000,
0x4b110000,
0x4b120000,
0x4b130001






]

handleTx.capCount++;
handleTx.pushDefault();

for( let x of w ) {
  handleTx.rbcb(x);
}

handleTx.printCapture();