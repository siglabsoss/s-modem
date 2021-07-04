'use strict';

const Stream = require("stream"); //require native stream library
const mt = require('mathjs');
const mutil = require('./mockutils.js');
var kindOf = require('kind-of');
const _ = require('lodash');
const pipeType = require('../mock/pipetypecheck.js');

// const binary = require('bops');




// id of next attached radio
let radioCount = 0;
let r = [];

let datas = [];

let matrix = [[]];

let then;
let now;

let timesWrittenBack = 0;

let sampleCountAccumulated = 0;
let sampleCountLatched = 0;

let rateLimit = 0;


// this function gets called so we can append the existing data buffer for each radio at the correct place
function getWriteIndexFor(id) {
  return datas[id].length;
}

let outstandingCbs = [];

// this gets called by each Stream.Writable after new data has been received
// we hold onto the callback object, and we will optionally call it later, or possibly now
// this function checks to see if each attached Stream.Writable has written something.
// we check this by looking at outstandingCbs
function writeCompleted(id, cb) {

  // console.log(`${id} entering writeCompleted`);
  // console.log(JSON.stringify(outstandingCbs));
  // console.log(typeof outstandingCbs[id]);

  if( typeof outstandingCbs[id] !== 'undefined' ) {
    throw(new Error('writeCompleted sees existing callback, something is not right'));
  }

  // console.log(`${id} check ok`);

  outstandingCbs[id] = cb;
  // console.log(outstandingCbs);

  let countOk = 0;

  for(let i = 0; i < radioCount; i++) {
    let res = typeof outstandingCbs[i] !== 'undefined';
    if( res ) {
      countOk ++;
    }
    // console.log('' + id + ' checking:' + i + ' result: ' + res);
  }

  // console.log(countOk + " - " + countOk);
  // console.log();

  if(countOk === radioCount) {
    // console.log('ok to go');
    writeReceivedData();
  }
}

// only after each incoming stream has written something new
// we can only run through a number of samples equal to the shortest buffer we've received
// loop through buffers and write combined data as if it were recieved over the air
// after we combine the samples, we write it back to each Stream.Readable
function writeReceivedData() {
  // console.log('writeReceivedData');

  // how many complex samples are shared by all buffers
  let thisRun = _.min(datas.map((x)=>x.length));

  let a,b; // indices into the phase matrix

  for(let rxradio = 0; rxradio < radioCount; rxradio++) {
    
    let thebuffer = new ArrayBuffer(thisRun*8*2); // in bytes
    let uint8_view = new Uint8Array(thebuffer);
    let float64_view = new Float64Array(thebuffer);

    for(let i = 0; i < thisRun; i++) {

      let sam = mt.complex(
        0.0075*Math.random()-0.00375,
        0.0075*Math.random()-0.00375
        );

      for(let txradio = 0; txradio < radioCount; txradio++) {
        if(rxradio === txradio) {
          continue; // FIXME this forces 'hearself' to false
        }

        [a,b] = mutil.lowerTriangular(txradio,rxradio);

        let phasor = matrix[a][b];

        sam = sam.add( datas[txradio][i].mul(phasor) );
        // console.log('Phasor between ' + rxradio + ' and ' + txradio + ' is ' + phasor.toString());

      } // for tx radio

      // sam is now a complex object with contributions from each radio
      // we convert it into float,float (real,imag)

      float64_view[i*2    ] = sam.re;
      float64_view[(i*2)+1] = sam.im;

      // if(rxradio === 1 && i < 16) {
      //   console.log(sam.re);
      // }

    } // for sample of rx radio

    // console.log('done with rx for radio ' + rxradio);
    // console.log(float64_view);

    // push data back into the radio
    // console.log("r" + rxradio + " " + uint8_view);
    r[rxradio].rx.ours.push(uint8_view);

  } // for rx radio

  let unblockSystem = () => {

    // second pass to cleanup and callback
    for(let rxradio = 0; rxradio < radioCount; rxradio++) {
      var fncopy = outstandingCbs[rxradio]; // copy the callback
      outstandingCbs[rxradio] = undefined; // wipe out the entry, this is for writeCompleted()
      datas[rxradio] = datas[rxradio].slice(thisRun); // delete our copy of the data

      //  call the callback, this will unblock the incoming stream for this radio
      // seems like this needs to be inside setImmediate or else streams will get data before this loop gets
      // to the next iteration
      setImmediate(fncopy);
    }
  }




  now = Date.now();

  sampleCountAccumulated += thisRun;

  const averagePeriod = 100; // ms

  let delayBy = 0;

  // how many times has this function been called?
  if( timesWrittenBack > 0) {
    if( rateLimit != 0 ) {
      let deltaMs = now - then;
      if( deltaMs >= averagePeriod ) {

        // console.log(now + ', ' + sampleCountAccumulated);
        // how many samples did we chew through since last check
        let deltaSamples = sampleCountAccumulated - sampleCountLatched;
        // how many should we expect if perfectly at rate limit
        let expectedForPeriod = deltaMs * (rateLimit/1000);
        // console.log('expected: ' + expectedForPeriod);
        // console.log('deltaMs: ' + deltaMs + ' samples since last check: ' + deltaSamples + ' samples at ' + sampleCountAccumulated/125E6);

        if( deltaSamples > expectedForPeriod ) {
          let rateForThisPeriod = deltaSamples / averagePeriod; // in samples per ms
          // how many samples did we do extra
          let overage = deltaSamples - expectedForPeriod;

          let desiredDelay = overage / rateForThisPeriod;

          let desiredDelayRound = Math.round(desiredDelay);

          if( desiredDelayRound > 0 ) {
            delayBy = desiredDelayRound;
          }

          // console.log('overage: ' + overage + ' delay by ' + desiredDelay + ' ms at rate ' + rateForThisPeriod);
        }

        then = now;
        sampleCountLatched = sampleCountAccumulated;
      }
    }
  } else {

    then = now;
  }

  if( delayBy === 0 ) {
    unblockSystem();
  } else {
    setTimeout(unblockSystem, delayBy);
  }

  // console.log('writeReceivedData exiting');

  timesWrittenBack++;

}

