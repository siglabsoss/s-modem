# Js folder
This folder has javascript files to help s-modem development.  Currently I am about 40% done implementing higgs functionality.

## Mock Higgs
This is a piece of javascript that behaves identically to higgs hardware.  It opens ethernet ports for data and ringubs, and s-modem connects to it.  From s-modems perspective, there is no difference.  The purpose of this is to allow rapid development because with deterministic behavior of "higgs".

## Mock Environment
This is a fake rf enviornment that will add reciprocal phase rotations and any other forms of rf distortions we want.  This is the `js/mock/mockenvironment.js` file.  A single instance of this object is made, and then each instance of the "mock higgs" connect to it by calling `attachRadio()`.  The environemnt waits until all connected radios have output samples, and then computes the rx samples equally.  It forces the streams to be in lock-step.

# Installing (Ubuntu)
Before installing ubuntu needs these deps installed
```bash
sudo apt-get install libjpeg-dev libgif-dev
sudo apt-get install libcairo2-dev
```

# Installing
```bash
nvm use
npm i
```

# Run Mock Higgs
You will need to edit `soapy/src/common/constants.hpp` and set:
```c++
#define HIGGS_IP     "127.0.0.1"
```

For now only the tx side of "Mock higgs" is working, so you will need to edit `init.json` and set these lines:
```bash
"role":"tx"
"debug_tx_fill_level":true
```

The `debug_tx_fill_level` option is optional, but shows an example of simple s-modem operation.

Now run higgs first from `libs/s-modem/soapy/js` with:
```bash
node mock/mockall.js 
```

Now run s-modem from `libs/s-modem/soapy/build` with:
```bash
make -j3 HiggsDriver modem_main && ./modem_main
```

You should see output from both windows showing that they are connected.

# Speed
Currently the tx-chain alone runs at about 1/10th of "realtime" (on my laptop).  This causes significant problems as many parts of s-modem use system clock timers.  My current (hacked and wip) solution is to set an artifical limit on mock higgs to be slower than 1/10th to allow headroom (edit `let globalRateLimit = (1024+256) * (1000)` in `mockall.js`) and then to lengthen all the s-modem system timer calls by the correct amount.  Currently this method is very manual, and will need to be cleaned up in the future.

As we add more mock-higgs, we may wish to come up with a multi-computer solution, so that we are not running at 1/100th of speed with 10 radios in the enviornment etc.


# Tests
To runs tests from this directory, run:
```bash
./node_modules/.bin/mocha
```

## Running Specific Tests
Mocha has the `-g` flag which gives a case sensative grep for tests names.  For example you can run
```bash
node node_modules/.bin/mocha  -g 'FFT'
```
which will execute all tests with `FFT` in the name.

## Running worker tests
Just messing with this for now, but:
```bash
NODE_OPTIONS=--experimental-worker node node_modules/.bin/mocha
```

# Further Reading
* streams - https://codeburst.io/nodejs-streams-demystified-e0b583f0005
* streams - https://nodejs.org/docs/latest/api/stream.html#stream_writable_writev_chunks_callback
* streams - https://github.com/substack/stream-handbook
* typed arrays - https://nodejs.org/docs/latest/api/buffer.html#buffer_buffers_and_typedarray
* async / await - https://hackernoon.com/6-reasons-why-javascripts-async-await-blows-promises-away-tutorial-c7ec10518dd9