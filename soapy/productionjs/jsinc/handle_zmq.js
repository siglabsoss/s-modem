'use strict';

const hc = require('../../js/mock/hconst.js');
const mutil = require('../../js/mock/mockutils.js');
const safeEval = require('safe-eval');
const Rpc = require('grain-rpc/dist/lib');
const util = require('util');
const EventEmitter = require('events').EventEmitter;


function VecNotify() {
  EventEmitter.call(this);
  this.setMaxListeners(Infinity);
}
util.inherits(VecNotify, EventEmitter);



// copied from fbparse.js
function g_vtype(b) {
  return b[4];
}


class DispatchZmqClient {
  constructor(parent) {
    this.status = "idle";
    this.statusChecks = 0;
    this.parent = parent;
  }

  eval(cmd) {
    return safeEval(cmd, this.parent.context);
  }

  ddd() {
    console.log('ddddddd');
    return 'yyyy';
  }
}

class WrapZmqClient {
  constructor(parent, id, messageOut, print) {

    this.id = id;
    this.messageOut = messageOut;
    this.printRpc = print;

    let rpcArgs = {
      sendMessage: (m) => {
        if( this.printRpc ) {
          console.log('sending ' + JSON.stringify(m));
        }
        this.messageOut(m);
      }
    };

    if(!this.printRpc) {
      rpcArgs.logger = {info:undefined};
    }

    this.dispatch = new DispatchZmqClient(parent);

    this.rpc = new Rpc.Rpc(rpcArgs);
    this.rpc.registerImpl('client_' + id, this.dispatch);
  }

  messageIn(o) {
    this.rpc.receiveMessage(o);
  }
}



class DispatchZmqServer {
  constructor() {
  }
}

class WrapZmqServer {
  constructor(id, peer_id, messageOut, print) {

    this.id = id;
    this.peer_id = peer_id;
    this.messageOut = messageOut;
    this.printRpc = print;

    let rpcArgs = {
      sendMessage: (m) => {
        if( this.printRpc ) {
          console.log('sending ' + JSON.stringify(m));
        }
        this.messageOut(m);
      }
    };

    if(!this.printRpc) {
      rpcArgs.logger = {info:undefined};
    }

    this.dispatch = new DispatchZmqServer();

    this.rpc = new Rpc.Rpc(rpcArgs);
    this.rpc.registerImpl('server_' + id, this.dispatch);
    this.control = this.rpc.getStub('client_' + peer_id);
  }

  messageIn(o) {
    this.rpc.receiveMessage(o);
  }
}


class HandleZmq {
  constructor(sjs, role) {
    this.sjs = sjs;
    this.seq = 0;
    this.role = role;

    this.our_id = this.sjs.settings.getIntDefault(-1,"exp.our.id");
    this.master_id = this.sjs.settings.getIntDefault(-1,"exp.rx_master_peer");

    if( this.our_id === -1 ) {
      console.log("HandleZmq got something very wrong from settings: exp.our.id was missing!");
      exit(1);
    }

    this.isServer = (this.role === "rx") || (this.role === "arb");

    // Event Emitter for vector types
    this.notify = new VecNotify();

    this.evalHeader = '';


    setTimeout(()=>{
      if(this.isServer) {
        console.log("with role as '" + this.role + "' using Server Mode");
      } else {
        console.log("with role as '" + this.role + "' using Client Mode");
      }


      this.peers = sjs.allPeers();

      this.dispatchPrint = false;


      this.wrap = [];
      this.wrapServer = [];
      if(this.isServer) {

        for( let p of this.peers ) {
          this.setupMasterRpc(this.our_id, p);
        }

        if( role === "arb" ) {
          this.setupMasterRpc(this.our_id, this.master_id); // here master id is really rx id
        }

      } else {
        this.setupClientRpcForMaster(this.master_id);
      }

    },1000);

    this.bindTypes();
  }

  // turns out if we have two masters (arb / rx) talking to one client
  // we need an instance of the client rpc for each master.
  // this is because the client needs a way to know who to send the reply to
  // grain rpc doesn't give us an easy way to tag messages so we do this
  // seems complicated but should be perfect once done
  setupClientRpcForMaster(master_peer_id) {
    // console.log(master_peer_id);
    if( typeof this.wrap[master_peer_id] === 'undefined' ) {
      // console.log("late binding for " + master_peer_id);
      let messageOutToMaster = (o) => {
        // console.log("last bind send " + this.our_id + ", " + master_peer_id);
        // console.log(o);
       this.sendFromForPartner(this.our_id, master_peer_id, o);
     };
      this.wrap[master_peer_id] = new WrapZmqClient(this, this.our_id, messageOutToMaster, this.dispatchPrint);
    }
  }

