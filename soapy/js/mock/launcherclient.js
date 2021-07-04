const net = require('net');
const bufferSplit = require('../mock/nullbuffersplit');
const fs = require('fs');
const crypto = require('crypto');

// const ICalc = require('../mock/ICalc.js');
const Rpc = require('grain-rpc/dist/lib');

const { spawn } = require('child_process');

const program = require('commander');



const printRpc = false;
const printConnections = false;

let stopWhenRemoteDisconnects = true;

let closeOnDisconnect = false;

let gotSuccessfulConnect = false;

// console.log(Rpc);

// class Calc {
//     add(x,y) {
//         return x + y;
//     }
// }

// this actually launches and kills s-modem subprocess
class ClientControl {
    constructor() {
      this.status = "idle";
      this.statusChecks = 0;
      this.smodemHandle = null;
    }

    getStatus() {
      // this.statusChecks++;
      // if(this.statusChecks == 3) {
      //   this.status = "launching";
      // }
      return this.status;
    }

    // seems like grain-rpc does not correctly send undefined over the wire for defaults args
    // we can just use null instead.  this may bite me in the future. maybe this is my fault due to json serialization
    launchSmodemOptions(wd, options) {
      const name = './modem_main';

      if( options === null || typeof options === 'undefined') {
        throw(new Error('launchSmodemOptions() requires options argument'));
      }

      // pick a unique input using a hash
      let hashInput = " " + Date.now() + " " + Math.random();

      let suffix = crypto.createHash('sha1').update(hashInput).digest('hex').slice(0,8);

      let tmppath = "/tmp/init_" + suffix + ".json";
      fs.writeFileSync(tmppath, JSON.stringify(options));

      return this.launchSmodemConfigPath(wd, tmppath);
    }

    // seems like grain-rpc does not correctly send undefined over the wire for defaults args
    // we can just use null instead.  this may bite me in the future. maybe this is my fault due to json serialization
    launchSmodemConfigPath(wd, path1=null, path2=null) {
      const name = './modem_main';

      console.log("launchSmodemConfigPath " + path1 + " " + path2);

      const defaults = {
        cwd: wd,
        env: process.env
      };

      const args = [];

      if( path1 !== null) {
        args.push('--config');
        args.push(path1);
      }

      if( path2 !== null) {
        args.push('--patch');
        args.push(path2);
      }


      this.smodemHandle = spawn(name, args, defaults);

      // find.stdout.pipe(wc.stdin);

      this.smodemHandle.stdout.on('data', (data) => {
        let asStr = data.toString('ascii');

        process.stdout.write(asStr); // without newline
        // console.log(`Number of files ${data}`);
      });

      this.status = 'running';
    }

    launchSmodem(wd) {
      return this.launchSmodemConfigPath(wd);
    }

    // returns ok
    kill() {
      if( this.smodemHandle !== null ) {
        this.smodemHandle.kill('SIGHUP');
        this.smodemHandle = null;
        this.status = "idle";
        console.log('Killed');
        console.log('');
        console.log('');
        console.log('');
        return true;
      }
      return false;
    }
}


let yourSendMessageFunction;

let rpc;

let rpcBuffer;


const LAUNCH_PORT = 10009;
const LAUNCH_IP = '127.0.0.1';
const RECONNECT_TIME = 250;

let clientControlInst = new ClientControl();

let client;

function connect() {

client = new net.Socket();

client.connect(LAUNCH_PORT, LAUNCH_IP, function() {
    if( printConnections ) {
      console.log('Connected');
    }
    gotSuccessfulConnect = true;
    // client.write('Hello, server! Love, Client.');

    yourSendMessageFunction = function(fn) {
      if( printRpc ) {
        console.log('sending ' + JSON.stringify(fn));
      }
      client.write(bufferSplit.prepare(JSON.stringify(fn)));
    }

    let rpcArgs = {
      sendMessage: yourSendMessageFunction
    };

    if(!printRpc) {
      rpcArgs.logger = {info:undefined};
    }

    rpc = new Rpc.Rpc(rpcArgs);
    // rpc.registerImpl("calc", new Calc());
    rpc.registerImpl("clientControl", clientControlInst);

    // let stub = rpc.getStub("calc");
    // stub.add(10,10).then((x)=>{console.log(x)});
    // console.log();

    gotFullString = function(as_str) {
      try {
        let o = JSON.parse(as_str);
        rpc.receiveMessage(o);
      } catch (e) {
        console.log("that one was bad json: " + as_str);
      }
    }

    rpcBuffer = new bufferSplit.NullBufferSplit(gotFullString);


    testServerLink = function () {
      let control = rpc.getStub("serverControl");
      // control.add(10,10).then((x)=>{console.log(x)});
      control.getId().then((x)=>{console.log("server assigned us id: " + x)});
    };

    setTimeout(testServerLink, 1500);

});

client.on('data', function(data) {
  if( printRpc ) {
    console.log('Received: ' + data);
  }
  rpcBuffer.addData(data);
});

client.on('close', function() {
  if( printConnections ) {
    console.log('Connection closed');
  }
  optionalStop();
  optionalTerminate();
  reconnect();
});

client.on('end', function() {
  
  if( printConnections ) {
    console.log('Connection end');
  }
  optionalStop();
  optionalTerminate();
  reconnect();
});

client.on('error', function(e) {
  if( printConnections ) {
    console.log('Connection error ' + e);
  }
});

} // function connect()

function optionalStop() {
  if( stopWhenRemoteDisconnects ) {
    clientControlInst.kill();
  }
}

function optionalTerminate() {
  if( gotSuccessfulConnect && closeOnDisconnect ) {
    clientControlInst.kill();
    process.exit(0);
  }
}

let intervalConnect = null;

function reconnect() {


  if( intervalConnect === null ) {
    if( printConnections ) {
      console.log("reconnecting in timeout...");
    }
    intervalConnect = setTimeout(() => {
        client.removeAllListeners(); // the important line that enables you to reopen a connection
        connect();
        intervalConnect = null;
    }, RECONNECT_TIME);
  } else {
    console.log("ignoring reconnect due to pending");
  }


}

console.log("Launcher Client Boot");


program
  .version('0.1.0')
  .option('-c, --close', 'Auto close when TCP connection terminates')
  .parse(process.argv);

if(program.close) {
  console.log('got auto close');
  closeOnDisconnect = true;
}


connect();





