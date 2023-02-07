#ifndef __NET_IP_H__
#define __NET_IP_H__

#include <stddef.h>

#define AF_INET  1
#define AF_INET6 2

typedef size_t socklen_t;

enum net_ip_protocol {
    IPPROTO_IP     = 0,
    IPPROTO_ICMP   = 1,
    IPPROTO_IGMP   = 2,
    IPPROTO_IPIP   = 4,
    IPPROTO_TCP    = 6,
    IPPROTO_UDP    = 17,
    IPPROTO_IPV6   = 41,
    IPPROTO_ICMPV6 = 58,
    IPPROTO_RAW    = 255,
};

enum net_ip_protocol_secure {
    IPPROTO_TLS_1_0  = 256,
    IPPROTO_TLS_1_1  = 257,
    IPPROTO_TLS_1_2  = 258,
    IPPROTO_DTLS_1_0 = 272,
    IPPROTO_DTLS_1_2 = 273,
};

enum net_sock_type { SOCK_STREAM = 1, SOCK_DGRAM, SOCK_RAW };

struct sockaddr {
    void *dummy;
};

#endif /* __NET_IP_H__ */
