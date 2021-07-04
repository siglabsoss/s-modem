const higgs = require('./mockhiggs.js');
const menv = require('./mockenvironment.js');
const hc = require('./hconst.js');

const radios = [];


function startAll() {

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

  r0.log.rbsock   = false; // lock ringbus socket packets
  r0.log.dsock    = false; // log data socket packets
  r0.log.rb       = true;
  r0.log.tx_speed = true; // log in OFDM frames per second
  r0.log.successful_mapmov_frame = false; // log in OFDM frames per second

  r0.disableTxProcessing = false;
  r0.disableRxProcessing = true;

  r0.useNativeTxChain = true; // only valid if enabled (see disableTxProcessing)
  r0.useNativeRxChain = false; // only valid if enabled (see disableTxProcessing)

  r0.streamHexFromFilename = undefined;
  // r0.streamHexFromFilename = 'test/data/long_cs21_out.hex';

  r0.rateLimit = hc.SCHEDULE_FRAMES_PER_SECOND;

  let h0 = new higgs.Higgs();

  h0.start(r0, menv);

  radios.push({h:h0,options:r0});

}


startAll();

