ZMQ is a socket library that gives us extra features ontop of tcp.

We use ZMQ to transport many types of data between the radios.  Currently all of these data transports are done via wifi.

# Flow

* HiggsDriver::zmqHandleMessage
* HiggsDriver::zmqAcceptRemoteFeedbackBus
* HiggsDriver::zmqSendFeedbackBusForPcBevOffThread
* EventDsp::handleParsedFrameForPc
* EventDsp::handlePcPcVectorType