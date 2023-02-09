#ifndef __evco_wait_h__
#define __evco_wait_h__

#include <evco/evco.h>
#include <stddef.h>

namespace evco {

class ListNode {
public:
    ListNode() : next(this), prev(this) {
    }

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
    struct ListNode *next;
    struct ListNode *prev;
};

struct ContextNode : ListNode {
    Context *ctx;
};

}  // namespace evco

#endif