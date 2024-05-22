#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <zephyr/net/net_ip.h>
#include <zephyr/sys/util_macro.h>

#define SOL_TLS          282
#define TLS_SEC_TAG_LIST 1
#define TLS_HOSTNAME     2
#define TLS_PEER_VERIFY  5

struct zsock_addrinfo {
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    socklen_t        ai_addrlen;
    struct sockaddr *ai_addr;
};

int  zsock_socket(int family, int type, int proto);
int  zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen);
int  zsock_setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);
int  zsock_getaddrinfo(const char *host, const char *service, const struct zsock_addrinfo *hints, struct zsock_addrinfo **res);
void zsock_freeaddrinfo(struct zsock_addrinfo *ai);
int  zsock_close(int sock);

#endif /* __SOCKET_H__ */
