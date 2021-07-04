'use strict';

function prepare(a) {
    const asBuf = Buffer.from(a, 'ascii');
    const bufz = Buffer.from([0x0]);

    return Buffer.concat([asBuf, bufz]);
}

class NullBufferSplit {
    constructor(cb1) {
        this.string_buffer = "";

        if( typeof cb1 === 'undefined' ) {
            throw(new Error("NullBufferSplit constructor must have function callback as first argument"));
        }

        this.cb1 = cb1;
    }

    addData(data) {
       let as_str = data.toString('ascii');
       // console.log(as_str);

       this.string_buffer += as_str;

       // console.log("before, buffer:");
       //      console.log(string_buffer);
       //      debugString(string_buffer);

       //  console.log("------------");

       let largest_slice = null;
       let prev_slice = 0;
       
       for (let i = 0; i < this.string_buffer.length; i++) {
          if( this.string_buffer.charAt(i) == "\0" ) {
            // console.log("found zero " + i);
            // console.log("slice: " + prev_slice + " " + i );

            let validLength = prev_slice !== i;

            if( validLength ) {
                let slc = this.string_buffer.slice(prev_slice, i);
                // console.log();
                // console.log();
                // console.log();
                // console.log(slc);
                // debugString(slc);
                setImmediate(()=>{this.cb1(slc)});
            }
            
            // executeCustomDDPUpdate(slc);
            largest_slice = i;
            prev_slice = i+1;
          }
        }

        // if(largest_slice === null) {
        //     executeCustomDDPUpdate(this.string_buffer);
        //     this.string_buffer = "";
        // }

        if(largest_slice !== null) {
            this.string_buffer = this.string_buffer.slice(largest_slice+1);
            // console.log("after, buffer:");
            // console.log(string_buffer);
            // debugString(string_buffer);
            // console.log(typeof string_buffer);
        }
    }
}




module.exports = {
    NullBufferSplit
  , prepare
}
