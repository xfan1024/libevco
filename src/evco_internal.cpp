#include "evco_internal.h"

#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

namespace evco {

static thread_local Coroutine *co_ = nullptr;
static thread_local struct ev_loop *loop_ = nullptr;

struct ev_loop *current_loop() {
    if (!loop_) {
        fprintf(stderr, "%s: current loop is not set, should report to developer to fix\n", __func__);
        abort();
    }
    return loop_;
}

void current_loop(struct ev_loop *loop) {
    if (loop_) {
        fprintf(stderr, "%s: current loop is already set, should report to developer to fix\n", __func__);
        abort();
    }
    if (!loop) {
        fprintf(stderr, "%s: loop is nullptr, should report to developer to fix\n", __func__);
        abort();
    }
    loop_ = loop;
}

void current_loop_clear() {
    if (!loop_) {
        fprintf(stderr, "%s: current loop is not set, should report to developer to fix\n", __func__);
        abort();
    }
    loop_ = nullptr;
}

Coroutine *current() {
    return co_;
}

void current(Coroutine *co) {
    co_ = co;
}

}  // namespace evco
