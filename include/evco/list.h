#ifndef __evco_wait_h__
#define __evco_wait_h__

#include <evco/coroutine.h>
#include <stddef.h>

namespace evco {

class ListNode {
public:
    bool empty() {
        return next == this;
    }

    ListNode *pop() {
        if (empty()) {
            return nullptr;
        }
        ListNode *ret = next;
        next = ret->next;
        next->prev = this;
        ret->next = ret->prev = ret;
        return ret;
    }

    void push(ListNode *node) {
        node->next = this;
        node->prev = prev;
        prev->next = node;
        prev = node;
    }

    bool is_linked() {
        return next != this;
    }

    void unlink() {
        next->prev = prev;
        prev->next = next;
        next = prev = this;
    }

    size_t size() {
        size_t ret = 0;
        for (ListNode *p = next; p != this; p = p->next) {
            ++ret;
        }
        return ret;
    }

private:
    ListNode *next{this};
    ListNode *prev{this};
};

struct CoroutineNode : ListNode {
    Coroutine *co;
};

}  // namespace evco

#endif
