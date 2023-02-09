#ifndef __evco_sem_h__
#define __evco_sem_h__

#include <evco/evco.h>

#include <queue>

namespace evco {

class Semaphore {
public:
    Semaphore(int count = 0);
    ~Semaphore();

    void post();
    bool wait(Context *ctx);

private:
    std::queue<Context *> pending_ctxs_;
    size_t count_{0};
};

}  // namespace evco

#endif  // __evco_sem_h__