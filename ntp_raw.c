/*
 * ntp_raw.c - NTP client that gets time from an NTP server using raw
 * sockets.
 *
 * Author: Denis Kobozev <d.v.kobozev@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>
#include <unistd.h>

#include "protocol.h"
#include "ntp.h"
#include "ip.h"
#include "checksum.h"

#define NTP_VERSION 3
#define UDP_PORT 32776

void print_time(struct ntpdata *);
void send_request(const char *, const char *);
struct ntpdata get_response(void);
struct ip_header ip_header(const char *, const struct sockaddr_in *, size_t);
struct udp_header udp_header(size_t);
struct ntpdata ntp_request(void);


int main(int argc, char *argv[])
{
    struct ntpdata response;

    /* check the commandline arguments */
    if(argc < 3) {
        fprintf(stderr, "Usage: %s interface ntp_server_url\n", argv[0]);
        fprintf(stderr, "Example: %s eth0 pool.ntp.org\n", argv[0]);
        exit(1);
    }

    send_request(argv[1], argv[2]);
    response = get_response();
    print_time(&response);
    
    return 0;
}

/* Send an empty ntp message to an ntp server.
 *
 * Message is sent from network interface `interface` to server with
 * hostname `hostname`.
 */
void send_request(const char *interface, const char *hostname)
{
    struct hostent *remote_host;
    struct sockaddr_in remote_addr;
    struct ip_header ip_head;
    struct udp_header udp_head;
    struct ntpdata message;
    uint8_t *buffer;
    uint16_t *udp_csum_p, *ip_csum_p;
    size_t ip_len, udp_len, ntp_len, total_len;
    int sockfd;

    /* convert hostname to IP address */
    if((remote_host = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname");
        exit(1);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(123);
    remote_addr.sin_addr = *((struct in_addr *) remote_host->h_addr);
    memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));
    
    ip_len = sizeof(struct ip_header);
    udp_len = sizeof(struct udp_header);
    ntp_len = sizeof(struct ntpdata);
    total_len = ip_len + udp_len + ntp_len;

    ip_head = ip_header(interface, &remote_addr, udp_len + ntp_len);
    udp_head = udp_header(ntp_len);
    message = ntp_request();

    buffer = (uint8_t *) malloc(total_len);
    memcpy(buffer, &ip_head, ip_len);
    memcpy(buffer + ip_len, &udp_head, udp_len);
    memcpy(buffer + ip_len + udp_len, &message, ntp_len);

    /* calculate and set UDP checksum */
    udp_csum_p = (uint16_t *) (buffer + ip_len +
            offsetof(struct udp_header, checksum));
    *udp_csum_p = udp_checksum(&udp_head, &message, ip_head.src_addr,
            ip_head.dest_addr, ntp_len);
    
    /* calculate and set IP checksum */
    ip_csum_p = (uint16_t *) (buffer + offsetof(struct ip_header, checksum));
    *ip_csum_p = checksum((uint16_t *) buffer, total_len);

    /* send the packet */
    if((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket");
        exit(1);
    }
    
    sendto(sockfd, buffer, total_len, 0, (struct sockaddr *) &remote_addr, 
            sizeof(struct sockaddr));
    
    free(buffer);
    close(sockfd);
}

/* create ip header with source address that belongs to network interface
 * `interface`, destination address passed in `remote` and data of size
 * `data_len`.
 */
struct ip_header ip_header(const char *interface, 
        const struct sockaddr_in *remote,
        size_t data_len)
{
    struct in_addr local_ip;
    struct ip_header header;
    size_t len;

    /* get IP address for the interface */
    if (interface_ip(interface, &local_ip) < 0) {
        exit(1);
    }

    len = sizeof(struct ip_header);
    
    header.version = 4;
    header.ihl = len / 4;
    header.tos = 0;
    header.tlength = htons(len + data_len);
    header.id = htons(0xF00); /* bogus id number */
    header.flags_off = 0;
    header.ttl = 0xFF; /* maximum TTL */
    header.protocol = IPPROTO_UDP;
    /* real checksum will be computed when we have the whole datagram */
    header.checksum = 0;
    header.src_addr = local_ip;
    header.dest_addr = remote->sin_addr;

    return header;
}

/* create udp header for data of size `data_len` */
struct udp_header udp_header(size_t data_len)
{
    struct udp_header header;

    header.src_port = htons(UDP_PORT);
    header.dest_port = htons(123);
    header.length = htons(sizeof(struct udp_header) + data_len);
    header.checksum = 0;

    return header;
} 

/* create empty ntp request */
struct ntpdata ntp_request()
{
    struct ntpdata request;

    memset(&request, 0, sizeof(struct ntpdata));
    request.status |= (NTP_VERSION << 3); 
    request.status |= MODE_CLIENT;

    return request;
}

/* receive NTP response from remote server */
struct ntpdata get_response()
{
    struct sockaddr_in local_addr;
    struct ntpdata response;
    int sockfd;
    int bind_ret;

    if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(UDP_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    memset(local_addr.sin_zero, 0, sizeof(local_addr.sin_zero));
    
    bind_ret = bind(sockfd, (struct sockaddr *) &local_addr, 
            sizeof(struct sockaddr));
    if (-1 == bind_ret) {
        perror("bind");
        exit(1);
    }

    recvfrom(sockfd, &response, sizeof(struct ntpdata), 0, NULL, NULL);

    close(sockfd);
    return response;
}

/* format and print time from an ntp server response */
void print_time(struct ntpdata *message)
{
    time_t seconds;

    seconds = ntohl(message->xmt.int_part) - JAN_1970;
    printf("%s", ctime(&seconds));
}
