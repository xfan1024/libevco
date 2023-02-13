#include <evco/signal.h>
#include <stdio.h>

#include "evco_internal.h"

namespace evco {

Signal::Signal() {}

Signal::~Signal() {
    if (!pending_ctxs_.empty()) {
        fprintf(stderr, "[%s:%d] %zu pending, should report to developer to fix\n", __func__, __LINE__, pending_ctxs_.size());
        abort();
    }
}

void Signal::notify() {
    if (pending_ctxs_.empty()) {
        return;
    }
    CoroutineNode *node = static_cast<CoroutineNode *>(pending_ctxs_.pop());
    node->co->resume();
}

void Signal::notify_all() {
    while (!pending_ctxs_.empty()) {
        CoroutineNode *node = static_cast<CoroutineNode *>(pending_ctxs_.pop());
        node->co->resume();
    }
}

bool Signal::wait() {
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
