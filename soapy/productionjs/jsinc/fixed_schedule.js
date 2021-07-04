'use strict'

const mutil = require('../../js/mock/mockutils.js');
const hc = require('../../js/mock/hconst.js');
const schedule = require('./schedule.js');

class FixedSchedule {
  constructor(api) {
    
    this.api = api;
    this.name = 'FixedSchedule';


    this.setup();

    // this.mode = 0;
    this.mode(3);

    this.history = {};

    this._wp = 0;
    this._wpp = 0;
    this.auto = false;

  }


  setup() {

  }

  loop(start,stop,step) {
    let r = [];
    for(let i = start; i < stop; i += step) {
      r.push(i);
    }
    return r;
  }


  mode(m) {
    if( m === 0 ) {
      this.allWake = this.loop(8, 44, 9);
      this.words = 12E3;
    }

    if( m === 1 ) {
      this.allWake = this.loop(8, 44, 10);
      this.words = 30E3;
    }

    if( m === 2 ) {
      this.allWake = this.loop(8, 44, 12);
      this.words = 240E3;
    }

    if( m === 3 ) {
      this.allWake = this.loop(8, 50, 19);
      // this.words = 200E3;
      this.words = 150E3;
    }

    this.allWake = [0].concat(this.allWake);
    this._m = m;
  }


  frameForTs(ts) {
    return ts*512;
  }

  // api
  wakeupForEpoc(sec) {
    //console.log('asked for wake ' + sec);
    //console.log(this.allWake);
    return this.allWake;
  }

  decideSpeed(y) {
    if( typeof y === 'undefined' ) {
      return;
    }
    // console.log(y);
    let sum = 0;
    for( let x of y ) {
      sum += x;
    }
    console.log("words " + sum);

    if( sum > 130000 ) {
      
      if( this._m !== 3 ) {
        console.log("\n\n Auto choosing mode 3\n\n");
        this.mode(3);
      }

    } else if( sum > 90000 ) {

      if( this._m !== 2 ) {
        console.log("\n\n Auto choosing mode 2\n\n");
        this.mode(2);
      }

    } else if( sum > 20000 ) {

      if( this._m !== 1 ) {
        console.log("\n\n Auto choosing mode 1\n\n");
        this.mode(1);
      }
    } else {
      if( this._wpp < 500 && this._wp < 500 ) {
        if( this._m !== 0 ) {
          console.log("\n\n Auto choosing mode 0\n\n");
          this.mode(0);
        }
      }
    }

    this._wpp = this._wp;
    this._wp = sum;


  }

  getWords(epoc, ts) {

    let words = this.api.wordsInTun();

    // console.log(ts);

    if( ts === 0 ) {

      if( words > 0 ) {
        console.log('' + words + ' words at ' + epoc);
      }

      if( this.auto ) {
        this.decideSpeed(this.history[epoc-1]);
      }

      this.history[epoc] = [words];    // build a new entry in history
    } else {
      if( typeof this.history[epoc] === 'undefined') {
        console.log('did not run for epoc ' + epoc + ' ts 0');
        this.history[epoc] = [0];
      }
      this.history[epoc].push(words);  // push subsequent onto existing history
    }

    return words;
  }

  // api
  hitTimeslot(ts) {
    let now = this.api.nowFrame();

    let epoc = now[0];
    // console.log('wokeup at ' + ts);
    // console.log('api reports ' + now + '   ' + now[1]/25600 * 50);
    // if( ts == 7) {
    //   this.api.send(now[0], 8, 1000);
    // }
    let words = this.getWords(now[0], ts);

    if( words > 0 ) {
      if( ts !== 0 ) {

        if( (epoc % 4) == 0 ) {
          return;

        }

        // if( this.words)

        // if( this._m === 0 ) {

          // every other second
          // if( (now[0] % 2) == 1 ) {

          //   // give a "breather" second where no ts larger than 20 is scheduled
          //   // this is a debug to help eq
          //   if( ts <= 30 ) {
          //     let res = this.api.send(now[0] + 2, ts, this.words);
          //   } else {
          //     console.log("breather");
          //   }
          // } else {
            // normal, full second

            if( this._m === 3 && ts == 27 ) {
              let longWords = 200E3;
              let res = this.api.send(now[0] + 3, ts, longWords);
            // mode 3 has an awk final timeslot
            } else if( this._m === 3 && ts == 46 ) {
              // console.log('breather');
              // let shortWords = 12E3;
              // let res = this.api.send(now[0] + 3, ts, shortWords);
              // do noithing
            } else {

              let res = this.api.send(now[0] + 3, ts, this.words);
            }

          // }



        // } else if( this._m == 1 ) {
        //   // if( words > 10E3 ) {
        //     let res = this.api.send(now[0] + 1, ts, this.words);
        //   // }
        // } else if( this._m == 2 ) {
        //   // if( words > 200E3 ) {
        //     let res = this.api.send(now[0] + 1, ts, this.words);
        //   // }
        // }


      }
    } else {
      // console.log('no words in hitTimeslot()');

      if( words > 2000000 ) {
        let dumped = this.api.dump();
        console.log('d ' + dumped + ' words');
      }

    }



  }


}

module.exports = {
    FixedSchedule
}
