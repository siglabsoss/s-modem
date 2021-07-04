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
//   console.log("Post test sleep");
//   let start = Date.now();
//   let now;
//   let sleepFor = 1000;
//   do {
//     now = Date.now();
//   } while( now - start < sleepFor );
//   console.log("Post test sleep done");
// });


it('Smodem can start and stop', function(done) {
  this.timeout(30000);

  let t = jsc.forall(jsc.constant(0), function () {
    console.log("in jsc: Smodem can start and stop");


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

        r0.verifyMapMovDebug = true;

        let h0 = new higgs.Higgs();

        h0.start(r0, menv);

        if( true ) {
          var obj = JSON.parse(fs.readFileSync('../config/tx.json', 'ascii'));
          // obj.net.higgs_udp.tx_cmd++;
          obj.exp.debug_tx_fill_level = true;
          s0.startOptions(obj);
          // console.log(obj);
        }

        let testStart = Date.now();

        let mapMovPass = () => {
          let testEnd = Date.now();
          let testDelta = testEnd - testStart;

          console.log("test took " + testDelta);
          // h0.notify.off('verify.mapmov.pass', mapMovPass);
          // h0.notify.removeAllListeners();


          if( true ) {


            console.log("stop() higgs");

            h0.stop().then(()=>{
              console.log("higgs stopped");
              s0.stop();
              resolve(true);
            });


          }

          if( false ) {


            h0.unpipeTx();

            setTimeout(()=>{
              console.log("stop() higgs");
              // all.stop();
              s0.stop();
              h0.stop2();

              // smodemLauncher.server.close();

              resolve(true);
            }, 500);

          }


          // if( false ) {
          //   h0.astop1();
          // }

          // clearInterval(h0.ethFillInterval);
          // clearInterval(h0.logTxSpeedInterval);

          // h0.rbsock.close();
          // h0.dsock.close();


          // setTimeout(()=>{
          //   console.log('');
          //   console.log('');
          //   console.log('');
          //   console.log(process._getActiveHandles());
          //   console.log('');
          //   console.log('');
          //   console.log('');
          //   console.log(process._getActiveRequests());
          //   console.log('');
          //   console.log('');
          // }, 1000);


          // setImmediate(()=>{

          // });
        }

        h0.notify.on('verify.mapmov.pass', mapMovPass);

        // setTimeout(()=>{
          
        // },20000);
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