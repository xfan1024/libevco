#include <evco/file.h>
#include <evco/signal.h>
#include <evco/sleep.h>
#include <stdio.h>
#include <sys/socket.h>

#include <boost/intrusive/list.hpp>
#include <vector>

#include "utils.h"

struct RuntimeData {
    struct sockaddr_storage server_addr;
    evco::Signal signal;
    int timeout;
    int number;
    int connected;
};

class Client : public evco::Coroutine {
public:
    void init(RuntimeData *data) {
        data_ = data;
    }

protected:
    void entry() override {
        std::vector<char> buf(4096, 'a');

        int fd = create_client(AF_INET);
        if (fd < 0) {
            return;
        }
        client_.set_fd(fd);
        const sockaddr *addr = (sockaddr *)&data_->server_addr;
        std::string str_addr = sockaddr_to_string(addr);
        if (client_.connect(addr, sockaddr_len(addr)) < 0) {
            if (errno == EINTR) {
                return;
            }
            fprintf(stderr, "connect to %s failed: %s\n", str_addr.c_str(), strerror(errno));
            return;
        }
        data_->connected++;
        // wait for all clients connected
        if (data_->signal.wait([this]() { return data_->connected == data_->number; })) {
            data_->signal.notify_all();
        }

        while (1) {
            if (!client_.write_ensure(&buf[0], buf.size())) {
                return;
            }
            if (!client_.read_ensure(&buf[0], buf.size())) {
                return;
            }
        }
    }

private:
    RuntimeData *data_;
    evco::File client_;
};

class Timer : public evco::Coroutine {
public:
    Timer(RuntimeData *data) {
        data_ = data;
    }

protected:
    void entry() override {
        evco::sleep(data_->timeout);
    }

private:
    RuntimeData *data_;
};

int echo_client_start(const char *server_addr, size_t number, int timeout) {
    RuntimeData data;
    data.server_addr = sockaddr_from_string(server_addr);
    data.timeout = timeout;
    data.number = number;
    data.connected = 0;

    if (data.server_addr.ss_family == AF_UNSPEC) {
        fprintf(stderr, "Invalid server address: %s\n", server_addr);
        return 1;
    }

    std::unique_ptr<Client[]> clients = std::make_unique<Client[]>(number);
    Timer timer(&data);
    std::vector<evco::Coroutine *> coroutines;

    int need_timer = timeout > 0;
    coroutines.reserve(number + need_timer + 1);
    auto finish_callback = [&]() {
        for (size_t i = 0; i < number; ++i) {
            // if not clean finish callback
            // this callback will run number times, in each time, this loop will run number times
            // so client.interrupt() will be called number squared times, too slow
            // so we need to clean finish callback, and this callback only run once
            clients[i].set_finish_callback(nullptr);
            clients[i].interrupt();
        }
        timer.set_finish_callback(nullptr);
        timer.interrupt();
    };

    for (size_t i = 0; i < number; i++) {
        clients[i].init(&data);
        clients[i].set_finish_callback(finish_callback);
        coroutines.push_back(&clients[i]);
    }

    if (need_timer) {
        timer.set_finish_callback(finish_callback);
        coroutines.push_back(&timer);
    }

    coroutines.push_back(nullptr);
    example_run(coroutines.data());
    return 0;
}

int main(int argc, char *argv[]) {
    const char *server_addr = "127.0.0.1:8415";
    int timeout = 10;
    size_t clients_number = 10;

    if (argc > 4) {
        fprintf(stderr, "usage: %s [server [clients_number [timeout]]]\n", argv[0]);
        return 1;
    }
    if (argc > 1) {
        server_addr = argv[1];
    }
    if (argc > 2) {
        clients_number = atoi(argv[2]);
    }
    if (argc > 3) {
        timeout = atoi(argv[3]);
    }

    return echo_client_start(server_addr, clients_number, timeout);
}
