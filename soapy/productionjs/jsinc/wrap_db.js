'use strict';

const low = require('lowdb');
const FileAsync = require('lowdb/adapters/FileSync');
const RecursiveIterator = require('recursive-iterator');

const dbPath = 'db/db.json';

class WrapDb {
  constructor(sjs) {
    this.sjs = sjs;
    this.sjs.dbHandle = this;

    this.setupDb();
    this.sjs.db = this.db;
  }

  setupDb() {
    this.adapter = new FileAsync(
      dbPath,
      {
        serialize:this.dbSerialize.bind(this),
        deserialize:this.dbDeserialize.bind(this)
      });
    this.db = low(this.adapter);


    this.db.defaults({ history:{dsp:{sfo:{peers:{t0:0}},cfo:{peers:{t0:0}}}}, timeAlive: 0 }).write();
  }

  gotWrite(x) {
    // console.log('ser: ');
    // console.log(JSON.stringify(x));
    let iterator = new RecursiveIterator(
      x /*{Object|Array}*/,
      0 /*{Number}*/,
      true /*{Boolean}*/,
      100 /*{Number}*/
    );

    for(let {node, path} of iterator) {
      if(typeof node !== 'object') {
        let settingsPath = 'db.' + path.join('.');
        if(typeof node === 'string') {
          // console.log("string path " + settingsPath + " " + node);
          this.sjs.settings.setString(settingsPath, node);
        }
        if(typeof node === 'number') {
          // console.log("number path " + settingsPath + " " + node);
          this.sjs.settings.setDouble(settingsPath, node);
        }
        if(typeof node === 'boolean') {
          this.sjs.settings.setBool(settingsPath, node);
        }
      }
    }
  }

  dbSerialize(x) {
    setImmediate(this.gotWrite.bind(this),x);
    return JSON.stringify(x);
  }
  dbDeserialize(x) {
    // console.log('dser: ' + x);
    return JSON.parse(x);
  }


}


module.exports = {
    WrapDb
}