#include <arpa/inet.h>
#include <ev.h>
#include <evco/file.h>
#include <stdio.h>
#include <sys/socket.h>

#include <boost/intrusive/list.hpp>

#include "scope_exit.h"

int create_server(uint16_t port, int af_family) {
    int fd = socket(af_family, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    scope_exit fd_cleaner([fd] { close(fd); });
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    if (af_family == AF_INET) {
        sockaddr_in *addr4 = (sockaddr_in *)&addr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        addr4->sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (af_family == AF_INET6) {
        sockaddr_in6 *addr6 = (sockaddr_in6 *)&addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        addr6->sin6_addr = in6addr_any;
    } else {
        fprintf(stderr, "unknown address family: %d\n", af_family);
        return -1;
    }

    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }
    if (listen(fd, 1) < 0) {
        perror("listen");
        return -1;
    }

    fd_cleaner.release();
    return fd;
}

// return ipv4:port or [ipv6]:port
std::string sockaddr_to_string(const sockaddr *addr) {
    char buf[INET6_ADDRSTRLEN + 8];
    if (addr->sa_family == AF_INET) {
        sockaddr_in *addr4 = (sockaddr_in *)addr;
        inet_ntop(AF_INET, &addr4->sin_addr, buf, sizeof(buf));
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%d", ntohs(addr4->sin_port));
    } else if (addr->sa_family == AF_INET6) {
        sockaddr_in6 *addr6 = (sockaddr_in6 *)addr;
        buf[0] = '[';
        inet_ntop(AF_INET6, &addr6->sin6_addr, buf + 1, sizeof(buf) - 1);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "]:%d", ntohs(addr6->sin6_port));
    } else {
        snprintf(buf, sizeof(buf), "unknown address family: %d", addr->sa_family);
    }
    return buf;
}

class Client : public boost::intrusive::list_base_hook<> {
public:
    Client(int fd, const sockaddr *addr, socklen_t addrlen) : client_(fd) {
        memcpy(&remote_addr_, addr, addrlen);
        addrlen = sizeof(local_addr_);
        getsockname(fd, (sockaddr *)&local_addr_, &addrlen);
    }

protected:
    void entry(evco::Context *ctx) {
        char buf[1024];
        std::string str_addr;
        str_addr = sockaddr_to_string((sockaddr *)&remote_addr_);

        printf("connected by %s\n", str_addr.c_str());
        while (1) {
            ssize_t nread = client_.read(ctx, buf, sizeof(buf));
            if (nread < 0) {
                return;
            }
            if (nread == 0) {
                printf("connection closed by %s\n", str_addr.c_str());
                return;
            }
            ssize_t remain = nread;
            while (remain > 0) {
                ssize_t nwrite = client_.write(ctx, buf + nread - remain, remain);
                if (nwrite < 0) {
                    return;
                }
                remain -= nwrite;
            }
        }
    }

private:
    sockaddr_storage local_addr_;
    sockaddr_storage remote_addr_;
    evco::File client_;
};

class Server {
public:
    Server(int fd) : listener_(fd) {
    }

protected:
    void entry(evco::Context *ctx) {
        boost::intrusive::list<Client> client_list;
        sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        while (1) {
            int cfd = listener_.accept(ctx, (sockaddr *)&addr, &addrlen);
            if (cfd < 0) {
                // closed
                break;
            }

            auto *client = new evco::Coroutine<Client>(cfd, (sockaddr *)&addr, addrlen);
            client_list.push_back(*client);
            client->set_finish_callback([&client_list, client]() {
                client_list.erase(client_list.iterator_to(*client));
                delete client;
            });

            client->start(ctx->loop);
        }

        // remove all clients from list
        while (!client_list.empty()) {
            auto *client = static_cast<evco::Coroutine<Client> *>(&client_list.front());
            client->interrupt();  // auto remove from list and delete in finish_callback
        }
    }

private:
    evco::File listener_;
};

class KeyboardWatcher {
protected:
    std::string getline(evco::Context *ctx) {
        std::string line;
        while (1) {
            char c;
            ssize_t nread = stdin_.read(ctx, &c, 1);
            if (nread <= 0) {
                return "";
            }
            line += c;
            if (c == '\n') {
                return line;
            }
        }
    }

    void entry(evco::Context *ctx) {
        std::string line;
        while (1) {
            line = getline(ctx);
            if (line.empty()) {
                break;
            }
            line.pop_back();
            if (line.empty()) {
                continue;
            }
            if (line == "quit" || line == "q") {
                break;
            }
            printf("unknown command: %s\n", line.c_str());
        }
        stdin_.release();
    }

private:
    evco::File stdin_{STDIN_FILENO};
};

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    int fd = create_server(6666, AF_INET6);
    evco::Coroutine<Server> server{fd};
    evco::Coroutine<KeyboardWatcher> keyboard_watcher;
    keyboard_watcher.set_finish_callback([&server]() { server.interrupt(); });
    server.start(loop);
    keyboard_watcher.start(loop);
    ev_run(loop, 0);
    ev_loop_destroy(loop);
    return 0;
}
