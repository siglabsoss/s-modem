const higgs = require('../mock/mockhiggs.js');
const menv = require('../mock/mockenvironment.js');
const hc = require('../mock/hconst.js');
const launcherServer = require('../mock/launcherserver.js');
const fs = require('fs');
const assert = require('assert');
const jsc = require("jsverify");


let smodemLauncher;

describe('Integration Tx', function () {

// after(()=>{
//   process.exit(0);
// });


it('Smodem can tx mapmov', function(done) {
  this.timeout(30000);

  let t = jsc.forall(jsc.constant(0), function () {
    console.log("in jsc")

    return new Promise(function(resolve, reject) { try {
      console.log("in Promise");
      smodemLauncher = new launcherServer.LauncherServer({printStatusLoop:false, printRpc:false});

      function startAll(all, smodems) {


        [s0] = smodems;

        menv.reset(); // required to call reset


        const r0 = higgs.getDefaults();


        r0.disableTxProcessing = false;
        r0.disableRxProcessing = true;

        r0.useNativeTxChain = true; // only valid if enabled (see disableTxProcessing)
        r0.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

        r0.streamHexFromFilename = undefined;
        // r0.streamHexFromFilename = 'test/data/long_cs21_out.hex';

        r0.verifyMapMovDebug = true;

        let h0 = new higgs.Higgs();

        h0.start(r0, menv);


        // above is building the Mock Higgs (we normally start higgs hardware first)

        // now launch and connect all smodems (we normally start ./modem_main second)
        // s0.start();

        if( false ) {
          // launch as tx options from this file
          s0.startConfigPath('../config/tx.json');
        }

        if( true ) {
          var obj = JSON.parse(fs.readFileSync('../config/tx.json', 'ascii'));
          // obj.net.higgs_udp.tx_cmd++;
          obj.exp.debug_tx_fill_level = true;
          s0.startOptions(obj);
          // console.log(obj);
          
        }

        let testStart = Date.now();

        h0.notify.on('verify.mapmov.pass', ()=>{
          let testEnd = Date.now();
          let testDelta = testEnd - testStart;

          console.log("test took " + testDelta);
          // s0.stop();
          // h0.stop();
          // resolve(true);

          h0.stop().then(()=>{
            console.log("higgs stopped");
            s0.stop();
            resolve(true);
          });

          
        });

        // setTimeout(()=>{
        //   let o = [];
        //   o[0]={u:1000};
        //   o[1]={'k':['runtime','is_awake'],'t':'bool',v:true};
        //   h0.sendToHiggsRpc(o);
        //   console.log("send to higgs");
        //   // h0.sendToHiggsRpcRaw("ABCDE");
        //   // h0.sendToHiggsRpcRaw("A");
        //   // h0.sendToHiggsRpcRaw("A");
        //   },9000);


        // s0.stopAfter(1000);

      }

      smodemLauncher.selfLaunch();

      smodemLauncher.waitForIdle(1).then((x)=>{startAll(...x)});

     }catch(e){reject(e)}
    }); // promise
  }); // forall

  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it

}); // describe