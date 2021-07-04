'use strict';
const net = require('net');
const Rpc = require('grain-rpc/dist/lib');
const bufferSplit = require('../mock/nullbuffersplit');
const { spawn } = require('child_process');
const _ = require('lodash');


// different but parallal to mockhiggs.js:portNumbers()
function portNumbers(idx) {
  let obj = {net:{higgs_udp:{},zmq:{}}};
  let bump = idx*10;

  obj.net.higgs_udp.rx_cmd  = (10001+bump);
  obj.net.higgs_udp.tx_cmd  = (20000+bump);
  obj.net.higgs_udp.tx_data = (30000+bump);
  obj.net.higgs_udp.rx_data = (40001+bump);

  obj.net.zmq.listen = (10005+bump);
  
  return obj;
}



class ServerControl {
  constructor(parent, id) {
    this.id = id;
    this.parent = parent;
  }

  getId() {
    return this.id;
  }
}

class LauncherServer {

  constructor(options={}) {
    // grab defaults from options
    const { printRpc = false, printConnections = false, printStatusLoop = true, launcherPort = 10009, debugLaunchKill = false } = options;
  
    // load defaults into object
    this.printRpc = printRpc;
    this.printConnections = printConnections;
    this.launcherPort = launcherPort;
    this.debugLaunchKill = debugLaunchKill;
    this.printStatusLoop = printStatusLoop;

    // locals
    this.peers = [];
    this.peerIdUnique = 0;
    this.waitingForPromise = null; // used in waitForIdle()
    this.waitingForCount = null;  // used in waitForIdle()
    this.remoteLaunchDir = null;
    this.doSelfLaunch = false;

    this.createAndOpenSocket();

    // dirname is "higgs_sdr_rev2/libs/s-modem/soapy/js/mock"
    this.setLaunchDirectory(__dirname + '/../../build');

    this.listPeersInterval = setInterval(this.listPeers.bind(this), 1000);
    this.talkPeersInterval = setInterval(this.talkToPeers.bind(this), 1000);

    this.currentLaunch = null;
  }

  setLaunchDirectory(dir) {
    this.remoteLaunchDir = dir;
  }

  selfLaunch() {
    console.log('set self launch');
    this.doSelfLaunch = true;
    return this;
  }

  _performSelfLaunch(n) {
    if( !this.doSelfLaunch ) {
      return;
    }
    const nodePath = process.execPath;

    const defaults = {
      cwd: process.cwd(), // call this function to get wd of this process
      env: process.env
    };

    const args = [];

    args.push('mock/launcherclient.js');
    args.push('--close');

    this.launcherClientHandles = [];

    for(let i = 0; i < n; i++) {
      // console.log(defaults.cwd);
      // console.log(nodePath);
      // console.log(args);
      let handle = spawn(nodePath, args, defaults);

      handle.stdout.on('data', (data) => {
        let asStr = data.toString('ascii');
        process.stdout.write(asStr); // without newline
      });

      this.launcherClientHandles.push(handle);
    }

  }

  // waits for N launchers to be alive
  waitForIdle(n) {
    this._performSelfLaunch(n);
    console.log("will wait for " + n);
    this.waitingForCount = n;
    let resolveCopy;
    let rejectCopy;
    this.waitingForPromise = new Promise(function(resolve, reject) {
      resolveCopy = resolve;
      rejectCopy = reject;
    });
    // non standard, bind the functions to the object itself. this allows us to resolve "outside" the callback
    this.waitingForPromise.resolve = resolveCopy;
    this.waitingForPromise.reject = rejectCopy;
    return this.waitingForPromise;
  }

  getUnique() {
    this.peerIdUnique++;
    return this.peerIdUnique;
  }

  // called by the socket when a launchClient connects
  addPeer(sock) {
    let unique = this.getUnique();

    // Important this is the format that many other functions interact with
    var hiveStructure = { 
      id:unique,
      socket:sock,
      state:{}
    };

    this.peers.push(hiveStructure);
    return unique;
  }

