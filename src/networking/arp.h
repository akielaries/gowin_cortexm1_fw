/*
 * arp.h — Layer 2½: Address Resolution Protocol
 *
 * ARP answers the question "who on this LAN has IP address X?"
 * Before any host can send an IPv4 packet to us, it must first broadcast
 * an ARP request.  We respond with our MAC so the host can address frames
 * directly to us.
 */

#ifndef ARP_H
#define ARP_H

#include <stdint.h>

/* Reply to an ARP request targeting our IP.
 * req: raw received frame buffer starting at byte 0 (Ethernet dst). */
void arp_reply(const uint8_t *req);

/* Gratuitous ARP — broadcast our MAC/IP mapping at startup so all devices
 * on the LAN learn us immediately without waiting for someone to ask. */
void arp_announce(void);

#endif /* ARP_H */
