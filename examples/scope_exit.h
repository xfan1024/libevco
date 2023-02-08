#ifndef __scope_exit_h__
#define __scope_exit_h__

#include <functional>
#include <utility>

class scope_exit {
public:
    scope_exit(const scope_exit &) = delete;
    scope_exit &operator=(const scope_exit &) = delete;

    template <typename F>
    scope_exit(F &&f) {
        f_ = std::forward<F>(f);
    }
    ~scope_exit() {
        if (f_) {
            f_();
        }
    }

    void release() {
        f_ = nullptr;
    }

private:
    std::function<void()> f_;
};

#endif