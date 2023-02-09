#include <evco/signal.h>
#include <stdio.h>

namespace evco {

Signal::Signal() {
}

Signal::~Signal() {
    if (!pending_ctxs_.empty()) {
        fprintf(stderr, "%s: %zu pending, should report to developer to fix\n", __func__, pending_ctxs_.size());
        abort();
    }
}

void Signal::notify() {
    if (pending_ctxs_.empty()) {
        return;
    }
    static_cast<ContextNode *>(pending_ctxs_.pop())->ctx->resume();
}

void Signal::notify_all() {
    while (!pending_ctxs_.empty()) {
        static_cast<ContextNode *>(pending_ctxs_.pop())->ctx->resume();
    }
}

bool Signal::wait(Context *ctx) {
    ContextNode node;
    node.ctx = ctx;
    pending_ctxs_.push(&node);
    ctx->yield();
    node.unlink();
    if (ctx->interrupted) {
        errno = EINTR;
        return false;
    }
    return true;
}

}  // namespace evco
