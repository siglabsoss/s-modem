const mutil = require('../js/mock/mockutils.js');

// const bad = require('./capture/bad_packet_1551486882718.json');
// const bad = require('./capture/bad_packet_1551486881711.json');


// const bad = require('./capture/bad_packet_1551488672571.json');
// const bad = require('./capture/bad_packet_1551488673559.json');



const bad = require('./capture/bad_packet_1551488681114.json');
// const bad = require('./capture/bad_packet_1551488682122.json');

// /mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/capture/bad_packet_1551488681114.json
// /mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/capture/bad_packet_1551488682122.json



console.log(bad.settings);

let expectedWords = mutil.streamableToIShort(bad.expected);
let gotWords = mutil.streamableToIShort(bad.got);

console.log("-------------------------------");

for(let i = 0; i < expectedWords.length; i++) {
  if( expectedWords[i] != gotWords[i] ) {
    console.log(expectedWords[i].toString(16) + ' ' + gotWords[i].toString(16) );
  }
}

console.log("-------------------------------");

for(let i = 0; i < expectedWords.length; i++) {
  console.log(gotWords[i].toString(16));
}