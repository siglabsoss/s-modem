const sendFile = require('./jsinc/send_udp.js');
const hc = require('../js/mock/hconst.js');
const mutil = require('../js/mock/mockutils.js');


let cbb = (d) => {
  console.log('cb');
}

let ip = '10.2.4.2';
// ip = '127.0.0.1';

let port = 10040;

let delay = 5000;

let lu = new sendFile.SendUDP( {launcherPort:port, printConnections:true, cb:cbb, ip:ip, delay:delay});

lu.count = 400;