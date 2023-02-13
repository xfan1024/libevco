#include <evco/coroutine.h>
#include <evco/sleep.h>
#include <stdio.h>

class HelloWorld : public evco::Coroutine {
protected:
    void entry() override {
        while (1) {
            printf("hello world\n");
            if (!evco::sleep(1)) {
                return;
            }
        }
    }
};

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    evco::init(loop);

    HelloWorld helloworld;
    helloworld.start();

    ev_run(loop, 0);
    evco::deinit();
    ev_loop_destroy(loop);
    return 0;
}
