'use strict';


class WorkKeeper {
  constructor(options={}) {

    const {tolerance=4,print=false,printFirstBlock=false} = options;

    Object.assign(this, {tolerance,print,printFirstBlock});

    this.peerCount = 0;
    this._peers = {};
    this.firstBlockFlag = true;
  }


  _addKey(id) {
    if( this.print ) {
      console.log('adding id ' + id);
    }
    this._peers[id] = {c:0,cb:null};
    this.peerCount++;
  }

  _callOthers() {
    let {max,maxid,min,minid} = this.maxWork();

    for(let id = 0; id < this.peerCount; id++) {
      let peer = this._peers[id];
      if( peer.cb !== null ) {
        let delta = peer.c - min;
        if( delta < this.tolerance ) {
          let copy = peer.cb;
          peer.cb = null;

          if(this.print) {
            console.log('id ' + id + ' ran delayed _callOthers');
          }

          copy();

        }
      }
    }
  }

  do(id, cb) {
    if( !this._peers.hasOwnProperty(id) ) {
      this._addKey(id);
    }

    let peer = this._peers[id];

    if( this.peerCount === 1 ) {
      peer.c++;
      if(this.print) {
        console.log('id ' + id + ' ran immediately');
      }
      cb();
      return;
    } else {
      peer.c++;
      const {max,maxid,min,minid} = this.maxWork();
      let delta = peer.c - min;
      if( delta < this.tolerance ) {
        if(this.print) {
          console.log('id ' + id + ' ran immediately');
        }
        cb(); // call allowed
        this._callOthers();
      } else {
        if( peer.cb !== null ) {
          if(this.print) {
            console.log('id ' + id + ' called twice and a callback was dropped');
          }
        } else {
          if(this.print) {
            console.log('id ' + id + ' saved callback');
          }
          peer.cb = cb; // save call
          if( this.printFirstBlock && this.firstBlockFlag ) {
            this.firstBlockPrint();
          }
        }
      }
    }
  }

  firstBlockPrint() {
    this.firstBlockFlag = false;
    console.log("FIRST BLOCK:");
    for(let id = 0; id < this.peerCount; id++) {
      let p = this._peers[id];
      console.log("peer " + id + ": calls " + p.c);
    }
  }

  maxWork() {
    let max = null;
    let maxid = null;
    let min = null;
    let minid = null;

    for(let id = 0; id < this.peerCount; id++) {
      let p = this._peers[id];
      if(id === 0) {
        max = p.c;
        maxid = id;
        min = p.c;
        minid = id;
      } else {
        if( p.c > max ) {
          max = p.c;
          maxid = id;
        }
        if( p.c < min ) {
          min = p.c;
          minid = id;
        }
      }
    }

    // console.log("max: " + max);
    // console.log("maxid: " + maxid);
    // console.log("min: " + min);
    // console.log("minid: " + minid);
    // console.log('');

    return {max,maxid,min,minid};

  }

  peers(n) {
    for(let id = 0; id < n; id++) {
      if( !this._peers.hasOwnProperty(id) ) {
        this._addKey(id);
      }
    }
  }

}


module.exports = {
    WorkKeeper
}
