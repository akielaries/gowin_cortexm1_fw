/*
 * icmp.h — Layer 3½: ICMP echo (ping)
 *
 * ICMP (Internet Control Message Protocol) runs inside IPv4.
 * We handle only Echo Request (type 8) → Echo Reply (type 0), which is
 * everything needed to respond to ping.
 */

#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>

/* Reply to an ICMP Echo Request.
 * req:       raw received frame buffer starting at byte 0 (Ethernet dst).
 * frame_len: total length of the received frame. */
void icmp_echo_reply(const uint8_t *req, uint32_t frame_len);

#endif /* ICMP_H */
