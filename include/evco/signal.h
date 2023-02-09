#ifndef __evco_signal_h__
#define __evco_signal_h__

#include <evco/file.h>

namespace evco {

class Signal {
public:
    Signal();

    void notify();
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
    bool pending_{false};
    File file_;
};

}  // namespace evco

#endif  // __evco_signal_h__
