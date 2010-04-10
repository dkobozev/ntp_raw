/**********************************************************
 * ntp_client_raw.c - NTP client that gets time from an NTP server using raw
 * sockets.
 *
 * Author: Denis Kobozev
 *********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>

#include <time.h>

#include "protoheaders.h"
#include "ntp.h"

#define NTP_VERSION 3
#define NTP_BUFSIZE 48
#define UDP_PORT 32776

void printntptime(struct ntpdata *, char);

int main(int argc, char *argv[])
{
    struct hostent *h;
    struct sockaddr_in serveraddr, myaddr;
    struct sockaddr *received_from;
    struct ip_header iphead;
    struct udp_header udphead;
    struct ntpdata ntp_message;
    int sockfd, sock_udp;
    int ip_len, udp_len, ntp_len, total_len;
    uint16_t ip_csum, udp_csum;
    uint16_t *udp_csum_ptr, *ip_csum_ptr;
    char my_hostname[255];
    uint8_t *datagrambuf;
    uint8_t ntp_response_buf[NTP_BUFSIZE];
    int size_received;
    
    /* Check the commandline arguments */
    if(argc != 2) {
        fprintf(stderr, "Usage: %s URL\n", argv[0]);
        exit(1);
    }
    
    /* Convert hostname to IP address */
    if((h = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname");
        exit(1);
    }
    
    /* Create a raw socket */
    if((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("socket");
        exit(1);
    }
    
    /* Initialize the server address for sendto and recvfrom calls */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(123);
    serveraddr.sin_addr = *((struct in_addr *) h->h_addr);
    memset(serveraddr.sin_zero, '\0', sizeof(serveraddr.sin_zero));
    
    /* Create IP header.
     *
     * Try looking up IP header spec for field descriptions and values.
     * - Meaningless magic number is used for id
     * - Max TTL value is used for ttl
     * - Checksum is set to 0 for now and computed when we have the whole IP
     * datagram
     */
    ip_len = sizeof(struct ip_header);
    udp_len = sizeof(struct udp_header);
    ntp_len = sizeof(struct ntpdata);
    total_len = ip_len + udp_len + ntp_len;
    
    /* Get this computer's IP address */
    if((gethostname(my_hostname, sizeof(my_hostname))) == -1) {
        perror("gethostname");
        exit(1);
    }
    if((h = gethostbyname(my_hostname)) == NULL) {
        herror("gethostbyname");
        exit(1);
    }
    
    iphead.version = 4;
    iphead.ihl = ip_len / 4;
    iphead.tos = 0;
    iphead.tlength = htons(total_len);
    iphead.id = htons(0xF00);
    iphead.flags_off = 0;
    iphead.ttl = 0xFF;
    iphead.protocol = IPPROTO_UDP;
    iphead.checksum = 0;
    iphead.src_addr = *((struct in_addr *) h->h_addr);
    iphead.dest_addr = serveraddr.sin_addr;
    
    /* Create UDP header */
    udphead.src_port = htons(UDP_PORT);
    udphead.dest_port = htons(123);
    udphead.length = htons(udp_len + ntp_len);
    udphead.checksum = 0;
    
    /* Create NTP message */
    ntp_message.status |= (NTP_VERSION << 3); 
    ntp_message.status |= MODE_CLIENT;
    
    /* Create the whole packet */
    datagrambuf = (uint8_t *) malloc(total_len);
    memcpy(datagrambuf, &iphead, ip_len);
    memcpy(datagrambuf + ip_len, &udphead, udp_len);
    memcpy(datagrambuf + ip_len + udp_len, &ntp_message, ntp_len);
    
    /* Calculate and set UDP checksum. */
    udp_csum = udpcsum(&udphead, &ntp_message, iphead.src_addr, \
            iphead.dest_addr, ntp_len);
    udp_csum_ptr = (uint16_t *) (datagrambuf + ip_len + \
            offsetof(struct udp_header, checksum));
    *udp_csum_ptr = udp_csum;
    
    /* Calculate and set IP checksum */
    ip_csum = csum_calc((uint16_t *) datagrambuf, total_len);
    ip_csum_ptr = (uint16_t *) (datagrambuf + offsetof(struct ip_header, \
            checksum));
    *ip_csum_ptr = ip_csum;
    
    /* Send the packet */
    sendto(sockfd, datagrambuf, total_len, 0, \
            (struct sockaddr *) &serveraddr, sizeof(struct sockaddr));
    
    free(datagrambuf);
    close(sockfd);
    
    /* Receive NTP response */
    if((sock_udp = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(UDP_PORT);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    memset(myaddr.sin_zero, '\0', sizeof(myaddr.sin_zero));
    
    if (bind(sock_udp, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    
    recvfrom(sock_udp, ntp_response_buf, NTP_BUFSIZE, 0, received_from, \
            &size_received);
    
    /* Print time */
    printntptime((struct ntpdata *) &ntp_response_buf, 'n');
    
    close(sock_udp);
    
    return 0;
}

/* printntptime() function extracts and prints time from an NTP response
 * message referenced by the pointer argument. Character argument specifies the
 * order of bytes in the message: 'n' for network, 'h' for host.
 */
void printntptime(struct ntpdata *ntpmessage, char order)
{
    time_t seconds;

    if('n' == order) {
        seconds = ntohl(ntpmessage->xmt.int_part) - JAN_1970;
    } else if('h' == order) {
        seconds = ntpmessage->xmt.int_part - JAN_1970;
    } else {
        printf("printntptime: invalid second argument");
        return;
    }
    
    printf("Time: %s\n", ctime(&seconds));
}
