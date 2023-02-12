#ifndef __evco_signal_h__
#define __evco_signal_h__

#include <evco/coroutine.h>
#include <evco/list.h>

namespace evco {

class Signal {
public:
    Signal();
    ~Signal();

    void notify();
    void notify_all();
    bool wait();

    template <typename Pred>
    bool wait(Pred pred) {
        while (!pred()) {
            if (!wait()) {
                return false;
            }
        }
        return true;
    }

    template <typename Pred>
    bool fence(Pred pred) {
        if (!pred()) {
            return wait(pred);
        }
        notify_all();
        return true;
    }

private:
    ListNode pending_ctxs_;
};

}  // namespace evco

#endif  // __evco_signal_h__
