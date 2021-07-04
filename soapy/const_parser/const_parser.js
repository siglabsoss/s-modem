const fs = require('fs');
const constMap = new Map();
const redef = /(?:^)#define *(\b[^ ]*\b) *?.(\b\d?[0-9xX](?!,)[0-9A-Fa-f+.*/<]*\b).*/;
const restring = /(?:^)#define *(\b[^ ]*\b) *(".*")/;
const rering = /(?:^)#define *(\b[^ ]*\b) *(\(?.*)/;

const reject = [
'SUGGESTED_FEEDBACKBUS_FLUSH_TAIL'
,'FRAME_PRIMITIVE'
,'FEEDBACK_VECTOR_ZERO_LIST'
,'FEEDBACK_STREAM_ZERO_LIST'
];



function stripComments(s) {
    const found = s.indexOf('//');
    if( found === -1 ) {
        return s;
    }

    return s.slice(0,found);
}

function parseConstFile(filename, reMap, map) {
    const file = fs.readFileSync(filename, 'utf8');
    const lines = file.split('\n');

    for (let lineRaw of lines) {
        let line = stripComments(lineRaw);
        let match = reMap.exec(line);
        if (match) {
            if (match[2] !== '') {
                map.set(match[1], match[2]);
            }
        }
    }
}

function writeConstFile(filepath, map, save=true) {

    for( let r of reject ) {
        map.delete(r);
    }

    let data = "";
    let exportData = "\nmodule.exports = {\n";
    for (let [key, value] of map) {
        data += "const " + key + " = " + value + ";\n";
        exportData += key + ",\n";
    }
    exportData = exportData.slice(0, -2);
    exportData += "};";

    if( save ) {
        fs.writeFile(filepath, data, (err) => {  
            if (err) throw err;

            fs.appendFile(filepath, exportData, (err) => {  
                if (err) throw err;
            });
        });
    } else {
        console.log(data);
        console.log(exportData)
    }
}

function fixedAdditions(map) {
    map.set('OUR_RING_ENUM', '0');
}

const higgs_root = '../../../..';
const libs_root = higgs_root + '/libs';
const soapy_root = libs_root + '/s-modem/soapy'

let work = [
 [libs_root + '/riscv-baseband/c/inc/ringbus.h', redef]
,[libs_root + '/riscv-baseband/c/inc/ringbus2_pre.h', rering]
,[libs_root + '/riscv-baseband/c/inc/ringbus2_post.h', rering]
,[libs_root + '/riscv-baseband/c/inc/ringbus3.h', rering]
,[soapy_root + '/src/common/constants.hpp', redef]
,[soapy_root + '/src/common/constants.hpp', restring]
,[soapy_root + '/src/driver/CustomEventTypes.hpp', redef]
,[libs_root + '/riscv-baseband/c/inc/schedule.h', rering]
,[libs_root + '/riscv-baseband/c/inc/feedback_bus.h', rering]
,[libs_root + '/riscv-baseband/c/inc/feedback_bus_types.h', rering]
,[soapy_root + '/src/driver/EventDspFsmStates.hpp', redef]
]

fixedAdditions(constMap);

for(let [path, re] of work) {
  parseConstFile(path, re, constMap);
}

const write_path = soapy_root + '/js/mock/hconst.js';

writeConstFile( write_path, constMap);
