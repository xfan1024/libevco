#ifndef __evco_sem_h__
#define __evco_sem_h__

#include <evco/file.h>

namespace evco {

class Semaphore {
public:
    Semaphore(int count = 0);

    void post();
    int wait(Context *ctx);

private:
    File file_;
    int count_{0};
};

}  // namespace evco

#endif  // __evco_sem_h__