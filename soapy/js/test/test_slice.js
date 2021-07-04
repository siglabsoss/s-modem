'use strict';
var jsc = require("jsverify");

const _ = require('lodash');
const FBParse = require("../mock/fbparse.js");
const mutil = require('../mock/mockutils.js');
var assert = require('assert');
const hc = require('../mock/hconst.js');
const streamers = require('../mock/streamers.js');
const fs = require('fs');

const FBTag = require("../mock/fbtag.js");


let raw1;
let raw18;
let raw19;

describe("Demod Mapmov", function () {

before(()=>{
  raw1 = mutil.readHexFile('/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_feedback_bus_14/cs20_out.hex');
  // raw1 = raw1.slice(0, 1024*128);
});


// this example shows how to make a test pass or fail
// inside a callback using a promise
it.skip('slice1', function(done) {
  this.timeout(60000);

  const resolvingPromise = new Promise( (resolve, reject) => {

    let dout = [];

    let mapmovData = (sliced_out) => {
      // for(let y of x) {
      //   dout.push(mutil.bitMangleSliced(y));
      // }

      for(let i = 0; i < sliced_out.length; i++) {
        let j = mutil.transformWords128(i);
        dout.push(mutil.bitMangleSliced(sliced_out[j]));
      }
    }


    let source = new streamers.StreamArrayType({},{type:'float64', chunk:1024*2});

    let tagger = new FBTag.Tagger({},{onMapmovData:mapmovData, printSlicer:false});

    let cap = new streamers.ByteCaptureStream({});

    
    let len = raw1.length;

    let mod = len % 1024;

    if( mod !== 0 ) {
      len -= mod;
    }

    console.log(len);

    let raw2 = raw1.slice(0,len);

    let len2 = raw2.length;

    let raw3 = [];

    for(let i = 0; i < len2; i+=1024) {
      let chunk = raw2.slice(i+1, i+1024);
      chunk.reverse();

      raw3.push(0);
      raw3 = raw3.concat(chunk);

      // console.log(chunk);
    }

    // console.log(raw3);

    let ifloat = mutil.IShortToIFloatMulti(raw3);


    source.load(ifloat);

    source.pipe(tagger).pipe(cap);

    let check = () => {

      // console.log(cap.capture);

      // for( let x of dout ) {
      //   console.log(x.toString(16));
      // }
      let valid = false;
      let x_p;
      let discon = [];
      let i = 0;

      for( let x of dout ) {

        if( i >= 8200 ) {
          break;
        }

        if( x === 0x24782427 ) {
          valid = true;
          x_p = x;
          i++;
          continue;
        }

        if( valid ) {
          if( x !== x_p+1 ) {
            discon.push(i);
          }
        }
        x_p = x;
        i++;
      }

      for( let x of discon ) {
        console.log('discon: ' + x);
      }

      console.log("dout len " + dout.length);


      for( let i = 193; i < 8200 + 16; i++ ) {
        console.log(dout[i].toString(16));
      }


      setTimeout(resolve, 1);
    };

    setTimeout(check, 2000);
    

  });

  resolvingPromise.then( (result) => {

    done();
  });
});











}); // describe

