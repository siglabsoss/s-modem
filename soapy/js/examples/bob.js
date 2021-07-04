/*
In this example we'll take input from a file and write that to console.
*/

const stream = require("stream"); //require native stream library

/*
Implementing our own stream, It should extend native stream class.
Since we are implementing wrtiable stream it should extend native
writable class (stream.Writable)
*/

class myWritable extends stream.Writable {
  //Every class should have a constructor, which takes options as arguments. (REQUIRED)
  
  constructor(options){
    super(options) //Passing options to native constructor (REQUIRED)    
  }
  
  /*
    Every stream class should have a _write method which is called intrinsically by the api,
    when <streamObject>.write method is called, _write method is not to be called directly by the object.
  */
  
  _write(chunk,encoding,cb){
    let tag = Math.floor(Math.random() * 100);
    console.log('entered _write ' + tag + ' -> ' + JSON.stringify(chunk));
    console.log('writableLength ' + this.writableLength);

    function dly() {
        console.log('finished _write ' + tag);
        cb(); //It is important to callback, so that our stream can process the next chunk. 
    }

    setTimeout(dly, 1000);
  }

  // _writev(chunk, cb) {
  //   console.log('writeev');
  //   cb();
  // }

}

//Here we'll create a file stream using the native "fs" api.

const fs = require("fs");
let fileStream =  fs.createReadStream("hello.txt"); //This creates a readable stream for the file "hello.txt".

//Now an object for myWrtiable stream class

/*
  options can be:
   - decodeStrings: If you want strings to be buffered (Default: true)
   - highWaterMark: Memory for internal buffer of stream (Default: 16kb)
   - objectMode: Streams convert your data into binary, you can opt out of by setting this to true (Default: false)
*/

let myStream = new myWritable({highWaterMark:32});

/*
Now we'll just pipe our fileStream to myStream 
(pipe calls write method intrinsically which in turn calls _write method intrinsically)
*/


function fill() {
  const items = Uint8Array.from([0x61,0x20,0x20,0x20]);

  let runs = 0;
  let ret = true;
  while(ret) {
    ret = myStream.write(items);
    runs++;
  }
  console.log('runs: ' + runs);

}

function fillTime() {
  fill();
  setTimeout(fillTime, 500);
}

fillTime();




// fileStream.pipe(myStream);

/*
  When writable stream finishes writing data it fires "finish" event,
  When readable stream finishes reading data it fires "end" event.
  
  if you have piped a writable stream, like in our case
  it ends the writable stream once readable stream ends reading data by calling 
  <writableStreamObject>.end() method (you can use this manually in other cases)
*/

fileStream.on("end",()=>{
  console.log("Finished Reading");
});

myStream.on("finish",()=>{
  console.log("Finished Writing");
});