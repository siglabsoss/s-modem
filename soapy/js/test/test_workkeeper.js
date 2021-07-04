var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const mt = require('mathjs');
const enumerate = require('pythonic').enumerate;
const bufferSplit = require('../mock/nullbuffersplit');
const workKeeper = require('../mock/workkeeper.js');

var assert = require('assert');

let print = false;



describe('Work Keeper', function () {

it('Work Keeper1', function(done) {
  this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let keeper = new workKeeper.WorkKeeper({tolerance:4});

    let start = Date.now();

    let count = 0;

    let runTest = true;


    let workOuter = () => {

      let now = Date.now();
      let delta = now - start;
      if( print ) {
        console.log("work at " + delta);
      }
      count++;

      if( runTest ) {
        let ms = jsc.random(0,200);

        setTimeout( ()=>{keeper.do(0,workOuter)}, ms);
      }
    }

    workOuter();

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(()=>{try {
        runTest = false;
      
        resolve(true);
      
      }catch(e){reject(e)}},2000);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('Work Keeper2', function(done) {
  this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let keeper = new workKeeper.WorkKeeper({tolerance:10});
    keeper.peers(2);

    let start = Date.now();
    let count0 = 0;
    let count1 = 0;
    let runTest = true;

    let pp = () => {
      if( print ) {
        console.log("( " + count0.toString().padEnd(3) + ', ' + count1.toString().padEnd(3) + " )");
      }
    }


    let workRandom = () => {
      let now = Date.now();
      let delta = now - start;
      // console.log("work0 at " + delta);
      pp();
      count0++;

      if( runTest ) {
        let ms = jsc.random(0,200);

        setTimeout( ()=>{keeper.do(0,workRandom)}, ms);
      }
    }

    let workConsistent = () => {
      let now = Date.now();
      let delta = now - start;
      // console.log("work1 at " + delta);
      pp();
      count1++;

      if( runTest ) {
        let ms = 10;

        setTimeout( ()=>{keeper.do(1,workConsistent)}, ms);
      }
    }

    workRandom();
    workConsistent();

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(()=>{try {
        runTest = false;
      
        resolve(true);
      
      }catch(e){reject(e)}},2000);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it



it('Work Keeper3', function(done) {
  this.timeout(4000);
  let t = jsc.forall('bool', function (junk) {

    let keeper = new workKeeper.WorkKeeper({tolerance:3,print:print});
    keeper.peers(2);

    let start = Date.now();
    let count0 = 0;
    let count1 = 0;
    let runTest = true;


    let work0 = () => {
      let now = Date.now();
      let delta = now - start;
      if(print) {
        console.log("                                    work0 at " + delta);
      }
      count0++;
    }

    let work1 = () => {
      let now = Date.now();
      let delta = now - start;
      if(print) {
        console.log("                                    work1 at " + delta);
      }
      count1++;
    }

    keeper.do(0,work0);
    keeper.do(1,work1);
    keeper.do(0,work0);
    keeper.do(0,work0);
    keeper.do(0,work0);
    keeper.do(1,work1);


    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {
      setTimeout(()=>{try {
        runTest = false;
      
        resolve(true);
      
      }catch(e){reject(e)}},2000);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it


it('Work Keeper4', function(done) {
  this.timeout(6000);
  let t = jsc.forall(jsc.integer(1,1000), function (tol) {
    // print = true;
    // let tol = 10;
    if( print ) {
      console.log("tol: " + tol);
    }

    let keeper = new workKeeper.WorkKeeper({tolerance:tol,print:false});
    keeper.peers(3);

    let start = Date.now();
    let count0 = 0;
    let count1 = 0;
    let count2 = 0;
    let runTest = true;

    // use setImmediate to allow the streams to fully flush
    // when the streams are done, we resolve with our pass / fail
    return new Promise(function(resolve, reject) {

      let pp = () => {
        if( print ) {
          console.log("( " 
            + count0.toString().padEnd(3) + ', '
            + count1.toString().padEnd(3) + ', '
            + count2.toString().padEnd(3) 
            + " )");
        }

          assert(Math.abs(count0-count1) <= (tol) );
          assert(Math.abs(count0-count2) <= (tol) );
          assert(Math.abs(count1-count2) <= (tol) );

          if( print ) {
            console.log(Math.abs(count0-count1));
            console.log(Math.abs(count0-count2));
            console.log(Math.abs(count1-count2));
          }

      }


      let workRandom = () => {
        let now = Date.now();
        let delta = now - start;
        // console.log("work0 at " + delta);
        pp();
        count0++;

        if( runTest ) {
          let ms = jsc.random(0,50);

          setTimeout( ()=>{keeper.do(0,workRandom)}, ms);
        }
      }

      let workConsistent0 = () => {
        let now = Date.now();
        let delta = now - start;
        // console.log("work1 at " + delta);
        pp();
        count1++;

        if( runTest ) {
          let ms = 4;

          setTimeout( ()=>{keeper.do(1,workConsistent0)}, ms);
        }
      }

      let workConsistent1 = () => {
        let now = Date.now();
        let delta = now - start;
        // console.log("work1 at " + delta);
        pp();
        count2++;

        if( runTest ) {
          let ms = 2;

          setTimeout( ()=>{keeper.do(2,workConsistent1)}, ms);
        }
      }

      workRandom();
      workConsistent0();
      workConsistent1();

      setTimeout(()=>{try {
        runTest = false;
      
        resolve(true);
      
      }catch(e){reject(e)}},500);
    }); // promise
  }); // forall

  // when using 'array nat' above jsverify internally limits the length of the array
  // to the log of the size param, while each element is a random value [0, size]
  let props = {size: 10000000000, tests: 5};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

  // props.rngState = "07526afb819c307d4e";
  // jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));
}); // it




}); // describe
