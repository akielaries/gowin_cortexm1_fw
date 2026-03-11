/*
 * udp.h — Layer 4: UDP transmit
 *
 * UDP (User Datagram Protocol) adds source/destination port numbers on top
 * of IPv4.  It is connectionless — no handshake, no acknowledgement.  You
 * fire a datagram and it either arrives or it doesn't.
 *
 * Wireshark filter: udp.port == <dst_port>
 * Receive on host:  nc -lu <dst_port> | xxd
 */

#ifndef UDP_H
#define UDP_H

#include <stdint.h>

/* Send a UDP datagram.
 *
 * dst_mac:     6-byte Ethernet MAC of the destination host
 * dst_ip:      4-byte IPv4 address of the destination host
 * src_port:    our UDP port (appears as source in Wireshark)
 * dst_port:    destination port (what the far-end process listens on)
 * payload:     your data — will appear at byte 42 in the raw frame
 * payload_len: byte count of payload */
void udp_send(const uint8_t *dst_mac,
              const uint8_t *dst_ip,
              uint16_t src_port,
              uint16_t dst_port,
              const uint8_t *payload,
              uint16_t payload_len);

#endif /* UDP_H */
