#include <evco/file.h>
#include <evco/list.h>
#include <evco/signal.h>
#include <evco/sleep.h>
#include <stdio.h>
#include <sys/socket.h>

#include <vector>

#include "utils.h"

class Speeder;
class Server;

struct RuntimeData {
    Speeder *speeder;
    Server *server;
};

class Speeder : public evco::Coroutine {
public:
    void init(RuntimeData *data) {
        data_ = data;
    }

    void stat_tx(size_t bytes) {
        tx_bytes_ += bytes;
        signal_.notify();
    }

    void stat_rx(size_t bytes) {
        rx_bytes_ += bytes;
        signal_.notify();
    }

    void entry() override {
        while (1) {
            auto start = std::chrono::steady_clock::now();
            if (!evco::sleep(1)) {
                return;
            }
            if (tx_bytes_ == 0 && rx_bytes_ == 0) {
                signal_.wait([this]() { return tx_bytes_ > 0 || rx_bytes_ > 0; });
                continue;
            }
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            size_t tx_speed = tx_bytes_ * 1000 / duration.count();
            size_t rx_speed = rx_bytes_ * 1000 / duration.count();
            tx_bytes_ = 0;
            rx_bytes_ = 0;
            printf("[TX] %6s [RX] %6s\n", speed_to_string(tx_speed).c_str(), speed_to_string(rx_speed).c_str());
        }
    }

private:
    static std::string speed_to_string(size_t bytes) {
        if (bytes < 10ull * 1024) {
            return std::to_string(bytes) + "B/s";
        } else if (bytes < 10ull * 1024 * 1024) {
            return std::to_string(bytes / 1024) + "KB/s";
        } else if (bytes < 10ull * 1024 * 1024 * 1024) {
            return std::to_string(bytes / 1024 / 1024) + "MB/s";
        } else {
            return std::to_string(bytes / 1024 / 1024 / 1024) + "GB/s";
        }
    }
    size_t tx_bytes_{0};
    size_t rx_bytes_{0};
    evco::Signal signal_;
    RuntimeData *data_{nullptr};
};

class Client : public evco::Coroutine, public evco::ListNode {
public:
    void init(RuntimeData *data, int fd, const sockaddr *addr, socklen_t addrlen) {
        data_ = data;
        client_.set_fd(fd);
        memcpy(&remote_addr_, addr, addrlen);
        addrlen = sizeof(local_addr_);
        getsockname(fd, (sockaddr *)&local_addr_, &addrlen);
    }

protected:
    void entry() override {
        std::string str_addr;
        str_addr = sockaddr_to_string((sockaddr *)&remote_addr_);

        printf("connected by %s\n", str_addr.c_str());
        while (1) {
            ssize_t nread = client_.read(buffer_, sizeof(buffer_));
            if (nread <= 0) {
                printf("disconnected %s\n", str_addr.c_str());
                return;
            }
            data_->speeder->stat_rx(nread);
            if (!client_.write_ensure(buffer_, nread)) {
                printf("disconnected %s\n", str_addr.c_str());
                return;
            }
            data_->speeder->stat_tx(nread);
        }
    }

private:
    RuntimeData *data_{nullptr};
    sockaddr_storage local_addr_{};
    sockaddr_storage remote_addr_{};
    evco::File client_;
    char buffer_[65536];
};

class Server : public evco::Coroutine {
public:
    void init(RuntimeData *data, int fd) {
        data_ = data;
        listener_.set_fd(fd);
    }

protected:
    void entry() override {
        std::string client_base_name = "Client-";
        sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);

        while (1) {
            int cfd = listener_.accept((sockaddr *)&addr, &addrlen);
            if (cfd < 0) {
                // closed
                break;
            }

            Client *client = new Client();
            client->init(data_, cfd, (sockaddr *)&addr, addrlen);
            client->set_name(client_base_name + sockaddr_to_string((sockaddr *)&addr));
            client_list_.push(client);
            client->set_finish_callback([&, client](evco::Coroutine *) {
                client->unlink();
                delete client;
            });

            client->start();
        }

        // remove all clients from list
        while (!client_list_.empty()) {
            Client *client = static_cast<Client *>(client_list_.pop());
            client->interrupt();
        }
    }

private:
    evco::ListNode client_list_;
    evco::File listener_;
    RuntimeData *data_{nullptr};
};

int echo_server(const sockaddr *addr) {
    int fd = create_server(addr);
    if (fd < 0) {
        return 1;
    }

    RuntimeData data;
    Server server{};
    Speeder speeder{};
    server.init(&data, fd);
    speeder.init(&data);
    data.server = &server;
    data.speeder = &speeder;

    server.set_name("Server");
    speeder.set_name("Speeder");

    server.set_finish_callback([&](evco::Coroutine *) { speeder.interrupt(); });

    evco::Coroutine *coroutines[] = {&server, &speeder, nullptr};
    example_run(coroutines);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *addr_str = "127.0.0.1:8415";
    if (argc > 2) {
        fprintf(stderr, "usage: %s ip:port\n", argv[0]);
        return 1;
    }
    if (argc > 1) {
        addr_str = argv[1];
    }
    sockaddr_storage addr = sockaddr_from_string(addr_str);
    if (!sockaddr_has_value((sockaddr *)&addr)) {
        fprintf(stderr, "invalid address: %s\n", addr_str);
        return 1;
    }
    std::string str_addr = sockaddr_to_string((sockaddr *)&addr);
    printf("listen on %s\n", str_addr.c_str());
    return echo_server((sockaddr *)&addr);
}