  setupMasterRpc(our_id, peer_id) {
    let messageOutForPeer = (o) => { this.sendFromForPartner(our_id, peer_id, o); }
    this.wrapServer[peer_id] = new WrapZmqServer(our_id, peer_id, messageOutForPeer, this.dispatchPrint);
  }

  r(index) {
    return this.wrapServer[this.peers[index]].control;
  }

  id(peer_id) {
    return this.wrapServer[peer_id].control;
  }

  bindTypes() {
    this.sjs.dsp.zmqCallbackCatchType(hc.FEEDBACK_VEC_JS_ARB);
    this.sjs.dsp.zmqCallbackCatchType(hc.FEEDBACK_VEC_JS_INTERNAL);
    this.sjs.dsp.registerCallback(this.gotZmq.bind(this));
  }

  gotZmq(x) {
    // console.log('zmq ' + x.toString(16));
    let vtype = g_vtype(x);
    switch(vtype) {
      case hc.FEEDBACK_VEC_JS_ARB:
        this.gotArb(x);
        break;
      case hc.FEEDBACK_VEC_JS_INTERNAL:
        this.evalWords(x);
        break;
      default:
        let header = x.slice(0,16);
        let body   = x.slice(16);
        this.notify.emit(''+vtype, header, body);
        break;
    }
  }

  gotArb(x) {
    // console.log('gotArb');
    let fromWord = x.slice(16,17)[0];
    let body = x.slice(17);

    // console.log(body);

    let bodyBytes = mutil.IShortToStreamable(body);
    // console.log(bodyBytes);
    let asString = new TextDecoder("utf-8").decode(bodyBytes);
    // console.log(fromWord);
    // console.log(asString);

    this.tryEvalParse(fromWord, asString);
  }

  tryEvalParse(from, asString) {
    try {
      let o = JSON.parse(asString);
      // this.gotValidJson(o);
      this.feedRpc(from, o);
    } catch (err) {
      if (err instanceof SyntaxError) {
        console.log('tryEvalParse() Json parse error');
      }
    }
  }



  feedRpc(from, o) {

    if(this.isServer) {
      // console.log("As server got rpc from " + from);

      if(typeof this.wrapServer[from] !== 'undefined' ) {
        this.wrapServer[from].messageIn(o);
      } else {
        this.setupClientRpcForMaster(from);
        this.wrap[from].messageIn(o);
      }

    } else {
      // console.log("As client got rpc from " + from);
      // first time this clinet gets a message from the arb, this will be created
      this.setupClientRpcForMaster(from);
      this.wrap[from].messageIn(o);
      
    }
  }

  setContext(c) {
    this.context = c;


    let build = "";
    for(let y in this.context) {
      build += 'let ' + y + '=this.context[\'' + y + '\'];';
    }
    build += 'let zmq = this;';
    this.evalHeader = build + ' ';
  }

  evalWords(x) {
    // console.log("SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSsafeEvalWords");
    // return safeEval(cmd, this.parent.context);
    const body   = x.slice(16);

    let bodyBytes = mutil.IShortToStreamable(body);
    // console.log(bodyBytes);
    let asString = new TextDecoder("utf-8").decode(bodyBytes);

    // console.log(bodyBytes);
    console.log(asString);

    this.eval(asString);
  }

  eval(x) {
    eval(this.evalHeader + x);
    // setImmediate(function(){eval(____wrap + ' ' + x)}.bind(this.context));
  }

  // sends an object over zmq to a peer_id
  sendForPartner(peer_id, o) {
    let asWords = this.objectToWords(o);

    this.sjs.dsp.sendVectorTypeToPartnerPC(peer_id, hc.FEEDBACK_VEC_JS_ARB, asWords);
  }

  sendFromForPartner(from_peer_id, peer_id, o) {
    const asWords = this.objectToWords(o);

    const finalWords = [from_peer_id].concat(asWords);

    this.sjs.dsp.sendVectorTypeToPartnerPC(peer_id, hc.FEEDBACK_VEC_JS_ARB, finalWords);
  }

  objectToWords(o) {
    let j = JSON.stringify(o);

    let jmod = 4-(j.length % 4);

    // pad length to multiple of 4
    for(let i = 0; i < jmod; i++) {
      j += ' ';
    }
    // console.log(j);
    
    let asBytes = new TextEncoder("utf-8").encode(j);
    // console.log(asBytes);

    let asWords = mutil.streamableToIShort(asBytes);
    // console.log(asWords);

    return asWords;
  }
}


module.exports = {
    HandleZmq
}
