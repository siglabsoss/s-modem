const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const request = require("request");


if(isMainThread) {
    console.log("This is the main thread")

    let w = new Worker(__filename, {workerData: null});
    w.on('message', (msg) => { //A message from the worker!
        console.log("First value is: ", msg.val);
        console.log("Took: ", (msg.timeDiff / 1000), " seconds");
    })
    w.on('error', console.error);
    w.on('exit', (code) => {
        if(code != 0)
            console.error(new Error(`Worker stopped with exit code ${code}`))
   });

    request.get('http://www.google.com', (err, resp) => {
        if(err) {
            return console.error(err);
        }
        console.log("Total bytes received: ", resp.body.length);
    })

} else { //the worker's code

    function random(min, max) {
        return Math.random() * (max - min) + min
    }

    const sorter = require("./test2-worker");

    const start = Date.now()
    let bigList = Array(1000000).fill().map( (_) => random(1,10000))

    sorter.sort(bigList);
    parentPort.postMessage({ val: sorter.firstValue, timeDiff: Date.now() - start});

} 