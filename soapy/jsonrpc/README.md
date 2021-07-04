# re-generate side-control keys
These steps allow you to add side-control keys.
* Edit the spec.json
  * Add something like `{"name" : "triggerTxSymbolSync"}`
* run `make`
* edit `JsonServer.hpp`
  * Add `void triggerTxSymbolSync();` with matching arguments to the json
* edit `JsonServer.cpp`
  * Add the function...
  * In the cpp you have a pointer to HiggsSoapyDriver.
  * Call the function in the next step
* Write a new function in `HiggsSoapyDriver.cpp`
* Update `HiggsSoapyDriver.hpp`
* Update `modem_main.cpp` and add a hotkey
