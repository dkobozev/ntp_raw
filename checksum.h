/* checksum.h
 *
 * Author: Denis Kobozev <d.v.kobozev@gmail.com>
 */

#ifndef _CHECKSUM_H
#define _CHECKSUM_H

uint16_t checksum(uint16_t *, int);
uint16_t udp_checksum(struct udp_header *, void *, struct in_addr,
        struct in_addr, size_t);

#endif /* _CHECKSUM_H */
