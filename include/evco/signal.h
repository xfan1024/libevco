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
    bool wait(Coroutine *co);

    template <typename Pred>
    bool wait(Coroutine *co, Pred pred) {
        while (!pred()) {
            if (!wait(co)) {
                return false;
            }
        }
        return true;
    }

private:
    ListNode pending_ctxs_;
};

}  // namespace evco

#endif  // __evco_signal_h__
