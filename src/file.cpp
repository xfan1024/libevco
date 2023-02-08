#include <evco/file.h>

namespace evco {

CoroutineFile::CoroutineFile(int fd) {
    if (fd >= 0) {
        set_fd(fd);
    }
}

CoroutineFile::~CoroutineFile() {
    close();
}

void CoroutineFile::set_fd(int fd) {
    if (fd_ >= 0) {
        fprintf(stderr, "%s: fd should be negative, should report to developer to fix\n", __func__);
        close();
    }
    fd_ = fd;
    error_ = false;
    rio_.data = this;
    wio_.data = this;
    ev_io_init(&rio_, read_cb, fd, EV_READ);
    ev_io_init(&wio_, write_cb, fd, EV_WRITE);
    req_read_ = nullptr;
    req_write_ = nullptr;
}

int CoroutineFile::get_fd() {
    return fd_;
}

bool CoroutineFile::is_open() {
    return fd_ >= 0;
}

void CoroutineFile::release() {
    if (fd_ < 0) {
        return;
    }

    if (req_read_) {
        ev_io_stop(req_read_->loop, &rio_);
    }

    if (req_write_) {
        ev_io_stop(req_write_->loop, &wio_);
    }

    fd_ = -1;

    CoroutineContext *req_read = req_read_;
    CoroutineContext *req_write = req_write_;
    req_read_ = nullptr;
    req_write_ = nullptr;

    if (req_read) {
        req_read->resume();
    }

    if (req_write) {
        req_write->resume();
    }
}

void CoroutineFile::close() {
    if (fd_ < 0) {
        return;
    }
    int fd = fd_;
    release();
    ::close(fd);
}

ssize_t CoroutineFile::read(CoroutineContext *ctx, void *buf, size_t size) {
    if (!check_before_io(true)) {
        return -1;
    }
    if (!wait_io(ctx, true)) {
        return -1;
    }

    return ::read(fd_, buf, size);
}

ssize_t CoroutineFile::write(CoroutineContext *ctx, const void *buf, size_t size) {
    if (!check_before_io(false)) {
        return -1;
    }
    if (!wait_io(ctx, false)) {
        return -1;
    }

    return ::write(fd_, buf, size);
}

int CoroutineFile::accept(CoroutineContext *ctx, sockaddr *addr, socklen_t *addrlen) {
    if (!check_before_io(true)) {
        return -1;
    }
    if (!wait_io(ctx, true)) {
        return -1;
    }

    int fd = ::accept(fd_, addr, addrlen);
    if (fd < 0) {
        return -1;
    }

    return fd;
}

bool CoroutineFile::wait_io(CoroutineContext *ctx, bool rio) {
    CoroutineContext *&req = rio ? req_read_ : req_write_;
    struct ev_io *io = rio ? &rio_ : &wio_;

    assert(req == nullptr);
    req = ctx;
    ev_io_start(ctx->loop, io);
    ctx->yield();
    if (fd_ < 0) {
        errno = EBADF;
        return false;
    }
    req = nullptr;
    ev_io_stop(ctx->loop, io);

    if (error_) {
        close();
        errno = EBADF;
        return false;
    }

    if (ctx->interrupted) {
        errno = EINTR;
        return false;
    }

    return true;
}

bool CoroutineFile::check_before_io(bool rio) {
    if (fd_ < 0) {
        errno = EBADF;
        fprintf(stderr, "%s: fd is invalid\n", __func__);
        return false;
    }
    if (error_) {
        errno = EINVAL;
        fprintf(stderr, "%s: fd is in error state\n", __func__);
        return false;
    }
    if (rio && req_read_) {
        errno = EBUSY;
        fprintf(stderr, "%s: there is already a read request\n", __func__);
        return false;
    }
    if (!rio && req_write_) {
        errno = EBUSY;
        fprintf(stderr, "%s: there is already a write request\n", __func__);
        return false;
    }
    return true;
}

void CoroutineFile::handle_io_callback(bool rio, int revents) {
    int expected_event = rio ? EV_READ : EV_WRITE;
    CoroutineContext *context = rio ? req_read_ : req_write_;

    if (revents & EV_ERROR) {
        fprintf(stderr, "%s: unknown EV_ERROR, should report to developer to fix\n", __func__);
        error_ = true;
    } else if (!(revents & expected_event)) {
        fprintf(stderr, "%s: unknown event: %d, should report to developer to fix\n", __func__, revents);
        error_ = true;
    }

    context->resume();
}

void CoroutineFile::read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    (void)loop;
    CoroutineFile *cofile = (CoroutineFile *)w->data;
    cofile->handle_io_callback(true, revents);
}

void CoroutineFile::write_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    (void)loop;
    CoroutineFile *cofile = (CoroutineFile *)w->data;
    cofile->handle_io_callback(false, revents);
}

}  // namespace evco
