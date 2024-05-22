#include <zephyr/net/socket.h>

int
zsock_socket(int family, int type, int proto) {
    return 0;
}

int
zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen) {
    return 0;
}

int
zsock_setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen) {
    return 0;
}

int
zsock_getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints, struct zsock_addrinfo **res) {
    return 0;
}

void
zsock_freeaddrinfo(struct zsock_addrinfo *ai) {
}

int
zsock_close(int sock) {
    return 0;
}
