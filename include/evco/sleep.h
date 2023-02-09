#ifndef __evco_sleep_h__
#define __evco_sleep_h__

#include <evco/evco.h>

namespace evco {

int sleep(Context *ctx, unsigned int seconds);
int msleep(Context *ctx, unsigned int milliseconds);
int usleep(Context *ctx, unsigned int useconds);

}  // namespace evco

#endif  // __evco_sleep_h__
