const PCAPNGParser = require('pcap-ng-parser')
const pcapNgParser = new PCAPNGParser()
const myFileStream = require('fs').createReadStream(
  '/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/js/dump46.pcapng');

myFileStream.pipe(pcapNgParser)
    .on('data', parsedPacket => {
        console.log(parsedPacket)
    })
    .on('interface', interfaceInfo => {
        console.log(interfaceInfo)
    })

