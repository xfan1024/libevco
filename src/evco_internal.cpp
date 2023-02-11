#include "evco_internal.h"

#include <ev.h>

namespace evco {

static thread_local Coroutine *co_ = nullptr;
static thread_local struct ev_loop *loop_ = nullptr;

struct ev_loop *current_loop() {
    return loop_;
}

Coroutine *current() {
    return co_;
}

void current(Coroutine *co) {
    co_ = co;
}

void current_loop(struct ev_loop *loop) {
    loop_ = loop;
}

}  // namespace evco