/*
Implementing our own stream, It should extend native stream class.
Since we are implementing wrtiable stream it should extend native
writable class (stream.Writable)
*/

// the radio's tx pipe will pipe into this writable
class myWritable extends Stream.Writable {
  //Every class should have a constructor, which takes options as arguments. (REQUIRED)
  
  constructor(options,opts){
    super(options) //Passing options to native constructor (REQUIRED)

    this.id = opts.id;

    this.name = 'Environment tx access port for radio ' + this.id;
    this.pipeType = {in:{type:'IFloat',chunk:'any'},out:{type:'any',chunk:'any'}}; // out doesn't matter here
    this.on('pipe', this.pipeTypeCheck.bind(this));
  }
  
  pipeTypeCheck(them) {
    // console.log(this.name + " got pipe from");
    // console.log(them);
    pipeType.warn(this,them);
  }

  /*
    Every stream class should have a _write method which is called intrinsically by the api,
    when <streamObject>.write method is called, _write method is not to be called directly by the object.
  */

  // consider using https://www.npmjs.com/package/buffer-dataview which may be faster?
  
  _write(chunk,encoding,cb){
    // let tag = Math.floor(Math.random() * 100);


    // let uint8 = binary.readUInt8(chunk, 0);

    // console.log()
    // console.log('t chunk ' + kindOf(chunk));
    // console.log('t2 ' + kindOf(uint8));

    // Uint32Array

    // console.log('' + this.id + ' got '+ chunk.length);

    // console.log(r[this.id].tx.state.theirs.writeCount);


    // sadly this does a buffer copy accorind to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    let uint8_view = new Uint8Array(chunk, 0, chunk.length);

    // let uint32_view = new Uint32Array(uint8_view);

    var dataView = new DataView(uint8_view.buffer);

    // console.log(uint8_view);
    // console.log(uint32_view);

    let writeAt = getWriteIndexFor(this.id);

    // console.log('' + this.id + ' writing to: ' + writeAt);

    for(let i = 0; i < chunk.length/8; i+=2) {
      // console.log(dataView.getFloat64(i*8, true));

      let cplx = mt.complex(
        dataView.getFloat64(i*8, true),
        dataView.getFloat64((i*8)+8, true) );

      datas[this.id][writeAt] = cplx;

      writeAt++;

      // console.log(cplx.toString());

    }

    r[this.id].tx.state.theirs.writeCount += chunk.length;

    // console.log("r" + this.id + " " + r[this.id].tx.state.theirs.writeCount);

    // console.log('entered _write ' + tag + ' -> ' + JSON.stringify(chunk));
    // console.log('writableLength ' + this.writableLength);

    // we pass the cb function to this so that we can call all the cb()'s together once the received data
    // has been calculated.  this form of pushback will guarentee that the write streams don't get too far ahead of
    // the read streams
    writeCompleted(this.id, cb);

  }

}


