'use strict'
const fs = require('fs');

const schedule = require('./schedule.js');

const serialport = require("serialport");

const readline = require('readline');

const color = require("cli-color");


class Grav3Uart {
  constructor(path, options={}) {
    this.name = 'Grav3Uart';
    this.path = path;
    this.baud = 115200;

    this.status = {};
    this.pending = {};
    this.got = 0;
    this.num_printed = 0;

    this.search = [
 ['V2V5          = ','V2V5',   2.5, 0.1]
,['V1V8          = ','V1V8',   1.8, 0.1]
,['V3V8          = ','V3V8',   3.8, 0.3]
,['V5V5          = ','V5V5',   5.5, 0.8]
,['V5V5N         = ','V5V5N', -5.5, 0.8]
,['V29           = ','V29',    29, 1.0]
,['E_DAC_ALARM   = ','DAC',    0, 1.0]
];

  this.rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
  });

    this.setup();
    this.setupCheck();
  }

  setupCheck() {
    this.lastGot = 0;
    this.handle = setInterval(this.checkOk.bind(this), 500);
  }

  checkOk() {

    readline.clearLine(process.stdout, 0);
    readline.cursorTo(process.stdout, 0, null);

    if( this.got <= this.lastGot ) {

      console.log('NO UPDATES');
      process.stdout.write('');
    } else {

      let st = this.statusAsText();
      process.stdout.write(st);

      this.num_printed++;

      this.lastGot = this.got;
    }

    // console.log('hey');

    

    // readline.moveCursor(this.rl, 10, 4)
    // console.log('hi');
  }

  getSpinner() {
    let spin = ['/', '-', '\\', '|'];
    let i = this.num_printed % spin.length;
    return spin[i];
  }

  statusAsText() {
    let r = '';

    let bad = [];

    for(const [p,key,target,tol] of this.search) {

      let voltage;
      if( key === 'DAC') {
        voltage = this.status[key];
      } else {
        voltage = this.status[key] / 1000;
      }
      let nicename = (''+target).padStart(4, ' ');

      // r += '\n';

      let upper = target + tol;
      let lower = target - tol;

      if( voltage <= lower || voltage >= upper) {
        bad.push(nicename);
      }

      r += '   ' + nicename + ': ' + voltage;
    }

    r += '  ' + this.getSpinner();

    if( bad.length != 0 ) {
      r += '  ' + color.bgWhite.black('!!!!!!!!! ' + bad.length + ' V OUT OF TOL:') + ' ' + color.bgRed.black(bad.join(','));
    }

    return r;
  }

  setup() {
    try {
      // console.log("+++++++++++++++++++++++++++++++++++++++");
      let stats = fs.statSync(this.path);
      // console.log(stats);
      // console.log("+++++++++++++++++++++++++++++++++++++++");

      this.setupSerial();

    } catch(e) {
      console.log("Could not find serial path " + this.path);
      throw e;
    }
  }

  setupSerial() {
    this.port = new serialport(this.path, {
      baudRate: this.baud,
      parser: new serialport.parsers.Readline('\n')
    });

    this.port.on('open', this.opened.bind(this));
    this.port.on('data', (x)=> {
      let d = x.toString();
      let lines = d.split('\r');
      for( let y of lines ) {
        this.gotLine(y.trim());
      }
      // this.gotLine(x);
    });

  }



  // serial event
  opened() {
      console.log(this.path + ' opened');
  }



  // serial event
  gotLine(d) {
    // console.log(d);
    // return
    // const d = data.toString();
    // console.log(d.indexOf('telemetry dump'));
    if( d.indexOf('telemetry dump') !== -1 ) {
      this.got++;
      this.status = this.pending;
      this.pending = {};

      if(this.got > 2) {
        // console.log(this.got-1);
        // console.log(this.status);
      }
      // console.log(this.got);
      // console.log(data.toString());
      return;
    }



    for(const [p,key,...rest] of this.search) {
      const found = d.indexOf(p);
      if( found !== -1 ) {
        const value = parseInt(d.slice(p.length));
        this.pending[key] = value;
        // console.log(key);
        // console.log(value);
      }
    }
    // console.log(d.length);
    // console.log(d);
  }


}

module.exports = {
    Grav3Uart
}
