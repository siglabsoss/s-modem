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


it('Smodem can slowdown', function(done) {
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
        r0.disableRxProcessing = false;

        r0.useNativeTxChain = true; // only valid if enabled (see disableTxProcessing)
        r0.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

        r0.verifyMapMovDebug = true;

        r0.log.rpc = false;
        r0.log.tx_speed = true;

        let h0 = new higgs.Higgs();

        h0.start(r0, menv);

        if( true ) {
          var options = JSON.parse(fs.readFileSync('../config/rx.json', 'ascii'));
          // options.net.higgs_udp.tx_cmd++;
          options.exp.debug_tx_fill_level = true;
          options.exp.debug_loopback_fine_sync_counter = true;

          options.exp.DSP_WAIT_FOR_PEERS = 0;
          options.exp.skip_check_fb_datarate = true;
          options.exp.aggregate_attached_rb = true;
          options.exp.dashtie_into_mock = true;

          s0.startOptions(options);
          // console.log(obj);
        }

        let testStart = Date.now();

        let lastFrame = Date.now();
        let lastFrameCount = 0;

        let frameCounter = (frameCount) => {
          let thisFrame = Date.now();
          let delta = frameCount - lastFrameCount;
          let deltaT = thisFrame - lastFrame;

          // console.log(`${delta} ${deltaT}`);
          console.log("frame: " + frameCount + " (" + 1000*delta/deltaT + ")");
          lastFrame = thisFrame;
          lastFrameCount = frameCount;
        };

        h0.dash.notifyOn(100, 'fb.frame_counter', frameCounter);

        h0.notify.on('verify.mapmov.pass', () => {
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
        });

        

        // setTimeout(()=>{
          
        // },20000);
      }

      // smodemLauncher.selfLaunch();

      smodemLauncher.waitForIdle(1).then((x)=>{startAll(...x)});

     }catch(e){reject(e)}
    }); // promise
  }); // forall

  const props = {size: 10000000000, tests: 1};
  jsc.check(t, props).then( r => r === true ? done() : done(new Error(JSON.stringify(r))));

}); // it

}); // describe