#include <evco/sem.h>
#include <stdio.h>

namespace evco {

Semaphore::Semaphore(int count) : count_(count) {
}

Semaphore::~Semaphore() {
    if (!pending_ctxs_.empty()) {
        fprintf(stderr, "%s: %zu pending, should report to developer to fix\n", __func__, pending_ctxs_.size());
        abort();
    }
}

void Semaphore::post() {
    if (pending_ctxs_.empty()) {
        ++count_;
        return;
    }
    static_cast<ContextNode *>(pending_ctxs_.pop())->ctx->resume();
}

bool Semaphore::wait(Context *ctx) {
    if (count_ > 0) {
        --count_;
        return true;
    }
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
