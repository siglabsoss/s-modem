const mt = require('mathjs');
const fs = require('fs');
const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
// const enumerate = require('pythonic').enumerate;

let plan_user0 = [];

for(let n = 0; n <= 30; n++){
  let a = 4*n+6;
  let b = 4*n+2;
  plan_user0.push([a,b]);
  // console.log(`${a} vs ${b}`);
}

let plan_user1 = [];

for(let n = 1; n <= 31; n++){
  let a = 4*n+4;
  let b = 4*n;
  plan_user1.push([a,b]);
  // console.log(`${a} vs ${b}`);
}

// frame should be 1024 Complex values
// tones should be an array like [[2,6],[6,10],...]
// where the 1st arguemnt is a, and the 2nd is be in x[a]*conj(x[b])
function calculateSfo(frame, tones) {

  const len = tones.length;
  let sum = mt.complex(0,0);
  for(let i = 0; i < len; i++) {
    let a = tones[i][0];
    let b = tones[i][1];
    sum = sum.add( frame[a].mul( mt.complex(frame[b]) ));
  }

  // console.log(sum);

  // let dummy = mt.complex(1,0);
  let ret = mutil.riscvATANComplex(sum);
  let angle = ret & 0xffff;
  return angle;
}


function calculateSfoUser0(frame) {
  return calculateSfo(frame, plan_user0);
}

function calculateSfoUser1(frame) {
  return calculateSfo(frame, plan_user1);
}


module.exports = {
    calculateSfo
  , calculateSfoUser0
  , calculateSfoUser1
};