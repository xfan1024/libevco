#ifndef __evco_p_h__
#define __evco_p_h__

struct ev_loop;

namespace evco {

class Coroutine;

struct ev_loop *current_loop();
void current_loop(struct ev_loop *loop);
void current_loop_clear();

Coroutine *current();
void current(Coroutine *co);

}  // namespace evco

#endif  // __evco_p_h__
