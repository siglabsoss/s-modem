# Native Streams
This cpp code was modeled after nodejs streams, but better, and in cpp.  I used libevent's data passing capabilities and added some sugar on top.


# Flow
Multiple streams objects are created and the pipe()'d together.  Each class has it's own thread and does all work in this thread.  All data passed is in bytes.  When there is data ready, the gotData() method will be called.  when gotData is called a stream must process all of it.

# Creation
Inherit from BevStream() class.  See GainStream for a barebones example.

## Methods
The following must be implemented
* `setBufferOptions` - Set options such as `bufferevent_setwatermark()`
* `gotData` - Called when there is new data
* `stopThreadDerivedClass` - add any pre-shutdown code here, can be left empty


# Chains
A chain is a collection of `BevStream` objects that were pipe()'d together.  It should (must) end with ToJs.  ToJs is specal because it interacts with `libuv`.