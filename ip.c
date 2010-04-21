/* ip.c: Obtain IP address for an network interface.
 *
 * Gleaned from
 * http://www.linuxquestions.org/questions/showthread.php?p=3928347
 * 
 * Author: Denis Kobozev <d.v.kobozev@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>

#include "ip.h"

#define DEBUG 1

/* Copy IP address for `interface` (e.g. "eth0") into `ip` */
int interface_ip(const char *interface, struct in_addr *ip) {
    struct ifreq request;
    struct sockaddr *socket_addr;
    struct sockaddr_in *socket_addr_in;
    struct in_addr *ip_addr;
    int sockfd;

    /* create UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        if (DEBUG) {
            perror("socket");
        }
        fprintf(stderr, "error: Failed to open a socket\n");
        return -1;
    }

    /* make an ioctl request to find this machine's IP */
    request.ifr_addr.sa_family = AF_INET;
    strncpy(request.ifr_name, interface, sizeof(request.ifr_name));
    if (ioctl(sockfd, SIOCGIFADDR, &request) == -1) {
        if (DEBUG) {
            perror("ioctl");
        }
        fprintf(stderr, "error: Interface does not exists\n");
        return -1;
    }

    /* unwrap IP from several layers of structs */
    socket_addr = &request.ifr_addr;
    socket_addr_in = (struct sockaddr_in *) socket_addr;
    ip_addr = &socket_addr_in->sin_addr;
    memcpy(ip, ip_addr, sizeof(struct in_addr));

    return 0;
}

