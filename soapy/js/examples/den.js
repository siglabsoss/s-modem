// shows correct input vs output size for our current modulation methods

let header = 16;
let enabled_subcarriers = 128;


let loop = () => {

  let outp = -1;
  for(let i = 20000-1; i >= 0; i--) {
    // let dsize = 100;
    let dsize = i;

    let out = header + Math.ceil((dsize*16)/enabled_subcarriers)*1024;

    if( out != outp ) {
      console.log("out:  " + out + ", in " + dsize);
    }
    outp = out;
  }

}

let single = () => {
    let dsize = 768;

    let out = header + Math.ceil((dsize*16)/enabled_subcarriers)*1024;

    console.log("out:  " + out + ", in " + dsize);
}


loop();
// single();


// console.log(out);