/**********************************************************
 * sockraw-util.c - Various functions for creating packets.
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

#include "protoheaders.h"
#include "sockrawut.h"

/* Calculates the Internet checksum. Taken from R. Stevens' et al. Unix Network
 * Programming Vol. 1
 * Had to modify it a bit though - that book assumes int is 2 bytes long :/
 */
uint16_t csum_calc(uint16_t *addr, int len)
{
    int w_left = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t checksum = 0;
    
    /* Add sequential 16-bit words to a 32-bit accumulator and then fold back
     * the carry bits from top 16 bits into the lower 16 bits.
     */
    while(w_left > 1) {
        sum += *w++;
        w_left -= 2;
    }
    /* Take care of an odd byte if necessary */
    if(w_left == 1) {
        *(uint8_t *) (&checksum) = *(uint8_t *) w;
        sum += checksum;
    }
    
    /* Add high 16 bits to low 16 bits, add carry, truncate to 16 bits */
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    checksum = ~sum;
    
    return checksum;
}

/* Function udpcsum() calculates a checksum for a UDP packet. It takes as its
 * arguments: a pointer to an initialized UDP header, a pointer to the data
 * to be sent inside the packet, source IP address, destination IP address and
 * the size of data in bytes.
 *
 * udpcsum() creates a buffer for the whole packet, copies UDP pseudo header, 
 * UDP header and data into it and calculates a checksum for the whole buffer.
 * When the checksum is calculated, the buffer is discarded.
 */
uint16_t udpcsum(   struct udp_header *udphead, void *data,
                    struct in_addr srcip, struct in_addr destip,
                    size_t data_len)
{
    struct udp_pseudo udppseudo;
    uint8_t *pseudobuf;
    size_t len, udp_len, total_len;
    uint16_t udp_csum;
    
    len = sizeof(struct udp_pseudo);
    udp_len = sizeof(struct udp_header);
    total_len = len + udp_len + data_len;
    
    udppseudo.src_addr = srcip;
    udppseudo.dest_addr = destip;
    udppseudo.zeros = 0;
    udppseudo.protocol = IPPROTO_UDP;
    udppseudo.len = htons(udp_len + data_len);
    
    pseudobuf = (uint8_t *) malloc(total_len);
    memcpy(pseudobuf, &udppseudo, len);
    memcpy(pseudobuf + len, udphead, udp_len);
    memcpy(pseudobuf + len + udp_len, data, data_len);
    
    udp_csum = csum_calc((uint16_t *) pseudobuf, total_len);
    
    free(pseudobuf);
    
    return udp_csum;
}
