var jsc = require("jsverify");

const _ = require('lodash');
const mutil = require('../mock/mockutils.js');
const MapMov = require('../mock/mapmov.js');

var assert = require('assert');


let in0 = mutil.readHexFile('test/data/mapmov_in0.hex');
let ideal_out0 = mutil.readHexFile('test/data/mapmov_out0.hex');



describe("mapper mover", function () {

it('map mov', (done) => {

  let mapmov = new MapMov.MapMov();
  mapmov.reset();
  

  let got_out0 = mapmov.accept(in0);
  
  assert(_.isEqual(got_out0,ideal_out0));

  done();
});


it('converts map mov lengths correctly', (done) => {

  let mapmov = new MapMov.MapMov();
  mapmov.reset();
  

  for(let i = 0; i < 200; i++) {
    let input = 16 + (i*1024);
    let c1 = mapmov.getPreMapMovCount(input);
    let c2 = mapmov.getPostMapMovCount(c1);
    // console.log('if: ' + input + ' : ' + c1 + ' : ' + c2);

    assert( input == c2 );

  }
 
  
  // assert(_.isEqual(got_out0,ideal_out0));

  done();
});


});
// mapmov.reset();
// for(let i = 0; i < 200; i++) {
//   console.log('if: ' + i + ' : ' + mapmov.getPostMapMovCount(i));
// }

// for(let i = 0; i < 20496; i++) {
//   console.log('ir: ' + i + ' : ' + mapmov.getPreMapMovCount(i));
// }


// console.log('foo ' + mapmov.getPostMapMovCount(32));