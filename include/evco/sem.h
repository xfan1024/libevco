#ifndef __evco_sem_h__
#define __evco_sem_h__

#include <evco/coroutine.h>
#include <evco/list.h>

namespace evco {

class Semaphore {
public:
    Semaphore(int count = 0);
    ~Semaphore();

    void post();
    bool wait(Coroutine *co);

private:
    ListNode pending_ctxs_;
    size_t count_{0};
};

}  // namespace evco

#endif  // __evco_sem_h__