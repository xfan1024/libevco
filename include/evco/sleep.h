#ifndef __evco_sleep_h__
#define __evco_sleep_h__

#include <evco/coroutine.h>

namespace evco {

bool sleep(Coroutine *co, unsigned int seconds);
bool msleep(Coroutine *co, unsigned int milliseconds);
bool usleep(Coroutine *co, unsigned int useconds);

}  // namespace evco

#endif  // __evco_sleep_h__
