#ifndef __evco_sleep_h__
#define __evco_sleep_h__

#include <evco/evco.h>

namespace evco {

int sleep(CoroutineContext *ctx, unsigned int seconds);
int msleep(CoroutineContext *ctx, unsigned int milliseconds);
int usleep(CoroutineContext *ctx, unsigned int useconds);

}  // namespace evco

#endif  // __evco_sleep_h__
