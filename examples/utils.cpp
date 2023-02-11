#include "utils.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

void example_run(evco::Coroutine **coroutines) {
    struct ev_loop *loop = ev_default_loop(0);
    evco::Core core{loop};
    while (*coroutines) {
        (*coroutines)->start(&core);
        ++coroutines;
    }

    ev_run(loop, 0);
    ev_loop_destroy(loop);
}

int create_server(const sockaddr *addr) {
    int fd = socket(addr->sa_family, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    scope_exit fd_cleaner([&] { close(fd); });
    socklen_t addrlen = addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

    if (bind(fd, addr, addrlen) < 0) {
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

int create_client(int family) {
    int fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    return fd;
}

socklen_t sockaddr_len(const sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        return sizeof(sockaddr_in);
    } else if (addr->sa_family == AF_INET6) {
        return sizeof(sockaddr_in6);
    } else {
        return 0;
    }
}

std::string sockaddr_to_string(const sockaddr *addr) {
    char buf[128];
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
        strcpy(buf, "unknown");
    }
    return buf;
}
sockaddr_storage sockaddr_from_string(const char *str) {
    sockaddr_storage invalid = {};
    sockaddr_storage addr = {};
    unsigned long port;
    in_port_t *sock_port;
    void *addr_ptr;
    char str_addr[46];
    const char *tmp;

    invalid.ss_family = AF_UNSPEC;
    // check first char to determine if it's IPv4 or IPv6
    if (str[0] == '[') {
        // IPv6
        sockaddr_in6 *addr6 = (sockaddr_in6 *)&addr;
        sock_port = &addr6->sin6_port;
        addr_ptr = &addr6->sin6_addr;
        addr.ss_family = AF_INET6;

        tmp = strchr(str + 1, ']');
        if (!tmp || tmp[1] != ':') {
            return invalid;
        }

        memcpy(str_addr, str + 1, tmp - str - 1);
        str_addr[tmp - str - 1] = '\0';
        tmp += 2;
    } else {
        // IPv4
        sockaddr_in *addr4 = (sockaddr_in *)&addr;
        sock_port = &addr4->sin_port;
        addr_ptr = &addr4->sin_addr;
        addr.ss_family = AF_INET;

        tmp = strchr(str, ':');
        if (!tmp) {
            return invalid;
        }

        memcpy(str_addr, str, tmp - str);
        str_addr[tmp - str] = '\0';
        tmp += 1;
    }

    if (tmp[0] == '\0') {
        return invalid;
    }

    if (inet_pton(addr.ss_family, str_addr, addr_ptr) != 1) {
        return invalid;
    }

    port = strtoul(tmp, (char **)&tmp, 0);
    if (*tmp != '\0' || port > 65535) {
        return invalid;
    }
    *sock_port = htons(port);
    return addr;
}
