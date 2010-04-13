/* print_ip.c: standalone utility that prints out the IP address for a
 * network interface.
 *
 * Author: Denis Kobozev <d.v.kobozev@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ip.h"

int main(int argc, char **argv)
{
    struct in_addr ip;

    if (argc < 2) {
        fprintf(stderr, "Get IP address for a network interface.\n");
        fprintf(stderr, "Usage: %s eth0|wlan0|...\n", argv[0]);
        exit(1);
    }

    if (interface_ip(argv[1], &ip) == 0) {
        printf("%s\tinet addr:%s\n", argv[1], inet_ntoa(ip));
    }

    return 0;
}