  getHive(id) {
    return this.getPeer(id);
  }

  getPeer(id) {
    for(let p of this.peers) {
      if(p.id === id) {
        return p;
      }
    }
    return undefined;
  }

  // returns number of connected peers
  getConnectedPeerCount() {
    return this.peers.length;
  }

  // a promise which resolves with a number of connected status
  // which are in the "idle" state
  getConnectedIdleCount() {
    return new Promise((resolve,reject)=>{
      this.getAllConnectedStatus().then((x)=>{
        let idle = 0;
        for(let y of x) {
          if( y === 'idle' ) {
            idle++;
          }
        }
        resolve(idle);
      });
    });
  }

  // a promise which resolves with a number of connected status
  // which are in the "idle" state
  getConnectedIdleIds() {
    return new Promise((resolve,reject)=>{
      this.getAllConnectedStatusAndId().then((x)=>{
        let res = [];
        for(let y of x) {
          // console.log(y);
          if( y.status === 'idle' ) {
            res.push(y.id);
          }
        }
        // console.log("getConnectedIdleIds " + res);
        resolve(res);
      });
    });
  }

  // uses a promise to reach out to all connected users and return the number that are idle
  getAllConnectedStatus() {
    if( this.getConnectedPeerCount() == 0 ) {
      return Promise.resolve([]); // optimisze for empty case
    } else {

      let parallelResolve = [];

      for(let hive of this.peers) {
        let control = hive.control; //hive.rpc.getStub("clientControl");
        // control.getStatus().then((x)=>{console.log(x)});
        parallelResolve.push(control.getStatus());
      }

      return Promise.all(parallelResolve);
    }
    // return this.peers.length;
  } // getAllConnectedStatus

  // similar to getAllConnectedStatus but 
  // return type changes a bit if returnIds is set, because it now returns a pair of pairs
  getAllConnectedStatusAndId() {
    if( this.getConnectedPeerCount() == 0 ) {
      return Promise.resolve([]); // optimisze for empty case
    } else {

      let parallelResolve = [];
      let copyIds = [];

      // for every connected peer, copy out parallel ids and a promise which will resolve
      // to their status
      for(let hive of this.peers) {
        let control = hive.control; //hive.rpc.getStub("clientControl");
        // control.getStatus().then((x)=>{console.log(x)});
        parallelResolve.push(control.getStatus());
        copyIds.push(hive.id);
      }

      // promises.all() will give us parallel access to all "status" over the promise RPC
      // however when these promises come back we need to know the id of the radio that goes with that promise
      // so we make ANOTHER promise, and then copy the id's over

      let p1 = new Promise((resolve,reject) => {
        Promise.all(parallelResolve).then((allStatus)=>{
          let ret = [];
          // Returned values will be in order of the Promises passed
          for(let i = 0; i < allStatus.length; i++) {
            ret.push({id:copyIds[i],status:allStatus[i]});
          }
          resolve(ret);
        })
      });

      return p1;
    }
    // return this.peers.length;
  }

  setupRpc(id) {
    let hive = this.getPeer(id);

    let rpcArgs = {
      sendMessage: (m)=>{
        if( this.printRpc ) {
          console.log('sending ' + JSON.stringify(m));
        }
        hive.socket.write(bufferSplit.prepare(JSON.stringify(m)))
      }
    };

    if(!this.printRpc) {
      rpcArgs.logger = {info:undefined};
    }

    hive.rpc = new Rpc.Rpc(rpcArgs);
    hive.rpc.registerImpl("serverControl", new ServerControl(this, id));
    hive.control = hive.rpc.getStub("clientControl");
    hive.rpcBuffer = new bufferSplit.NullBufferSplit( (as_str)=>{
      try {
        if( this.printRpc ) {
        }
        let o = JSON.parse(as_str);
        hive.rpc.receiveMessage(o);
      } catch (e) {
        console.log("that one was bad json: " + as_str);
      }
    }
    );
  }

