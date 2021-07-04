'use strict';
const Stream = require("stream");
const _ = require('lodash');
const mt = require('mathjs');
const mutil = require("../mock/mockutils.js");
// const kindOf = require('kind-of');


// Wrap a native stream (stream chain) object so that it behaves like a stream
class WrapNativeTransformStream extends Stream.Transform {
  
  constructor(options, options2={}, wrapped, afterPipe, afterShutdown){
    super(options);

    Object.assign(this, options2);

    this.nativeStream = wrapped;
    this.afterShutdown = afterShutdown;
    this.afterPipe = afterPipe;


    this.on('pipe', this.didPipe.bind(this));
  }

  didPipe() {
    if( this.print ) {
      console.log('did pipe');
    }
    this.nativeStream.setStreamCallbacks(this._gotDataFromNative.bind(this),this._gotNativeShutdown.bind(this));
    // this.nativeStream.setStreamCallbacks(this.gotDataFromNative.bind(this),this.gotNativeShutdown);

    if( typeof this.args !== 'undefined' ) {
      this.nativeStream.startStream(...this.args); // starts threads with arguments
    } else {
      this.nativeStream.startStream(); // starts threads
    }

    if( typeof this.afterPipe !== 'undefined') {
      this.afterPipe(this);
    }
  }

  // wrap in setImmediate becuase I saw issues with similar callbacks having issues resolving Promises 
  _gotDataFromNative(d) {
    if( this.print ) {
      console.log('_gotDataFromNative');
    }
    setImmediate(this.gotDataFromNative.bind(this,d));
  }

  gotDataFromNative(d) {
    // console.log('foo');
    // console.log(d);
    this.push(d);
  }

  _gotNativeShutdown() {
    setImmediate(this.gotNativeShutdown.bind(this));
  }
  // _gotNativeShutdown() {
  //   if( this.print ) {
  //     console.log('_gotNativeShutdown ' + this.objBuilt);
  //   }
  //   if( typeof this.afterShutdown !== 'undefined') {
  //     // console.log(this.afterShutdown.toString());
  //     setImmediate(this.afterShutdown(this));
  //   }
  // }

  gotNativeShutdown() {
    if( this.print ) {
      console.log('_gotNativeShutdown');
    }
    if( typeof this.afterShutdown !== 'undefined') {
      // console.log(kindOf(this.afterShutdown));
      // console.log(this.afterShutdown.toString());
      setImmediate(()=>{this.afterShutdown(this);});
    }
  }

  stop() {
    this.nativeStream.stopStream();
  }

  
  // advance(count) {
  //   this.pending_advance += count;
  // }

  _transform(chunk, encoding, cb) {
    // console.log('got chunk ' + chunk.length);

    this.nativeStream.writeStreamData(chunk, chunk.length);

    cb();
  }
}







// Wrap a native stream (stream chain) object so that it behaves like a stream
class WrapCoarseChain extends WrapNativeTransformStream {
  
  constructor(options, options2={}, wrapped, afterPipe, afterShutdown, callbackAux0){
    super(options, options2, wrapped, afterPipe, afterShutdown);

    this.callbackAux0 = callbackAux0;
  }

  didPipe() {
    if( this.print ) {
      console.log('did pipe');
    }
    this.nativeStream.setStreamCallbacks(this._gotDataFromNative.bind(this),this._gotNativeShutdown.bind(this),(a)=>{setImmediate(this.callbackAux0.bind(this,a))});

    if( typeof this.args !== 'undefined' ) {
      this.nativeStream.startStream(...this.args); // starts threads with arguments
    } else {
      this.nativeStream.startStream(); // starts threads
    }

    if( typeof this.afterPipe !== 'undefined') {
      this.afterPipe(this);
    }
  }

  triggerCoarse() {
    this.nativeStream.triggerCoarse();
  }

  triggerCoarseAt(frame) {
    this.nativeStream.triggerCoarseAt(frame);
  }

}


// Wrap a native stream (stream chain) object so that it behaves like a stream
class WrapRxChain1 extends WrapCoarseChain {
  
  constructor(options, options2={}, wrapped, afterPipe, afterShutdown, callbackAux0){
    super(options, options2, wrapped, afterPipe, afterShutdown, callbackAux0);

    // this.callbackAux0 = callbackAux0;
  }

  advance(a) {
    this.nativeStream.turnstileAdvance(a);
  }


  // didPipe() {
  //   if( this.print ) {
  //     console.log('did pipe');
  //   }
  //   this.nativeStream.setStreamCallbacks(this._gotDataFromNative.bind(this),this._gotNativeShutdown.bind(this),(a)=>{setImmediate(this.callbackAux0.bind(this,a))});

  //   // FIXME set ofdm frames here
  //   this.nativeStream.startStream(); // starts threads

  //   if( typeof this.afterPipe !== 'undefined') {
  //     this.afterPipe(this);
  //   }
  // }

  // triggerCoarse() {
  //   this.nativeStream.triggerCoarse();
  // }

  // triggerCoarseAt(frame) {
  //   this.nativeStream.triggerCoarseAt(frame);
  // }

}







module.exports = {
    WrapNativeTransformStream
  , WrapCoarseChain
  , WrapRxChain1
};
