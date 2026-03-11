/*
 * ipv4.h — Layer 3: IPv4 helpers
 *
 * The IPv4 header checksum is a ones-complement sum of the header words.
 * It is also reused by ICMP for its own payload checksum (same algorithm).
 */

#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

/* RFC 1071 ones-complement checksum.
 * Pass the header/payload bytes and their length.  Zero the checksum field
 * before calling, then write the returned value into that field. */
uint16_t inet_checksum(const uint8_t *data, int len);

#endif /* IPV4_H */
