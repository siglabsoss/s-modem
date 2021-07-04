const { spawn } = require('child_process');


function _sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function _printStdout(data) {
  let asStr = data.toString('ascii');

  process.stdout.write(asStr); // without newline
}

class SetupHardware {
    constructor(options={}, cb, isDebugMode) {
    // grab defaults from options
    const { staleInit = {}, svf = 'svf2higgs.sh', printRpc = false, printConnections = false, printStatusLoop = true, launcherPort = 10009, debugLaunchKill = false } = options;
  
    // load defaults into object
    this.printRpc = printRpc;
    this.printConnections = printConnections;
    this.launcherPort = launcherPort;
    this.debugLaunchKill = debugLaunchKill;
    this.printStatusLoop = printStatusLoop;
    this.set = staleInit;
    this.svfPath = this.set.hardware.rx.svf;


    if( this.set.exp && this.set.exp.our && this.set.exp.our.role ) {
      this.role = this.set.exp.our.role;
      if(this.role === 'tx' || this.role === 'rx') {
        console.log("SetupHardware setting up for " + this.role + " hardware role");
      } else {
        console.log('Don\'t understand exp.our.role value of ' + this.role);
        process.exit(1);
      }
    } else {
      console.log('Something is wrong with init.json or exp.our.role');
      process.exit(1);
    }

    this.networks = null;


    this.cb = cb;

    this.setDirs();

    // warn network card first, then decide if we need hardware setup
    this.warnNetworkCard().then( ()=>{
      if(this.set.hardware.bitfile_and_bootload && isDebugMode !== true) {
        this.allHardware();
      } else {
        this.cb();
      }
    });

  }

  setDirs() {
    // grab cwd from process global
    const thisDir = process.cwd();

    // regex will strip of production js folder to get back to higgs root
    const stripProd = /\/libs\/s-modem\/soapy\/productionjs/gi;

    // fire regex and assign to class
    this.HIGGS_ROOT = thisDir.replace(stripProd, '');
    this.SMODEM_REPO = this.HIGGS_ROOT + '/libs/s-modem';
    this.VERILATOR = this.HIGGS_ROOT + '/sim/verilator';
  }


  async allHardware() {
    await this.checkOrFlashBitfiles();
    await this.testEthX3();
    await _sleep(300);
    await this.checkOrBootload();


    // Resume modem_main
    console.log("All Hardware ok, resuming");
    setImmediate(this.cb);

  }

  async testEthX3 () {
    await this.testEth();
    await this.testEth();
    const ret0 = await this.testEth();

    if( ret0 !== 0 ) {
      console.log("test_eth failed after bitfiles");
      process.exit(1);
    }
    console.log("test_eth passed after bitfiles");
  }