class myReadable extends Stream.Readable{
  
    //Every class should have a constructor, which takes options as arguments. (REQUIRED)
  
    constructor(options, opts){
        super(options); //Passing options to native class constructor. (REQUIRED)

        this.id = opts.id;
    }

    // we don't use this function, however we need to define it
    // instead we call .push() from outside this class
    _read(sz){
    }
}


/*
  options can be:
   - decodeStrings: If you want strings to be buffered (Default: true)
   - highWaterMark: Memory for internal buffer of stream (Default: 16kb)
   - objectMode: Streams convert your data into binary, you can opt out of by setting this to true (Default: false)
*/


function attachRadio(readableTx, writableRx) {
  let id = radioCount;

  // handle the dut -> env
  // highWaterMark:2048
  // the radio's tx pipe will pipe into this writable
  let myStream = new myWritable({decodeStrings:false},{id:id});

  readableTx.pipe(myStream);

  // handle env -> dut

  let writeToDut = new myReadable({decodeStrings:false},{id:id});

  writeToDut.pipe(writableRx);


  // tx/rx in this context are relative to DUT (why)
  let obj = {
    tx:{theirs:readableTx,ours:myStream,state:{theirs:{writeCount:0}}},
    rx:{theirs:writableRx,ours:writeToDut,state:{theirs:{writeCount:0}}}
  };

  r[id] = obj;
  datas[id] = [];
  outstandingCbs[id] = undefined;

  radioCount++;
  return id;
}

function getRadio(id) {
  return r[id];
}




/*
  When writable stream finishes writing data it fires "finish" event,
  When readable stream finishes reading data it fires "end" event.
  
  if you have piped a writable stream, like in our case
  it ends the writable stream once readable stream ends reading data by calling 
  <writableStreamObject>.end() method (you can use this manually in other cases)
*/

// fileStream.on("end",()=>{
//   console.log("Finished Reading");
// });

// myStream.on("finish",()=>{
//   console.log("Finished Writing");
// });

function reset(options={}) {
  radioCount = 0;
  r = [];
  datas = [];
  outstandingCbs = [];

  if(typeof options.rateLimit !== 'undefined') {
    rateLimit = options.rateLimit;
  }

  // FIXME make this dynamic somehow
  // its hardcoded for 3 or less radios right now

// >>> np.exp(1j*2) * 0.9
// (-0.3745321528924282+0.8183676841431136j)
// >>> np.exp(1j*4) * 0.8
// (-0.5229148966908895-0.6054419962463427j)
// >>> np.exp(1j*5) * 0.6
// (0.17019731127793575-0.575354564797883j)
// >>> 


  matrix = [[],[],[]];
  for(let i = 0; i < 3; i++) {
    for(let j = 0; j < 3; j++) {
      matrix[i][j] = mt.complex(0,0);
    }
  }

  let a,b;
  [a,b] = mutil.lowerTriangular(0,1);
  matrix[a][b] = mt.complex(-0.3745321528924282,0.8183676841431136);
  // matrix[a][b] = mt.complex(1.0,0);

  [a,b] = mutil.lowerTriangular(0,2);
  matrix[a][b] = mt.complex(-0.5229148966908895,0.6054419962463427);

  [a,b] = mutil.lowerTriangular(1,2);
  matrix[a][b] = mt.complex(0.17019731127793575,0.575354564797883);
}

module.exports = {
    attachRadio
  , getRadio
  , reset
};