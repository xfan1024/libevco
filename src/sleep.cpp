#include <evco/sleep.h>

#include "evco_internal.h"

namespace evco {

static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents) {
    (void)loop;
    (void)revents;
    Coroutine *co = (Coroutine *)w->data;
    co->resume();
}

static bool sleep_impl(double seconds) {
    if (seconds < 0) {
        errno = EINVAL;
        return false;
    }

    Coroutine *co = current();
    ev_timer timer;
    ev_timer_init(&timer, timer_cb, seconds, 0);
    timer.data = co;
    ev_timer_start(current_loop(), &timer);
    co->yield();
    ev_timer_stop(current_loop(), &timer);

    if (co->is_interrupted()) {
        errno = EINTR;
        return false;
    }

    return true;
}

bool sleep(unsigned int seconds) {
    return sleep_impl(seconds);
}

bool msleep(unsigned int milliseconds) {
    return sleep_impl(milliseconds / 1000.0);
}

bool usleep(unsigned int useconds) {
    return sleep_impl(useconds / 1000000.0);
}

}  // namespace evco
