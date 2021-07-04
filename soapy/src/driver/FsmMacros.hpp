#pragma once

// Used in the case statements of tick()

#define GO_NOW(x)         0; __imed = x;
#define GO_AFTER(x, tvx)  0; __timer = x; tv = tvx;
#define GO_EVENT(x, ev)   0; __timer = x; fsm_event_pending = ev;
#define GO_SELF(tvx)       0; __timer = dsp_state; tv = tvx;

/////////////////////////////////////////////////////////////////////

#define SMALLEST_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1 }
#define TINY_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 100 }
#define _1_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000 }
#define _5_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*5 }
#define _10_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*10 }
#define _15_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*15 }
#define _50_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*50 }
#define _100_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*100 }
#define _250_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*250 }
#define _500_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*500 }
#define _300_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*300 }
#define _700_MS_SLEEP (struct timeval){ .tv_sec = 0, .tv_usec = 1000*700 }
#define _1_SECOND_SLEEP (struct timeval){ .tv_sec = 1, .tv_usec = 0 }
#define _2_SECOND_SLEEP (struct timeval){ .tv_sec = 2, .tv_usec = 0 }
#define _3_SECOND_SLEEP (struct timeval){ .tv_sec = 3, .tv_usec = 0 }
#define _4_SECOND_SLEEP (struct timeval){ .tv_sec = 4, .tv_usec = 0 }
#define _5_SECOND_SLEEP (struct timeval){ .tv_sec = 5, .tv_usec = 0 }
#define _6_SECOND_SLEEP (struct timeval){ .tv_sec = 6, .tv_usec = 0 }
#define _8_SECOND_SLEEP (struct timeval){ .tv_sec = 8, .tv_usec = 0 }
#define _16_SECOND_SLEEP (struct timeval){ .tv_sec = 16, .tv_usec = 0 }
#define _32_SECOND_SLEEP (struct timeval){ .tv_sec = 32, .tv_usec = 0 }
#define _128_SECOND_SLEEP (struct timeval){ .tv_sec = 128, .tv_usec = 0 }
#define _256_SECOND_SLEEP (struct timeval){ .tv_sec = 256, .tv_usec = 0 }

///
/// Comprised of the above, these control specific timers
/// 

#define DEFAULT_NEXT_STATE_SLEEP _1_MS_SLEEP
#define RADIO_TICK_SLEEP _1_MS_SLEEP
#define POLL_RB_SOCKET_TIME   _1_MS_SLEEP