  checkShutdown() {
    console.log("checkShutdown()");
    if( this.currentLaunch === null ) {
      return;
    }
    // console.log("checkShutdown(1)");

    // console.log(this.currentLaunch);

    let stopped = 0;

    for( let smodem of this.currentLaunch[1] ) {
        // console.log("checkShutdown(2)");
      // let smodem = this.currentLaunch[1];
      // console.log(smodem);
      if( smodem.state.requested === 'stop') {
        stopped++;
      }
    }

    if( stopped == this.currentLaunch[1].length ) {
      console.log("all smodem were asked to stop, stopping LauncherServer");
      clearInterval(this.talkPeersInterval);
      clearInterval(this.listPeersInterval);

      for(let hive of this.peers) {
        let sock = hive.socket;
        sock.destroy();
        // sock.destroy((e)=>{console.log('destroy socket ' + e);});
      }

    }
  }

  // not an interface like a typescript interface, but for usage by startAll()
  // the idea here is that the user can do stuff like
  // s0.start()
  // s1.transmit()
  // s0.dataMode()
  getSmodemControlInterface(id) {
    let hive = this.getHive(id);
    let control = hive.control;

    if( this.remoteLaunchDir === null ) {
      throw(new Error("Can't proceed becaues remoteLaunchDir is not set"));
    }

    let theyThrew = (rej) => {
      console.log("Error remote side threw " + rej);
      console.log(rej);
    }

    let _setPorts = (settings, _id) => {
      // copy settings.  Object.assign will NOT do a deep clone
      const copy = _.cloneDeep(settings);

      // grab port number patch
      const patch = portNumbers(_id);

      // console.log('_setPorts ' + id + ' ' + _id);

      // assign.
      // we have to assign the deep path
      // if we do not to this. the entire net:{} will be blown away by the patch
      Object.assign(copy.net.higgs_udp, patch.net.higgs_udp);
      Object.assign(copy.net.zmq, patch.net.zmq);
      console.log(patch.net.higgs_udp.rx_cmd);

      return copy;
    }

    let calls = {
      start: () => {calls.state.requested = "start"; control.launchSmodem(this.remoteLaunchDir).catch(theyThrew)}
      ,startConfigPath: (x,y) => {calls.state.requested = "start"; control.launchSmodemConfigPath(this.remoteLaunchDir,x,y).catch(theyThrew)}
      ,startOptions: (x) => {calls.state.requested = "start"; control.launchSmodemOptions(this.remoteLaunchDir,x).catch(theyThrew);}
      ,stop: () => {calls.state.requested = "stop"; control.kill().catch(theyThrew); this.checkShutdown()}
      ,stopAfter: (x) => { setTimeout(calls.stop,x) }
      ,getName: () => "Smodem network id: " + id
      ,state: {requested:"idle"}
      ,id: id
      ,getPorts: (set)=>_setPorts(set, id)
    };
    return calls;
  }

  // the idea here is that the user can do
  // group.stopAll

  getSmodemGroupControlInterface(allIds, smodems) {
    let calls = {
      stopAll: ()=>{
        for( let m in smodems ) {
          m.stop();
        }
      }
    }
    return calls;
  }

