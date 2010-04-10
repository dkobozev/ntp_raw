/*
 * protoheaders.h - header file containing structs for IP and UDP protocol
 * headers.
 *
 * Author: Denis Kobozev
 */
 
struct ip_header {
    int ihl:4;
    int version:4;
    u_char tos;
    uint16_t tlength;
    uint16_t id;
    uint16_t flags_off;
    u_char ttl;
    u_char protocol;
    uint16_t checksum;
    struct in_addr src_addr;
    struct in_addr dest_addr;
};

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
};

struct udp_pseudo {
    struct in_addr src_addr;
    struct in_addr dest_addr;
    uint8_t zeros;
    uint8_t protocol;
    uint16_t len;
};
