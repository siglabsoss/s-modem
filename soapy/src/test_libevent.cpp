#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

// Libevent
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>


struct event *clock_update_timer;
static void
clock_update(int fd, short kind, void *userp)
{
    /* Print current time. */
    time_t t = time(NULL);
    int now_s = t % 60;
    int now_m = (t / 60) % 60;
    int now_h = (t / 3600) % 24;
    /* The trailing \r will make cursor return to the beginning
     * of the current line. */
    printf("%02d:%02d:%02d\r", now_h, now_m, now_s);
    fflush(stdout);

    /* Next clock update in one second. */
    /* (A more prudent timer strategy would be to update clock
     * on the next second _boundary_.) */
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    evtimer_add(clock_update_timer, &timeout);
}


int main(int argc, char **argv)
{
    /* Set up the libevent library. */
    struct event_base *evbase = event_base_new();

    /* Set up the PubNub library, with a single shared context,
     * using the libevent backend for event handling. */
    // struct pubnub *p = pubnub_init("demo", "demo", &pubnub_libevent_callbacks, pubnub_libevent_init(evbase));

    /* Set the clock update timer. */
    clock_update_timer = evtimer_new(evbase, clock_update, NULL);
    clock_update(-1, EV_TIMEOUT, NULL);

    /* First step in the PubNub call sequence is publishing a message. */
    // publish(p);

    /* Here, we could start any other asynchronous operations as needed,
     * launch a GUI or whatever. */

    /* Start the event loop. */
    event_base_dispatch(evbase);

    /* We should never reach here. */
    // pubnub_done(p);
    event_base_free(evbase);
    return 0;
}