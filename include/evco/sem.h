#ifndef __evco_sem_h__
#define __evco_sem_h__

#include <evco/file.h>

namespace evco {

class CoroutineSemaphore {
public:
    CoroutineSemaphore(int count = 0);

    void post();
    int wait(CoroutineContext *ctx);

private:
    CoroutineFile file_;
    int count_{0};
};

}  // namespace evco

#endif  // __evco_sem_h__