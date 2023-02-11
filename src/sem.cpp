#include <evco/sem.h>
#include <stdio.h>

#include "evco_internal.h"

namespace evco {

Semaphore::Semaphore(int count) : count_(count) {
}

Semaphore::~Semaphore() {
    if (!pending_ctxs_.empty()) {
        fprintf(stderr, "[%s:%d] %zu pending, should report to developer to fix\n", __func__, __LINE__, pending_ctxs_.size());
        abort();
    }
}

void Semaphore::post() {
    if (pending_ctxs_.empty()) {
        ++count_;
        return;
    }
    static_cast<CoroutineNode *>(pending_ctxs_.pop())->co->resume();
}

bool Semaphore::wait() {
    if (count_ > 0) {
        --count_;
        return true;
    }

    Coroutine *co = current();
    CoroutineNode node;
    node.co = co;
    pending_ctxs_.push(&node);
    co->yield();
    node.unlink();
    if (co->is_interrupted()) {
        errno = EINTR;
        return false;
    }
    return true;
}

}  // namespace evco
