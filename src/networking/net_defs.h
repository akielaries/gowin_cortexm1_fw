/*
 * net_defs.h — protocol constants and frame field byte offsets
 *
 * These are defined by IEEE/IETF standards and never change regardless of
 * what device, IP address, or application is using them.  Nothing in here
 * is project-specific — it's just the anatomy of the protocols.
 *
 * Frame byte map (complete layout of every Ethernet/IPv4/UDP frame):
 *
 *   [ 0 ..  5]  dst MAC       ─┐
 *   [ 6 .. 11]  src MAC        ├─ Ethernet header (14 bytes, Layer 2)
 *   [12 .. 13]  EtherType     ─┘
 *   [14 .. 33]  IPv4 header   ─── 20 bytes, Layer 3
 *   [34 .. 41]  UDP header    ─── 8 bytes,  Layer 4
 *   [42 ..   ]  payload       ─── your data
 */

#ifndef NET_DEFS_H
#define NET_DEFS_H

/* -------------------------------------------------------------------------
 * Layer 2 Ethernet header
 * ------------------------------------------------------------------------- */
#define ETH_DST  0  /* bytes  0- 5: destination MAC address */
#define ETH_SRC  6  /* bytes  6-11: source MAC address      */
#define ETH_TYPE 12 /* bytes 12-13: EtherType               */

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPX  0x8137
#define ETHERTYPE_IPV6 0x86DD
#define ETHERTYPE_LOOP 0x9000

/* -------------------------------------------------------------------------
 * Layer 2 ARP (Address Resolution Protocol)
 * Payload starts at byte 14, immediately after the Ethernet header.
 *
 *   HTYPE  hardware type    (1 = Ethernet)
 *   PTYPE  protocol type    (0x0800 = IPv4)
 *   HLEN   hw addr length   (6 for MAC)
 *   PLEN   proto addr length (4 for IPv4)
 *   OPER   1 = request, 2 = reply
 *   SHA    sender hw addr (MAC)
 *   SPA    sender proto addr (IP)
 *   THA    target hw addr (MAC)
 *   TPA    target proto addr (IP)
 * ------------------------------------------------------------------------- */
#define ARP_BASE      14
#define ARP_OPER      (ARP_BASE + 6)
#define ARP_SHA       (ARP_BASE + 8)
#define ARP_SPA       (ARP_BASE + 14)
#define ARP_THA       (ARP_BASE + 18)
#define ARP_TPA       (ARP_BASE + 24)
#define ARP_FRAME_LEN 42 /* 14 eth + 28 arp */

/* -------------------------------------------------------------------------
 * Layer 3 IPv4 header (20 bytes, starts at byte 14)
 *
 *   byte 0    version (4) + IHL (5 = 20-byte header, no options)
 *   byte 1    DSCP/ECN (QoS, usually 0)
 *   bytes 2-3 total length (header + payload)
 *   bytes 4-5 ID (fragmentation, 0 if we set DF)
 *   bytes 6-7 flags + fragment offset (0x4000 = DF bit set)
 *   byte 8    TTL (hop limit)
 *   byte 9    protocol (what is encapsulated: ICMP=0x01, UDP=0x11, TCP=0x06)
 *   bytes 10-11 header checksum
 *   bytes 12-15 source IP
 *   bytes 16-19 destination IP
 * ------------------------------------------------------------------------- */
#define IP_BASE  14
#define IP_LEN   (IP_BASE + 2)
#define IP_PROTO (IP_BASE + 9)
#define IP_CSUM  (IP_BASE + 10)
#define IP_SRC   (IP_BASE + 12)
#define IP_DST   (IP_BASE + 16)

#define IP_PROTO_ICMP 0x01
#define IP_PROTO_UDP  0x11
#define IP_PROTO_TCP  0x06

/* -------------------------------------------------------------------------
 * Layer 3ICMP (Internet Control Message Protocol)
 * Starts at byte 34 (14 eth + 20 ip).
 *
 *   byte 0    type (8 = echo request, 0 = echo reply)
 *   byte 1    code (0 for echo)
 *   bytes 2-3 checksum
 *   bytes 4-5 identifier
 *   bytes 6-7 sequence number
 *   bytes 8+  data (echoed verbatim)
 * ------------------------------------------------------------------------- */
#define ICMP_BASE (IP_BASE + 20)
#define ICMP_TYPE (ICMP_BASE + 0)
#define ICMP_CSUM (ICMP_BASE + 2)

#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

/* -------------------------------------------------------------------------
 * Layer 4 UDP (User Datagram Protocol)
 * Starts at byte 34 (14 eth + 20 ip).
 *
 *   bytes 0-1  source port
 *   bytes 2-3  destination port
 *   bytes 4-5  length (header + payload, minimum 8)
 *   bytes 6-7  checksum (0 = omitted, valid per RFC 768)
 *   bytes 8+   payload (byte 42 in the full frame)
 * ------------------------------------------------------------------------- */
#define UDP_BASE (IP_BASE + 20)
#define UDP_SRC  (UDP_BASE + 0)
#define UDP_DST  (UDP_BASE + 2)
#define UDP_LEN  (UDP_BASE + 4)
#define UDP_CSUM (UDP_BASE + 6)
#define UDP_DATA (UDP_BASE + 8) /* payload starts at byte 42 */

#endif /* NET_DEFS_H */
