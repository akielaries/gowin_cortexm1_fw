/*
 * udp.c — Layer 4: UDP transmit
 *
 * Builds a complete Ethernet/IPv4/UDP frame in tx_buf and transmits it.
 * UDP checksum is left zero (omitted) — valid per RFC 768 and accepted
 * by all modern OSes and Wireshark without complaint.
 */

#include "udp.h"
#include "mac.h"
#include "ipv4.h"
#include "net_defs.h"
#include <string.h>

void udp_send(const uint8_t *dst_mac,
              const uint8_t *dst_ip,
              uint16_t src_port,
              uint16_t dst_port,
              const uint8_t *payload,
              uint16_t payload_len) {
  const uint8_t *my_mac = mac_get_addr();
  const uint8_t *my_ip  = mac_get_ip();

  uint16_t udp_len  = 8 + payload_len;
  uint16_t ip_total = 20 + udp_len;

  /* eth frame header */
  memcpy(&tx_buf[ETH_DST], dst_mac, 6);
  memcpy(&tx_buf[ETH_SRC], my_mac, 6);
  wr16(&tx_buf[ETH_TYPE], ETHERTYPE_IPV4);

  /* IPv4 header using the static IP defined by the app (main) */
  tx_buf[IP_BASE + 0] = 0x45; /* version=4, IHL=5 (20 bytes, no options) */
  tx_buf[IP_BASE + 1] = 0x00; /* DSCP/ECN = default                      */
  wr16(&tx_buf[IP_LEN], ip_total);
  wr16(&tx_buf[IP_BASE + 4], 0x0000); /* ID = 0 */
  wr16(&tx_buf[IP_BASE + 6], 0x4000); /* DF bit set, fragment offset = 0 */
  tx_buf[IP_BASE + 8] = 64; /* TTL: drop after 64 hops                 */
  tx_buf[IP_PROTO]    = IP_PROTO_UDP;
  wr16(&tx_buf[IP_CSUM], 0);
  memcpy(&tx_buf[IP_SRC], my_ip, 4);
  memcpy(&tx_buf[IP_DST], dst_ip, 4);
  wr16(&tx_buf[IP_CSUM], inet_checksum(&tx_buf[IP_BASE], 20));

  /* UDP header */
  wr16(&tx_buf[UDP_SRC], src_port);
  wr16(&tx_buf[UDP_DST], dst_port);
  wr16(&tx_buf[UDP_LEN], udp_len);
  wr16(&tx_buf[UDP_CSUM], 0); /* checksum omitted (RFC 768 allows this)   */

  /* payload */
  memcpy(&tx_buf[UDP_DATA], payload, payload_len);

  mac_send(14 + ip_total);
}
