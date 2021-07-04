
'use strict';

const dot = require('dot-object');
const EventEmitter = require('events');
 

class ParseDashTieEmitter extends EventEmitter {}



class ParseDashTie {
    constructor() {
        this.uuid = {};

        this.notify = new ParseDashTieEmitter();
        // myEmitter.on('event', () => {
        //   console.log('an event occurred!');
        // });
        // myEmitter.emit('event');
    }

    eventString(uuid,path) {
      return `${uuid}.${path}`;
    }

    addData(asStr) {
      // console.log("added " + asStr);

      let o;

      try {
        o = JSON.parse(asStr);
      } catch (e) {
        console.log("that one was bad json: " + asStr);
        this.notify.emit('error1', 1, asStr);
        return;
      }

      let uuid;
      if(o && o.length == 2 && 'u' in o[0]) {
        uuid = o[0].u;
      } else {
        console.log("invalid u or something: " + asStr);
        this.notify.emit('error1', 2, asStr);
        return;
      }

      let dotPath;
      let valueOfSet;
      if( '$set' in o[1] ) {
        let p = o[1]['$set'];
        // hacked way to get first key of object
        for( x in p ) {
          dotPath = x;
          break;
        }
        valueOfSet = p[dotPath];
      } else {
        console.log("something wrong with $set: " + asStr);
        this.notify.emit('error1', 3, asStr);
        return;
      }

      if( typeof dotPath === 'undefined' ) {
        console.log("something wrong below $set: " + asStr);
        this.notify.emit('error', 4, asStr);
        return;
      }

      this.applyDot(uuid, dotPath, valueOfSet);
    }

    applyDot(uuid, dotPath, valueOfSet) {
      // console.log("Will set " + dotPath + " to " + valueOfSet + " in uuid " + uuid);

      if( typeof this.uuid[uuid] === 'undefined' ) {
        this.uuid[uuid] = {};
      }

      var tgt = this.uuid[uuid];
      dot.str(dotPath, valueOfSet, tgt);

      // notify listners afterwards
      this.notify.emit(this.eventString(uuid, dotPath), valueOfSet);

      // console.log(tgt);
    }

    notifyOn(uuid, dotPath, cb) {
      this.notify.on(this.eventString(uuid, dotPath),cb);
    }

    notifyOnError(cb) {
      this.notify.on('error1',cb);
    }
}




module.exports = {
    ParseDashTie
}
