#ifndef __utils_h__
#define __utils_h__

#include <evco/coroutine.h>
#include <stdio.h>
#include <sys/socket.h>

#include <functional>
#include <string>
#include <utility>

void example_run(evco::Coroutine **coroutines);

int create_server(const sockaddr *addr);
int create_client(int af_family);

socklen_t sockaddr_len(const sockaddr *addr);
std::string sockaddr_to_string(const sockaddr *addr);
sockaddr_storage sockaddr_from_string(const char *str);

static inline sockaddr_storage sockaddr_from_string(const std::string &str) {
    return sockaddr_from_string(str.c_str());
}
static inline bool sockaddr_has_value(const sockaddr *addr) {
    return addr->sa_family != AF_UNSPEC;
}

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

#endif  // __utils_h__
