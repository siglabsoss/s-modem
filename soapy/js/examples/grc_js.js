const listen_grc = require('../../productionjs/jsinc/listen_grc_port.js');

let mycb = (d) => {
  console.log("Data: " + d.toString());
}

smodemLauncher = new listen_grc.ListenGrcPort({printConnections:false, cb:mycb});