  // fires after setupRpc and others
  // the idea is that the user calls waitFor(2) and then this function
  // fires every time a client connets.  When there are enough clients connected that are also IDLE
  // we resolve the waitFor() promise.  The trick is that calling over RPC to the other side requires
  // a promise to do itself
  tickleNotifyWhenEnoughClientsIdle() {
    if( this.waitingForPromise !== null && this.waitingForCount !== null ) {
      // console.log('launching check');
      this.getConnectedIdleIds().then((activeAndIdle)=>{
        if( this.waitingForPromise === null || this.waitingForCount === null ) {
          return; // when we launched the promise this check was ok, but when we resolved it failed
                  // this happens when multiple promises launch when these are not null, and then as they all resolve the threshold
                  // is passed before all are resolved, leaving the late comers invalid
        }
        console.log('idle count result: ' + activeAndIdle + " waiting for " + this.waitingForCount );
        if( activeAndIdle.length >= this.waitingForCount ) {
          // trim so we only have the number of ids we want
          let usingIds = activeAndIdle.slice(0,this.waitingForCount);
          console.log('idle count selected: ' + usingIds );

          // at this point we are launching main simulation because all runners are connefted


          // build the second argument
          let smodems = [];

          for(let id of usingIds) {
            smodems.push(this.getSmodemControlInterface(id));
          }

          // build the first argument
          let group = this.getSmodemGroupControlInterface(usingIds, smodems);

          // this is a funny way to resolve a promise from "outside", see waitForIdle()
          this.waitingForPromise.resolve([group,smodems]);

          this.waitingForPromise = null; // reset for next call of waitForIdle
          this.waitingForCount = null;   // same

          this.currentLaunch = [group,smodems];
        }
      });
    }
  }

  clientConnectComplete(uniqueId) {
    this.tickleNotifyWhenEnoughClientsIdle();
  }

  removePeer(id) {
    let i = 0;
    let doSplice = false;
    for(let p of this.peers) {
      if(p.id === id) {
        doSplice = true;
        break;
      }
      i++;
    }

    if( doSplice ) {
      this.peers.splice(i,1);
    }
  }

  listPeers() {
    if(this.peers.length !== 0) {
      // console.log(JSON.stringify(this.peers));
      if( this.printStatusLoop ) {
        console.log("LaunchServer is in contact with " + this.peers.length + " peers ");
      }

    }
  }



  talkToPeers() {
    for(let hive of this.peers) {
      let control = hive.control;//hive.rpc.getStub("clientControl");

      if( true ) {
        if( typeof hive.state.pingCount === 'undefined') {
          hive.state.pingCount = 0;
        }

        if( this.debugLaunchKill ) {
          if( hive.state.pingCount == 4 ) {
            control.launchSmodem('/home/x/work/higgs_sdr_rev2/libs/s-modem/soapy/build');
          }

          if( hive.state.pingCount == 10 ) {
            control.kill();
          }
        }


  //         p.socket.write('hello we have designated you as peer ' + p.id + ' and this is your ' + p.state.pingCount + 'th ping message');

        hive.state.pingCount++;
      }

      // if( typeof p.state.rpc === 'undefined') {
      //   let rpc = new Rpc.Rpc({sendMessage: yourSendMessageFunction});
      //   rpc.registerImpl("calc", new Calc());
      // }

      if( this.printStatusLoop ) {

        control.getStatus().then((x)=>{console.log(x)});
      }

    } // for this.peers
  } // talkToPeers

  createAndOpenSocket() {

    this.server = net.createServer((c) => {
      // 'connection' listener
      if( this.printConnections ) {
        console.log('client connected');
      }

      let uniqueId = this.addPeer(c);

      this.setupRpc(uniqueId);

      c.on('end', () => {
        if( this.printConnections ) {
          console.log('client ' + uniqueId + ' disconnected');
        }
        this.removePeer(uniqueId);
      });

      c.on('data', (data) => {
        let hive = this.getPeer(uniqueId);
        if( this.printRpc ) {
          console.log('client ' + uniqueId + ' data:');
          console.log(data.toString());
        }
        hive.rpcBuffer.addData(data);
        // client.end();
      });

      this.clientConnectComplete(uniqueId);

      // c.write('hello\r\n');
      // c.pipe(c);
    });
    this.server.on('error', (err) => {
      throw err;
    });

    this.server.listen(this.launcherPort, '127.0.0.1');
  }

} // class 


// let instance = new LauncherServer({debugLaunchKill:true});

module.exports = {
    LauncherServer
  , portNumbers
}
