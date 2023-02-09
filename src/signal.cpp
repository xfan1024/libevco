#include <evco/file.h>
#include <evco/signal.h>
#include <sys/eventfd.h>

namespace evco {

Signal::Signal() {
    int fd = eventfd(0, EFD_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "%s: failed to create eventfd: %s\n", __func__, strerror(errno));
        abort();
    }
    file_.set_fd(fd);
}

void Signal::notify() {
    if (!pending_) {
        return;
    }
    if (eventfd_write(file_.get_fd(), 1) < 0) {
        fprintf(stderr, "%s: failed to write eventfd: %s\n", __func__, strerror(errno));
        abort();
    }
}

bool Signal::wait(Context *ctx) {
    if (pending_) {
        fprintf(stderr, "%s: signal is already pending, cannot wait twice\n", __func__);
        abort();
    }
    pending_ = true;
    uint64_t value = 0;
    ssize_t n = file_.read(ctx, &value, sizeof(value));
    pending_ = false;
    if (n < 0) {
        return false;
    }
    assert(n == sizeof(value));
    assert(value == 1);
    return true;
}

}  // namespace evco
