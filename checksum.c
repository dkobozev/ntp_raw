/* checksum.c: functions for calculating packet checksums.
 *
 * Author: Denis Kobozev <d.v.kobozev@gmail.com>
 */

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
#include "checksum.h"

/* Compute the Internet checksum for data of length `len` pointed to by
 * `addr`.
 *
 * Taken from R. Stevens' et al. "Unix Network Programming Vol. 1".
 * Had to modify it a bit though - that book assumes int is 2 bytes long :/
 * The succint name is not due to character economy, but a means to avoid
 * naming conflicts.
 */
uint16_t checksum(uint16_t *addr, int len)
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

/* Compute checksum for a UDP packet.
 *
 * Params:
 *      `udphead`   - a pointer to an initialized UDP header
 *      `data`      - a pointer to the data to be sent inside the packet
 *      `srcip`     - source IP address
 *      `destip`    - destination IP address
 *      `data_len`  - size of data in bytes
 */
uint16_t udp_checksum(struct udp_header *udphead, void *data,
        struct in_addr srcip, struct in_addr destip, size_t data_len)
{
    struct udp_pseudo pseudo;
    uint8_t *buffer;
    size_t len, udp_len, total_len;
    uint16_t udp_csum;
    
    len = sizeof(struct udp_pseudo);
    udp_len = sizeof(struct udp_header);
    total_len = len + udp_len + data_len;
    
    /* fill UDP pseudo header */
    pseudo.src_addr = srcip;
    pseudo.dest_addr = destip;
    pseudo.zeros = 0;
    pseudo.protocol = IPPROTO_UDP;
    pseudo.len = htons(udp_len + data_len);
    
    /* create a temporary buffer, stuff it with data and compute the checksum
     * for the whole thing
     */
    buffer = (uint8_t *) malloc(total_len);
    memcpy(buffer, &pseudo, len);
    memcpy(buffer + len, udphead, udp_len);
    memcpy(buffer + len + udp_len, data, data_len);
    udp_csum = checksum((uint16_t *) buffer, total_len);
    
    free(buffer);
    
    return udp_csum;
}

