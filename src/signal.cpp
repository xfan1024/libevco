#include <evco/signal.h>
#include <stdio.h>

namespace evco {

Signal::Signal() {
}

Signal::~Signal() {
    if (!pending_ctxs_.empty()) {
        fprintf(stderr, "%s: signal is pending, should report to developer to fix\n", __func__);
        abort();
    }
}

void Signal::notify() {
    if (pending_ctxs_.empty()) {
        return;
    }
    Context *ctx = pending_ctxs_.front();
    pending_ctxs_.pop();
    ctx->resume();
}

void Signal::notify_all() {
    while (!pending_ctxs_.empty()) {
        Context *ctx = pending_ctxs_.front();
        pending_ctxs_.pop();
        ctx->resume();
    }
}

bool Signal::wait(Context *ctx) {
    pending_ctxs_.push(ctx);
    ctx->yield();
    return !ctx->interrupted;
}

}  // namespace evco
