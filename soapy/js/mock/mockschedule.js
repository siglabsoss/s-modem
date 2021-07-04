const Bank = require('../mock/typebank.js');

const SCHEDULE_SLOTS = (50);
const SCHEDULE_LENGTH = (512);
const SCHEDULE_LENGTH_BITS = (9);
const SCHEDULE_FRAMES = ((SCHEDULE_SLOTS)*(SCHEDULE_LENGTH));


// typedef struct schedule_t {
//     uint32_t s[SCHEDULE_SLOTS];
//     uint32_t id; // our id
//     uint32_t id_mask; // our id as a mask
//     uint32_t offset; // offset to a counter
//     uint32_t epoc_time; // not an offset, actual epoc seconds, this gets changed inside get_timeslot2
// } schedule_t;

function schedule_t() {
  let obj = {};
  obj.s = new Bank.TypeBankUint32().access();
  obj.uint = new Bank.TypeBankUint32().access();
  obj.uint.id = 0;
  obj.uint.id_mask = 0;
  obj.uint.offset = 0;
  obj.uint.epoc_time = 0;
  return obj;
}

// sets id and mask
function set_id(o, id) {
    o.uint.id = id;
    o.uint.id_mask = feedback_peer_to_mask(id);
}

// came from feedback bus, actually belongs there
function feedback_peer_to_mask(peer_id) {
  let tmp = new Bank.TypeBankUint32().access();
  tmp.a = peer_id+1;
  tmp.b = 0x1<<(tmp.a);
  return tmp.b;
}


function schedule_all_on(o) {
  set_id(o, 1);
  for(let i = 0; i < SCHEDULE_SLOTS; i++) {
    o.s[i] = o.uint.id_mask; // us
  }
}



function schedule_get_timeslot2(o, counter) {
  let out = new Bank.TypeBankUint32().access();
  out.progress = 0;
  out.accumulated_progress = 0;
  out.timeslot = 0;
  out.epoc = 0;
  out.can_tx = 0;

  let stack = new Bank.TypeBankUint32().access();
  stack.start = 0;
  stack.now = 0
  stack.sample = 0;
  stack.counter_schedule_start = 0;

  stack.now = counter + o.uint.offset;

  // expensive, how to avoid?
  stack.sample = stack.now % SCHEDULE_FRAMES;

  // which sample did the current schedule start at?
  stack.counter_schedule_start = counter - stack.sample;
  // SET_REG(x3, sample);

  out.timeslot = stack.sample >> SCHEDULE_LENGTH_BITS; // same as divide by SCHEDULE_LENGTH

  // what sample did the timeslot start on
  stack.start = (out.timeslot * SCHEDULE_LENGTH);
  stack.start += stack.counter_schedule_start;

  out.progress = counter - stack.start;

  // bump at 0/0 that way we can always return the value that is in the struct
  if(
      out.timeslot == 0
      && out.progress == 0 ) {
      o.uint.epoc_time++;
  }

  out.epoc = o.uint.epoc_time;

  if(o.s[out.timeslot] & o.uint.id_mask) {
      out.can_tx = 1;
  } else {
      out.can_tx = 0;
  }

  out.accumulated_progress = stack.sample;

  return [
  out.progress,
  out.accumulated_progress,
  out.timeslot,
  out.epoc,
  out.can_tx
  ];


}



module.exports = {
    SCHEDULE_SLOTS
  , SCHEDULE_LENGTH
  , SCHEDULE_LENGTH_BITS
  , SCHEDULE_FRAMES
  , schedule_t
  , schedule_all_on
  , feedback_peer_to_mask
  , set_id
  , schedule_get_timeslot2


}