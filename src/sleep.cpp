#include <evco/sleep.h>

#include "evco_internal.h"

namespace evco {

static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents) {
    (void)loop;
    (void)revents;
    Coroutine *co = (Coroutine *)w->data;
    co->resume();
}

static bool sleep_impl(Coroutine *co, double seconds) {
    if (seconds < 0) {
        errno = EINVAL;
        return false;
    }

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

bool sleep(Coroutine *co, unsigned int seconds) {
    return sleep_impl(co, seconds);
}

bool msleep(Coroutine *co, unsigned int milliseconds) {
    return sleep_impl(co, milliseconds / 1000.0);
}

bool usleep(Coroutine *co, unsigned int useconds) {
    return sleep_impl(co, useconds / 1000000.0);
}

}  // namespace evco
