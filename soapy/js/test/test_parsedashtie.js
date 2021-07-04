'use strict';
var jsc = require("jsverify");
const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const enumerate = require('pythonic').enumerate;
// const bufferSplit = require('../mock/nullbuffersplit');
const parseDashTie = require('../mock/parsedashtie.js');

var assert = require('assert');

let input;

describe('Parse Dash', function () {

  before(()=>{
    input = [
 '[{"u":0},{"$set":{"fsm.state":1}}]',
 '[{"u":0},{"$set":{"fsm.control.should_mask_data_tone_tx_eq":true}}]',
 '[{"u":0},{"$set":{"fsm.control.should_run_background":false}}]',
 '[{"u":0},{"$set":{"fsm.control.should_mask_all_data_tone":false}}]',
 '[{"u":1},{"$set":{"fsm.state":1}}]',
 '[{"u":1},{"$set":{"fsm.control.should_mask_data_tone_tx_eq":true}}]',
 '[{"u":1},{"$set":{"fsm.control.should_run_background":false}}]',
 '[{"u":1},{"$set":{"fsm.control.should_mask_all_data_tone":false}}]',
 '[{"u":2000000},{"$set":{"fsm.state":45}}]',
 '[{"u":0},{"$set":{"sto.times_sto_estimated":0}}]',
 '[{"u":0},{"$set":{"sto.sto_estimated":0}}]',
 '[{"u":0},{"$set":{"sfo.sfo_estimated":0}}]',
 '[{"u":0},{"$set":{"sfo.sfo_estimated_sent":0}}]'];;

  });

it('Parse Dash1', function() {
  let parseDash = new parseDashTie.ParseDashTie();

  // parseDash.addData(input[0]);

  for(let x of input) {
    parseDash.addData(x);
  }


  assert(parseDash.uuid[0].fsm.state === 1);
  assert(parseDash.uuid[1].fsm.state === 1);
  assert(parseDash.uuid[0].fsm.control.should_run_background === false);
  assert(parseDash.uuid[1].fsm.control.should_run_background === false);
  assert(parseDash.uuid[0].sto.sto_estimated === 0);

  // console.log(parseDash.uuid);
  
}); // it


it('Parse Dash2', function() {
  let parseDash = new parseDashTie.ParseDashTie();

  // parseDash.addData(input[0]);

  let counter = 0;

  let cb = (x) => {
    assert.equal(x,1);
    assert.equal(counter,4);
    assert.equal(typeof parseDash.uuid[0].sto, 'undefined');

  }

  parseDash.notifyOn(1, 'fsm.state', cb);

  for(let x of input) {
    parseDash.addData(x);
    counter++;
  }
}); // it


it('Parse Dash3', function() {
  let parseDash = new parseDashTie.ParseDashTie();

  // parseDash.addData(input[0]);

  let counter = 0;

  let cb = (x) => {
    assert.equal(x,1);
    assert.equal(counter,4);
    assert.equal(typeof parseDash.uuid[0].sto, 'undefined');
  }

  let cbError = (code, str) => {
    // console.log("error on " + str);
    assert(str.indexOf('fsm.control.should_run_background') !== -1);
  }

  parseDash.notifyOn(1, 'fsm.state', cb);
  parseDash.notifyOnError(cbError);

  for(let x of input) {
    if( counter == 2 ) {
      parseDash.addData(x+'}');
    } else {
      parseDash.addData(x);
    }
    counter++;
  }
  
}); // it


}); // describe
