const SCHEDULE_SLOTS = (50);
const SCHEDULE_LENGTH = (512);
const SCHEDULE_LENGTH_BITS = (9);
const SCHEDULE_FRAMES = ((SCHEDULE_SLOTS)*(SCHEDULE_LENGTH));

console.log('over ' + SCHEDULE_FRAMES + ' frames');

let p = 0;
let cnt = 0;
for( let i = 0; i < SCHEDULE_FRAMES*2; i++) {

  let progress = i % SCHEDULE_FRAMES;

  if( progress == 0) {
    cnt = 0;
  }

  let go = 0;
  switch(cnt) {
    case 0:
    case 6:
    case 12:
    case 18:
    case 24:
      go = 1;
      break;
    default:
      break;
  }

  if( (progress & (1024-1)) === 0) {
    let delta = i-p
    if( go ) {
      console.log(
        progress.toString().padEnd(5)
        + ' delta: ' + delta.toString().padEnd(4)
        + ' count: ' + cnt
       );
    }

    cnt++;
    p = i;
  }

}