#ifndef __evco_sleep_h__
#define __evco_sleep_h__

#include <evco/coroutine.h>

namespace evco {

bool sleep(unsigned int seconds);
bool msleep(unsigned int milliseconds);
bool usleep(unsigned int useconds);

}  // namespace evco

#endif  // __evco_sleep_h__
