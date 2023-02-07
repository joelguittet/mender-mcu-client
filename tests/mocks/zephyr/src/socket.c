#include <zephyr/net/socket.h>

int
socket(int family, int type, int proto) {
    return 0;
}

int
connect(int sock, const struct sockaddr *addr, socklen_t addrlen) {
    return 0;
}

int
setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen) {
    return 0;
}

int
getaddrinfo(const char *host, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return 0;
}

void
freeaddrinfo(struct addrinfo *ai) {
}

int
close(int sock) {
    return 0;
}
