#include <evco/sem.h>
#include <stdio.h>

namespace evco {

Semaphore::Semaphore(int count) : count_(count) {
}

Semaphore::~Semaphore() {
    fprintf(stderr, "%s: semaphore is pending, should report to developer to fix\n", __func__);
}

void Semaphore::post() {
    if (pending_ctxs_.empty()) {
        ++count_;
        return;
    }
    Context *ctx = pending_ctxs_.front();
    pending_ctxs_.pop();
    ctx->resume();
}

bool Semaphore::wait(Context *ctx) {
    if (count_ > 0) {
        --count_;
        return true;
    }
    pending_ctxs_.push(ctx);
    ctx->yield();
    return !ctx->interrupted;
}

}  // namespace evco
