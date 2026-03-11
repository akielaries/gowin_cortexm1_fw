/*
 * icmp.c — Layer 3½: ICMP echo (ping)
 *
 * To reply to a ping: copy the whole frame into tx_buf, swap MACs and IPs,
 * change the ICMP type from 8 (request) to 0 (reply), recompute checksums.
 */

#include "icmp.h"
#include "mac.h"
#include "ipv4.h"
#include "net_defs.h"
#include "debug.h"
#include <string.h>

void icmp_echo_reply(const uint8_t *req, uint32_t frame_len) {
  const uint8_t *my_mac = mac_get_addr();
  const uint8_t *my_ip  = mac_get_ip();

  memcpy(tx_buf, req, frame_len);

  /* Layer 2: swap MACs */
  memcpy(&tx_buf[ETH_DST], &req[ETH_SRC], 6);
  memcpy(&tx_buf[ETH_SRC], my_mac, 6);

  /* Layer 3: swap IPs, recompute IPv4 header checksum */
  memcpy(&tx_buf[IP_DST], &req[IP_SRC], 4);
  memcpy(&tx_buf[IP_SRC], my_ip, 4);
  wr16(&tx_buf[IP_CSUM], 0);
  wr16(&tx_buf[IP_CSUM], inet_checksum(&tx_buf[IP_BASE], 20));

  /* ICMP: change type 8 (request) → 0 (reply), recompute ICMP checksum */
  tx_buf[ICMP_TYPE] = ICMP_ECHO_REPLY;
  wr16(&tx_buf[ICMP_CSUM], 0);
  uint16_t icmp_len = rd16(&req[IP_LEN]) - 20;
  wr16(&tx_buf[ICMP_CSUM], inet_checksum(&tx_buf[ICMP_BASE], icmp_len));

  mac_send(frame_len);
  dbg_printf("icmp: echo reply sent\r\n");
}
