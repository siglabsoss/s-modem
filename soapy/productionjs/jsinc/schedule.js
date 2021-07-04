const hc = require('../../js/mock/hconst.js');

function add_frames_to_schedule(old, frames) {
  let ret = [old[0],old[1]];
  let outstanding = frames;
  while(outstanding > hc.SCHEDULE_FRAMES) {
    outstanding -= hc.SCHEDULE_FRAMES;
    ret[0]++;
  }

  ret[1] += outstanding;

  while( ret[1] >= hc.SCHEDULE_FRAMES ) {
    ret[1] -= hc.SCHEDULE_FRAMES;
    ret[0]++;
  }

  return ret;
}



module.exports = {
  add_frames_to_schedule
};