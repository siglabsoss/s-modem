#pragma once

#define NOOP_EV (0)
#define REQUEST_FINE_SYNC_EV (1)
#define RADIO_ENTERED_BG_EV (2)
#define REQUEST_TDMA_EV (3)
#define FINISHED_TDMA_EV (4)
#define UDP_SEQUENCE_DROP_EV (5)
#define REQUEST_MAPMOV_DEBUG (6)
#define FSM_PING_EV (7)
#define FSM_PONG_EV (8)
#define EXIT_DATA_MODE_EV (9)
#define CHECK_TDMA_EV (10)
#define DUPLEX_FINE_SYNC (11)


/// List all custom event types here
/// NOTE if you are adding a new event type, please be aware
/// that RadioEstimate and EventDspFsm FILTER and DROP events by default
/// see :
/// - RadioEstimate::handleCustomEvent
/// - EventDsp::handleCustomEventRx
/// - EventDsp::handleCustomEventTx


#define MAKE_EVENT(x,y) (custom_event_t){(x),(y)}