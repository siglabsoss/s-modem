// https://codeburst.io/nodejs-streams-demystified-e0b583f0005

/*
  In this example we'll take input from console and write it to a file. Also we'll have some data 
  which is written on explicit calls to read method provided by `readable` stream.
*/

const stream = require("stream");

/*
Implementing our own stream, It should extend native stream class.
Since we are implementing readable stream it should extend native
readable class (stream.Readable)
*/

class myReadable extends stream.Readable{
  
    //Every class should have a constructor, which takes options as arguments. (REQUIRED)
  
    constructor(options){
        super(options); //Passing options to native class constructor. (REQUIRED)
        this.count = 10; //A self declared variable to limit the data which is to be read.
    }
    
    /*
    Every stream class should have a _read method which is called intrinsically by the api,
    when <streamObject>.read method is called, _read method is not to be called directly by the object.
    */
    _read(sz){
        console.log('sz ' + sz);
        if(this.count > 0){
          console.log('pushing');
            
            /*
              Pushing the data to the readable buffer when <streamObject>.read method is called
              This fires "data" event.
            */ 
            let ret = true;
            while(ret) {
              ret = this.push(`Data From Explicit Read Call : ${this.count} \n`);
            }
            //Decrement the count.
            this.count--;
        } else {
          // console.log('not pushing');
          // this.push();
        }
    }

}

//Now an object for myReadable stream class

/*
  options can be:
   - encoding: If you want buffer objects to be encoded to strings (Default: true)
   - highWaterMark: Memory for internal buffer of stream (Default: 16kb)
   - objectMode: Streams convert your data into binary, you can opt out of by setting this to true (Default: false)
*/

let myStream = new myReadable({highWaterMark:64});

//Here we'll create a file stream using the native "fs" api.
const fs = require("fs");

let fileStream = fs.createWriteStream("./output.txt"); //This creates a writable stream for the file "output.txt".

/*
  Now we'll just pipe our myStream to flieStream 
  (pipe calls read method intrinsically which in turn calls _read method intrinsically)
  Also it handles "data" event when fired, which in turn writes the chunk to the file.
*/
myStream.pipe(fileStream);

/*
  When Readable stream has no data left it fires "end" event.
*/

myStream.on("end",()=>{
    console.log("Ended --- ----");
});

/*
  Here `process.stdin` is builtin stream provided by node.js which represensts 
  Standard input, WE get data from console using this stream.
*/

process.stdin.on("data",(chunk)=>{
    myStream.push(chunk); //Push data from console to our readable stream
});




function fill() {




  var buffer = new ArrayBuffer(4);
  var int8View = new Uint8Array(buffer);
  var uint32View = new Uint32Array(buffer);
  // dataView.setInt32(0, 0x1234ABCD);
  uint32View[0] = 0x6263640a;

  const items = int8View;

  // const items = Uint8Array.from([0x61,0x20,0x20,0x20]);

  let runs = 0;
  let ret = true;
  while(ret) {
    ret = myStream.push(items);
    runs++;
  }
  console.log('runs: ' + runs);

}

function fillTime() {
  fill();
  setTimeout(fillTime, 1000);
}

fillTime();





console.log('type stuff, press ctrl+d to finish');