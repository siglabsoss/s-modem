const hc = require('./hconst.js');



const header = hc.FEEDBACK_HEADER_WORDS;

class MapMov {
  constructor() {
    this.map = undefined;
    this.pilot = undefined;
    this.enable_table = undefined;
    this.pilot_table = undefined;
    this.enabled_subcarriers = undefined;
  }

  getEnabledSubcarriers() {
    return enabled_subcarriers;
  }

  // pass in the number of words
  // that will enter the mapmov, and get the number
  // we should set in the feedback bus length field
  getPostMapMovCount(sz) {
    let y = header + Math.ceil((sz*16)/this.enabled_subcarriers)*1024;
    return y; // custom_size
  }

  getPreMapMovCount(y) {
    let sz = Math.floor((y-header)/1024) * this.enabled_subcarriers / 16;
    return sz;
  }

  reset() {
      this.map = [
          0xe95fe95f,
          0x16a1e95f,
          0xe95f16a1,
          0x16a116a1];

      this.pilot = 0x2000;

      this.enable_table = [];

      // build enabled table
      for(let sc = 0; sc < 1024; sc++) {
          // default 0
          this.enable_table[sc] = false;

          // if odd
          if( (sc & 0x1) == 1 ) {
              if( sc < 129 || sc > 896 ) {
                  this.enable_table[sc] = true;
              }
          }

          if( sc == 0 ) {
              this.enable_table[sc] = false;
          }
      }

      // loop through again to count how many subcarriers we set
      this.enabled_subcarriers = 0;
      for(let sc = 0; sc < 1024; sc++) {
        if( this.enable_table[sc] ) {
          this.enabled_subcarriers++;
        }
      }

      this.pilot_table = [];

      // build pilot tone table
      for(let sc = 0; sc < 1024; sc++) {
          if(
              ( sc <= 128 || sc >= 896 )
            )
              {
                  this.pilot_table[sc] = this.pilot;
              } else {
                  this.pilot_table[sc] = 0;
              }
      }

      // console.log(JSON.stringify(enable_table));
  }

  accept(a) {
      // std::vector<uint32_t> mapmov_mapped;

      let mapmov_mapped = [];

      // map the data
      for( let w of a ) {
        // console.log(w);

        for(let i = 0; i < 16; i++) {
            let bits = (w >> (i*2)) & 0x3;

            mapmov_mapped.push(this.map[bits]);
        }
      }

      let consumed = 0;
      let mapmov_out = [];

      while(consumed < mapmov_mapped.length) {
          for(let sc = 0; sc < 1024; sc++) {
              if(this.enable_table[sc]) {
                  mapmov_out.push( mapmov_mapped[consumed] );
                  consumed++;

                  if( consumed > mapmov_mapped.length ) {
                      throw new Error("mapmovjs::accept() out of bounds and may crash, fixme");
                  }

              } else {
                  mapmov_out.push( this.pilot_table[sc] );
              }
          }
      }

      return mapmov_out;
  } // accept

} // class



module.exports = {
    MapMov
};