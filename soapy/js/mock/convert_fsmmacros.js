let a = [
{ tv_sec : 0, tv_usec : 100 },
{ tv_sec : 0, tv_usec : 1000 },
{ tv_sec : 0, tv_usec : 1000*5 },
{ tv_sec : 0, tv_usec : 1000*50 },
{ tv_sec : 0, tv_usec : 1000*100 },
{ tv_sec : 0, tv_usec : 1000*250 },
{ tv_sec : 0, tv_usec : 1000*500 },
{ tv_sec : 0, tv_usec : 1000*300 },
{ tv_sec : 0, tv_usec : 1000*700 },
{ tv_sec : 1, tv_usec : 0 },
{ tv_sec : 2, tv_usec : 0 },
{ tv_sec : 3, tv_usec : 0 },
{ tv_sec : 5, tv_usec : 0 },
{ tv_sec : 8, tv_usec : 0 },
{ tv_sec : 16, tv_usec : 0 },
{ tv_sec : 32, tv_usec : 0 },
{ tv_sec : 128, tv_usec : 0 }]


let factor = 24.4;

for( x of a ) {
  // console.log(x);

  us = x.tv_sec * 1E6 + x.tv_usec;

  // us after factor is applied
  us_f = us * factor;

  // console.log(us_f);
  // how many whole seconds
  sec_f = Math.floor(us_f / 1E6);

  // how many us remain
  usec_f = us_f - (sec_f * 1E6);

  // console.log(sec_f + ' - ' + us_f);

// { .tv_sec = 0, .tv_usec = 1000*700 }

  console.log(`{ .tv_sec = ${sec_f}, .tv_usec = ${usec_f} }`);

}