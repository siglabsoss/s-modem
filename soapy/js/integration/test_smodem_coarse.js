const higgs = require('../mock/mockhiggs.js');
const menvPair = require('../mock/mockenvironmentpair.js');
const hc = require('../mock/hconst.js');
const launcherServer = require('../mock/launcherserver.js');
const fs = require('fs');
const assert = require('assert');
const jsc = require("jsverify");
const streamers = require('../mock/streamers.js');
const mutil = require('../mock/mockutils.js');

let smodemLauncher;

let doPlot = true;
let gp;

if( doPlot ) {
   gp = require('../mock/gplot.js');
}


let raw;
let rawf;
describe('Integration Tx', function () {

before(()=>{
  // console.log("running before");
  // raw = mutil.readHexFile('test/data/sc_16_144_880_1008_bpsk_mod.hex');
  // raw = mutil.readHexFile('/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/test_tx_7/cs10_out.hex');
  raw = mutil.readHexFile('test/data/cooked_cs10_out.hex');
  // console.log(process.cwd());
  console.log("raw length " + raw.length);

  let frames = Math.trunc(raw.length / (1024+256));
  // frames=5;
  raw = raw.slice(0,frames*(1024+256));

  raw = [].concat(raw,raw,raw,raw,raw,raw,raw,raw);

  rawf = mutil.IShortToIFloatMulti(raw);

  // gp.plot(rawf);

  let noiseGain = 0.0075/6;
  let halfNoiseGain = noiseGain/2;

  for(let i = 0; i < rawf.length; i++) {
    // console.log(rawf[i]);
    rawf[i] /= 4;
    rawf[i] += noiseGain*Math.random()-halfNoiseGain;
  }

  console.log("raw resized " + raw.length);

});

it('Mock Environment can coarse', function(done) {
  this.timeout(60000*30);


  let t = jsc.forall(jsc.constant(0), function () {
    console.log("in jsc: Smodem can start and stop");

    // let cplx = mutil.IShortToComplexMulti(raw);
    // let asreal0 = cplx.map((e,i)=>e.re);
    // gp.plot(asreal0);


    return new Promise(function(resolve, reject) { try {
      console.log("in Promise");
      smodemLauncher = new launcherServer.LauncherServer({printStatusLoop:false, printRpc:false});
      let shutdownTest;

      function startAll(all, smodems) {

        [s0] = smodems;

        menv = new menvPair.MockEnvironmentPair();

        const r0 = higgs.getDefaults();


        r0.disableTxProcessing = false;
        r0.disableRxProcessing = false;

        r0.useNativeTxChain = false; // only valid if enabled (see disableTxProcessing)
        r0.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

        r0.verifyMapMovDebug = true;

        r0.turnstileAppliesToRx = false; 

        r0.log.tx_speed = true;

        let h0 = new higgs.Higgs();


        let count = 0;
        let savedRxFrames = [];

        // h0.notify.on('streams.all.setup', ()=>{
        //   h0.rx.stream.tagger.onFrameIShort = (x)=>{
        //     // console.log(x);

        //     savedRxFrames = savedRxFrames.concat(x);

        //     if( count === 4 ) {
        //       mutil.writeHexFile('rx_frames.hex', savedRxFrames);
        //       setTimeout( shutdownTest, 0 );
        //     }
        //     count++;

        //   };
        // });

        h0.start(r0, menv);

        if( true ) {

          var inject = new streamers.StreamArrayType({},{type:'float64', chunk:1280*2, repeat:true});
          // let conv = new streamers.IShortToCF64({}, {});
          // conv.name = 'Inject convert';

          // conv.gain = 3E-5;

          inject.load(rawf);

          let dummy = new streamers.DummyStream();

          menv.attachRadio(inject, dummy);
        }

        let advanceInject = (adv) => {
          let zrs = [];
          for(let i = 0; i < adv*8*2; i++) {
            zrs.push(0);
          }
          inject.push(Buffer.from(zrs));
        }


        h0.notify.on('environment.attached', ()=>{
          // h0.rx.stream.turnstile.advance(16*(1024+256 - 4));

          let adv = jsc.random(0,2000);

          advanceInject(adv);
          // h0.rx.stream.coarse.print4 = true;
          // h0.rx.stream.coarse.print5 = true;

          // h0.rx.stream.coarse.triggerCoarseAt(300);

          // setTimeout(()=>{
          //   h0.rx.stream.coarse.triggerCoarse();
          // },1000);

        });

        h0.notify.on('cs10.sfoSignCallback', (adv, data)=>{
          console.log("Adjust " + adv + ", " + data);
          if( data === 4 ) {
            // advanceInject(adv);
            advanceInject(1280-adv);
          }
          // process.exit(0);
        });

        h0.notify.on('cs00.coarseResult', (x, cplx)=>{
          // if( y > 600 ) {
          //   y
          // }

          console.log("cs00 coarse " + x + " (" + x + ")" + " cplx arg " + cplx.arg());
          // process.exit(0);
        });



        // let handAdjust = 256*4;
        // setTimeout(()=>{
        //   console.log("hand adjust by " + handAdjust);
        //   h0.rx.stream.turnstile.advance(handAdjust*4);
        // }, 3000);



        // let j = 0;
        // for( let i = 0; i < 1280; i+= 100 ) {
        //    setTimeout(()=>{
        //     // if( i === 0 ) {
        //     //           h0.rx.stream.turnstile.advance(90);
        //     // }
        //       console.log("hand adjust by " + i);
        //       h0.rx.stream.turnstile.advance(10);
        //     }, 3000+7000*j);
        //    j++;
        // }




        if( true ) {
          var options = JSON.parse(fs.readFileSync('../config/rx.json', 'ascii'));
          // options.net.higgs_udp.tx_cmd++;
          options.exp.DSP_WAIT_FOR_PEERS = 0;
          options.exp.debug_tx_fill_level = false;
          options.exp.skip_check_fb_datarate = true;
          options.exp.debug_coarse_sync_on_rx = true;
          // options.exp.dashtie_into_mock = true;
          s0.startOptions(options);
          // console.log(obj);
        }

        let testStart = Date.now();

        shutdownTest = () => {
          let testEnd = Date.now();
          let testDelta = testEnd - testStart;

          console.log("test took " + testDelta);
          // h0.notify.off('verify.mapmov.pass', mapMovPass);
          // h0.notify.removeAllListeners();


          // if( true ) {
          //   console.log("stop() higgs");

          //   h0.stop().then(()=>{
          //     console.log("higgs stopped");
          //     s0.stop();
          //     resolve(true);
          //   });
          // }



        }

        // h0.notify.on('verify.mapmov.pass', mapMovPass);

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