#include <evco/sem.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace evco {

static bool write_count(int f, uint64_t count) {
    ssize_t n = ::write(f, &count, sizeof(count));
    if (n != sizeof(count)) {
        fprintf(stderr, "write eventfd failed: %s\n", strerror(errno));
        return false;
    }
    return true;
}

Semaphore::Semaphore(int count) : count_(count) {
    int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);
    assert(fd >= 0);
    file_.set_fd(fd);
    if (!write_count(fd, count)) {
        abort();
    }
}

void Semaphore::post() {
    if (!write_count(file_.get_fd(), 1)) {
        abort();
    }
}

int Semaphore::wait(Context *ctx) {
    uint64_t count;
    ssize_t n = file_.read(ctx, &count, sizeof(count));
    if (n < 0) {
        return (int)n;
    }
    assert(n == sizeof(count));
    assert(count == 1);
    return 0;
}

}  // namespace evco