  testEth() {
    return new Promise(function(resolve,reject) {
      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';
      const args = ['test_eth'];

      this.testEthHandle = spawn(name, args, defaults);

      this.testEthHandle.stdout.on('data', _printStdout);

      this.testEthHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  testRing() {
    return new Promise(function(resolve,reject) {
      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';
      const args = ['test_ring'];

      this.testEthHandle = spawn(name, args, defaults);

      this.testEthHandle.stdout.on('data', _printStdout);

      this.testEthHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  programBitfiles() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = './nodehelper0.js';
      const args = [this.svfPath];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  async checkOrFlashBitfiles() {
    console.log('checkOrFlashBitfiles()');

    let retx1 = await this.testRing();

    let ret0 = await this.testEth();
    await _sleep(300);
    let ret1 = await this.testEth();

    console.log(`tried two test_eth and got ${ret0},${ret1}`);

    if( ret0 !== ret1 ) {
      console.log('inconsitant test_eth results, try again');
      process.exit(1);
    }

    // at this point, just check ret0 because we've gotten consistent results

    if( ret0 === 0 ) {
      console.log('Bitfiles are in');
    } else {
      console.log('Bitfiles are not in');
      await this.programBitfiles();
      await _sleep(4000);
      await this.testRing();
      await this.testRing();
      await this.testRing();
      await _sleep(500);
      await this.testEth();
      await _sleep(300);
      let ret3 = await this.testEth();
      if( ret3 == 0 ) {
        console.log('testEth passed after bitfiles');
      } else {
        console.log('testEth returned ' + ret3 + ' after programming bitfiles');
        process.exit(1);
      }
    }
  }

  // automatically switches based on mode
  checkBootload() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';
      const args = [];

      if( this.role === 'rx' ) {
        args.push('test_bootload_rx');
      }
      if( this.role === 'tx' ) {
        args.push('test_bootload_tx');
      }

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  getBootloadDir() {
    if( this.role === 'rx' ) {
      return this.VERILATOR + '/' + this.set.hardware.rx.bootload.path;
    }
    if( this.role === 'tx' ) {
      return this.VERILATOR + '/' + this.set.hardware.tx.bootload.path;
    }
  }

  compileRiscv() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.getBootloadDir(),
        env: process.env
      };

      const name = 'make';

      const args = ['vall'];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  bootload() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.getBootloadDir(),
        env: process.env
      };

      const name = 'make';

      const args = ['bootload'];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }


  async postBootloadSetup() {
    if( this.role === 'rx' ) {
      if( this.set.hardware.rx.bootload.after.set_vga.enable ) {
        return await this.handleVga();
      }
    }
    if( this.role === 'tx' ) {
      if( this.set.hardware.tx.bootload.after.config_dac.enable ) {
        let resConfig = await this.handleConfigDac();
        if( resConfig !== 0 ) {
          console.log("Confic dac failed");
          return res;
        }
      }

      if( this.set.hardware.tx.bootload.after.tx_channel.enable ) {
        let resChannel = await this.handleTxChannel();
        if( resChannel !== 0 ) {
          console.log("TX Channel failed");
          return res;
        }
      }
    }

    return 0;
  }
  
  // only call if enabled, this fn does not check settings itself
  // only call if rx side
  handleVga() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';

      const args = ['set_vga', 'VGA_ATTENUATION='+this.set.hardware.rx.bootload.after.set_vga.attenuation];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  // only call if enabled, this fn does not check settings itself
  // only call if tx side
  handleConfigDac() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';

      const args = ['config_dac'];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }

  // only call if enabled, this fn does not check settings itself
  // only call if tx side
  handleTxChannel() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'make';

      const args = ['tx_channel', 'tx='+this.set.hardware.tx.bootload.after.tx_channel.channel];

      this.programHandle = spawn(name, args, defaults);

      this.programHandle.stdout.on('data', _printStdout);
      this.programHandle.stderr.on('data', _printStdout);

      this.programHandle.on('exit', code => {
        console.log(`Exit code is: ${code}`);
        resolve(code);
      });

    }.bind(this));
  }




  async checkOrBootload() {
    let bootloadIn = await this.checkBootload();

    if(bootloadIn === 0) {
      console.log("Bootload already done");
      return;
    }

    console.log("Bootload status:");
    console.log(bootloadIn);

    let compile = await this.compileRiscv();
    if(compile !== 0) {
      console.log("Compile failed");
      process.exit(1);
    }
    console.log("Compile ok");

    let bl = await this.bootload();

    if(bl !== 0) {
      console.log("Bootload failed");
      process.exit(1);
    }
    console.log("Bootload ok");

    let after = await this.postBootloadSetup();
    if( after !== 0) {
      console.log("After Bootload commands failed");
      process.exit(1);
    }

    return;
  }


  grabIpA() {
    return new Promise(function(resolve,reject) {
      let home = process.env.HOME;

      const defaults = {
        cwd: this.HIGGS_ROOT,
        env: process.env
      };

      const name = 'ip';
      const args = ['a'];

      const h = spawn(name, args, defaults);

      const cap = [];
      const capture =  (data) => {
        const asStr = data.toString('ascii');
        cap.push(asStr);
      };
      h.stdout.on('data', capture);

      h.on('exit', code => {
        const asStr = cap.join("");
        let networkIps = [];

        let r1 = /inet ([^\/ ]*)/g;

        let match;
        while(match = r1.exec(asStr)) {
          networkIps.push(match[1]);
        }

        resolve(networkIps);
      });

    }.bind(this));
  }

  async warnNetworkCard() {

    const enforceNicCheck = this.set.net.network.enforce_nic_ip;

    if(!enforceNicCheck) {
      console.log("warnNetworkCard() will not run");
      return;
    }


    let res = await this.grabIpA();
    this.networkIps = res;

    let found = false;

    const configured = this.set.net.network.pc_ip;

    let i = 0;
    for(let x of this.networkIps) {
      if( x === configured ) {
        found = true;
        console.log(`Found ${configured} after looking at ${i+1} network cards`);
        break;
      }
      i++;
    }

    if(!found) {
      console.log('-----------------------------------------');
      console.log('-----------------------------------------');
      console.log('-');
      console.log('-');
      console.log('- init.json:');
      console.log(`- 'net.network.pc_ip' (which is set to ${configured}) was not found on any network card`);

      console.log('-');
      console.log('- Looked at:');
      for(let x of this.networkIps) {
        console.log('-     ' + x);
      }
      console.log('-');
      console.log('-----------------------------------------');
      console.log('-----------------------------------------');
      process.exit(1);
    }
  }


}

module.exports = {
    SetupHardware
}
