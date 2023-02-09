#ifndef __evco_file_h__
#define __evco_file_h__

#include <evco/evco.h>
#include <sys/socket.h>

namespace evco {

class File {
public:
    File(int fd = -1);
    ~File();

    void set_fd(int fd);
    int get_fd();
    bool is_open();

    // release() will make the fd_ negative and complete all pending requests
    void release();

    // close() equals to release() and close the fd
    void close();

    ssize_t read(Context *ctx, void *buf, size_t size);
    ssize_t write(Context *ctx, const void *buf, size_t size);

    bool read_ensure(Context *ctx, void *buf, size_t size);
    bool write_ensure(Context *ctx, const void *buf, size_t size);

    int accept(Context *ctx, sockaddr *addr, socklen_t *addrlen);

private:
    bool check_before_io(bool rio);
    bool wait_io(Context *ctx, bool rio);
    void handle_io_callback(bool rio, int revents);
    static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents);
    static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents);

    int fd_{-1};
    bool error_{false};
    struct ev_io rio_ {};
    struct ev_io wio_ {};

    // for simplicity, we only support one read and one write request at the same time
    Context *req_read_{nullptr};
    Context *req_write_{nullptr};
};

}  // namespace evco

#endif  // __evco_file_h__
