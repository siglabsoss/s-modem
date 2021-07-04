const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
// hack and slash version of worker thread for fft
// however this version only took my code from 3.0k ofdm fps -> 3.3 ofdm fps.
// imo not worth it for complexity

// https://stackoverflow.com/questions/16071211/using-transferable-objects-from-a-web-worker
function workerThreadFn() {

  let that = {};

  that.n = 1024;
  that.inverse = true;

  that.fft = new FFT.complex(that.n, that.inverse);

  parentPort.on('message', (payloadIn)=>{
    // let a = msg.bufferInput;
    // console.log(msg);

    let chunkCopy = payloadIn.chunk;

    const ffttype = 'complex';

    const width = 8*2;

    // how many uint32's (how many samples) did we get
    const sz = 1024;


    var dataView = new DataView(chunkCopy);


    // it's possible that a proxy may be faster here instead of building bufferInput/fftInput
    // https://ponyfoo.com/articles/es6-proxies-in-depth
    
    let bufferInput = new ArrayBuffer(sz*width); // in bytes
    let bufferOutput = new ArrayBuffer(sz*width); // in bytes

    // this.w.postMessage(bufferInput,[bufferInput]);


    let fftInput = new Float64Array(bufferInput);
    let fftOutput = new Float64Array(bufferOutput);
    // let uint8_out_view = new Uint8Array(bufferOutput);


    // in place modify of the copy
    for(let i = 0; i < sz*2; i+=2) {
      let re = dataView.getFloat64(i*8, true);// * this.gain;
      let im = dataView.getFloat64((i*8)+8, true);// * this.gain;
      // console.log('re: ' + re + ' im: ' + im);
      fftInput[i] = re;
      fftInput[i+1] = im;
    }

    that.fft.simple(fftOutput, fftInput, ffttype);

    const payloadOut = {id:payloadIn.id, bufferOutput:bufferOutput};

    parentPort.postMessage(payloadOut, [payloadOut.bufferOutput]);
    // console.log('finished');


  });
}

if(isMainThread) {
  // mainThreadFn();
} else { //the worker's code
  workerThreadFn();
}

// pass size and cp length
class CF64StreamFFT extends Stream.Transform {
  constructor(options, options2){
    super(options); //Passing options to native constructor (REQUIRED)

    // this.gain = 1 / ((2**15)-1);

    Object.assign(this, options2);

    // this.fft = new FFT.complex(this.n, this.inverse);

    this.w = new Worker(__filename, {workerData: null});

    this.w.on('error', console.error);
    this.w.on('exit', (code) => {
        if(code != 0)
            console.error(new Error(`Worker stopped with exit code ${code}`))
    });

    this.callbacks = {};

    this.chunkId = 0;
    // w.postMessage('foo');


    this.w.on('message', (payloadIn) => { //A message from the worker!
      // console.log('got res');

    const width = 8*2;


      let uint8_out_view = new Uint8Array(payloadIn.bufferOutput);


        // console.log("First value is: ", msg.val);
        // console.log("Took: ", (msg.timeDiff / 1000), " seconds");


      // push cp
      if(this.cp !== 0) {
        // could be constant
        const n_byte = this.n * width;
        const cp_byte = this.cp * width;
        this.push(uint8_out_view.slice(n_byte-cp_byte));
      }

      // push output
      this.push(uint8_out_view);

      // this.callbacks[payloadIn.id]();
      // delete this.callbacks[payloadIn.id];
      // payloadIn.cb();

      });


  }
  
  _transform(chunk,encoding,cb) {

    // console.log(kindOf(chunk));


    const ffttype = 'complex';

    // width of input in bytes
    const width = 8*2;

    // how many uint32's (how many samples) did we get
    const sz = chunk.length / width;

    if( sz != this.n ) {
      throw(new Error('Illegal chunk size for CF64StreamFFT: ' + sz + ' expecting: ' + this.n));
    }

    // chaos in this thread:
    // https://stackoverflow.com/questions/8609289/convert-a-binary-nodejs-buffer-to-javascript-arraybuffer
    let chunkCopy = chunk.buffer.slice(chunk.byteOffset, chunk.byteOffset + chunk.byteLength);

    let thisId = this.chunkId;

    // this.callbacks[thisId] = cb;
    // this.chunkId++;


    // console.log('push res');

    const payloadOut = {id:thisId, chunk:chunkCopy};
    // arg1 a buffer or object with buffer members
    // arg2 a list of members of arg1 which should be moved and not copied
    // https://stackoverflow.com/questions/16071211/using-transferable-objects-from-a-web-worker
    this.w.postMessage(payloadOut, [payloadOut.chunk]);

    // sadly this does a buffer copy according to 
    // https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
    // const uint8_view = new Uint8Array(chunk, 0, chunk.length);

    setImmediate(cb);
    // cb();



  }
}
