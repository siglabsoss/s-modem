'use strict';

const Stream = require("stream"); //require native stream library
const mt = require('mathjs');
const mutil = require('./mockutils.js');
var kindOf = require('kind-of');
const _ = require('lodash');
const pipeType = require('../mock/pipetypecheck.js');

// const binary = require('bops');


class MockEnvironmentPair {
  constructor() {
        // id of next attached radio
    this.radioCount = 0;
    this.r = [];

    this.datas = [];

    this.matrix = [[]];

    this.then;
    this.now;

    this.timesWrittenBack = 0;

    this.sampleCountAccumulated = 0;
    this.sampleCountLatched = 0;

    this.rateLimit = 0;
  }

  attachRadio(readableTx, writableRx) {
    let id = this.radioCount;

    if( this.radioCount === 2) {
      throw(new Error('MockEnvironmentPair only accepts 2 radios'));
    }

    // handle the dut -> env
    // highWaterMark:2048
    // the radio's tx pipe will pipe into this writable
    // let myStream = new myWritable({decodeStrings:false},{id:id});

    // readableTx.pipe(myStream);

    // handle env -> dut

    // let writeToDut = new myReadable({decodeStrings:false},{id:id});

    // writeToDut.pipe(writableRx);


    // tx/rx in this context are relative to DUT (why)
    let obj = {
      tx:{theirs:readableTx,state:{}},
      rx:{theirs:writableRx,state:{}}
    };

    this.r[id] = obj;
    this.datas[id] = [];
    // this.outstandingCbs[id] = undefined;

    this.radioCount++;

    if(this.radioCount === 2) {
      this.crossAttachPipe();
    }

    return id;
  }

  getRadio(id) {
    return r[id];
  }

  crossAttachPipe() {
    let r = this.r;

    r[0].tx.theirs.pipe(r[1].rx.theirs);
    r[1].tx.theirs.pipe(r[0].rx.theirs);
  }
}


module.exports = {
    MockEnvironmentPair
};