#ifndef __evco_signal_h__
#define __evco_signal_h__

#include <evco/evco.h>
#include <evco/list.h>

namespace evco {

class Signal {
public:
    Signal();
    ~Signal();

    void notify();
    void notify_all();
    bool wait(Context *ctx);

    template <typename Pred>
    bool wait(Context *ctx, Pred pred) {
        while (!pred()) {
            if (!wait(ctx)) {
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
