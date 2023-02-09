#include <evco/sleep.h>

namespace evco {

static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents) {
    (void)loop;
    (void)revents;
    Context *ctx = (Context *)w->data;
    ctx->resume();
}

static int sleep_impl(Context *ctx, double seconds) {
    if (seconds < 0) {
        errno = EINVAL;
        return -1;
    }

    ev_timer timer;
    ev_timer_init(&timer, timer_cb, seconds, 0);
    timer.data = ctx;
    ev_timer_start(ctx->loop, &timer);
    ctx->yield();
    ev_timer_stop(ctx->loop, &timer);

    if (ctx->interrupted) {
        errno = EINTR;
        return -1;
    }

    return 0;
}

int sleep(Context *ctx, unsigned int seconds) {
    return sleep_impl(ctx, seconds);
}

int msleep(Context *ctx, unsigned int milliseconds) {
    return sleep_impl(ctx, milliseconds / 1000.0);
}

int usleep(Context *ctx, unsigned int useconds) {
    return sleep_impl(ctx, useconds / 1000000.0);
}

}  // namespace evco
