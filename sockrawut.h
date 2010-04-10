/* sockrawut.h - header file for sockraw-util.c packet functions.
 */

uint16_t csum_calc(uint16_t *, int);
uint16_t udpcsum(   struct udp_header *, void *, struct in_addr,
                    struct in_addr, size_t);
