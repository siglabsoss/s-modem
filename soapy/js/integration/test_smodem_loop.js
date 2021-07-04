const higgs = require('../mock/mockhiggs.js');
const menv = require('../mock/mockenvironment.js');
const hc = require('../mock/hconst.js');
const launcherServer = require('../mock/launcherserver.js');
const fs = require('fs');


const radios = [];


let smodemLauncher;

function setupLauncher() {

  smodemLauncher = new launcherServer.LauncherServer({printStatusLoop:false, printRpc:false});
  // smodemLauncher = new launcherServer.LauncherServer({debugLaunchKill:true});

  // smodem.waitFor(1).then(()=>{console.log('\n\n\n\nsmodem up \n\n\n')});
}

setupLauncher();

function pickPorts(idx) {
  let r0 = {};
  let i2 = idx*2;
  r0.RX_CMD_PORT = (10001+i2);
  r0.TX_CMD_PORT = (20000+i2);
  r0.TX_DATA_PORT = (30000+i2);
  r0.RX_DATA_PORT = (40001+i2);



  // return r0;
}


function startAll(all, smodems) {

  [s0] = smodems;

  // console.log("got ");
  // console.log(all);
  // console.log(smodems);



  // console.log(s0);
  // console.log(s1);
  // console.log(s2);
  


  // samples per second, limited at env
  // let globalRateLimit = 1909760 * 0.5;
  // let globalRateLimit = 31.25E6 / 24.4;
  // let globalRateLimit = (1024+256) * (1000);
  // let globalRateLimit = (1024+256) * (2200);
  let globalRateLimit = 0;

  menv.reset({rateLimit:globalRateLimit}); // required to call reset




  const r0 = {log:{}};
  r0.RX_CMD_PORT = (10001);
  r0.TX_CMD_PORT = (20000);
  r0.TX_DATA_PORT = (30000);
  r0.RX_DATA_PORT = (40001);
  r0.SMODEM_ADDR = '127.0.0.1';

  r0.log.rbsock      = false; // lock ringbus socket packets
  r0.log.dsock       = false; // log data socket packets
  r0.log.rb          = true; // turn on
  r0.log.rbUnhandled = true; // log unhandled messages
  r0.log.rbNoisy     = false; // if log rb is on, do we log noisy ones?
  r0.log.tx_speed    = false; // log in OFDM frames per second
  r0.log.successful_mapmov_frame = false; // log when s-modem sends us a mapmov frame
  r0.log.rpc      = false;

  r0.disableTxProcessing = false;
  r0.disableRxProcessing = false;

  r0.useNativeTxChain = true; // only valid if enabled (see disableTxProcessing)
  r0.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

  r0.streamHexFromFilename = undefined;
  // r0.streamHexFromFilename = 'test/data/long_cs21_out.hex';

  r0.verifyMapMovDebug = true;

  r0.rateLimit = hc.SCHEDULE_FRAMES_PER_SECOND;

  let h0 = new higgs.Higgs();

  h0.start(r0, menv);

  radios.push({h:h0,options:r0});


  // above is building the Mock Higgs (we normally start higgs hardware first)

  // now launch and connect all smodems (we normally start ./modem_main second)
  // s0.start();

  if( true ) {
    var options = JSON.parse(fs.readFileSync('../config/rx.json', 'ascii'));
    // obj.net.higgs_udp.tx_cmd++;

    options.exp.debug_loopback_demod_data = true;
    options.exp.DSP_WAIT_FOR_PEERS = 0;
    options.exp.dashtie_into_mock = true;
    options.exp.aggregate_attached_rb = true;
    options.exp.skip_check_fb_datarate = true;

    s0.startOptions(options);
    // console.log(obj);
    
  }

  let testStart = Date.now();

  h0.notify.on('verify.mapmov.pass', ()=>{
    let testEnd = Date.now();
    let testDelta = testEnd - testStart;

    console.log("test took " + testDelta);
    s0.stop();
    h0.stop();
    process.exit(0);
  });

  h0.dash.notifyOn(2000000, 'fsm.state', (x)=>{
    console.log("main fsm got to " + x);
  });

  // let o = [];
  // let radio = 0;
  // o[0]={u:2};
  // o[1]={'v1':hc.REQUEST_FINE_SYNC_EV,'v2':radio};
  // h0.sendToHiggsRpc(o);
  // console.log("required fine sync");

  h0.dash.notifyOn(0, 'fsm.state', (x)=>{
    console.log("r0 fsm got to " + x);
    if( x == 69 ) {
      let o = [];
      o[0]={u:1000};
      o[1]={'k':['runtime','r0', 'skip_tdma'],'t':'bool',v:true};
      h0.sendToHiggsRpc(o);
      console.log("required TDMA skip");
    }

  });

  h0.dash.notifyOnError((code, str)=>{
    console.log("Failed to parse " + str);
    process.exit(1);
  });





  // s0.stopAfter(1000);

}

if( false ) {
  setTimeout(()=>{
    smodemLauncher.getConnectedIdleCount().then((x)=>{
        console.log('idle count result: ' + x);
      });
  }, 2000); // timeout
} // if

smodemLauncher.waitForIdle(1).then((x)=>{startAll(...x)});

