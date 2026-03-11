/*
 * arp.c Layer 2: Address Resolution Protocol
 */

#include "arp.h"
#include "mac.h"
#include "net_defs.h"
#include "GOWIN_M1_ethernet.h"
#include "debug.h"
#include <string.h>

void arp_reply(const uint8_t *req) {
  const uint8_t *my_mac = mac_get_addr();
  const uint8_t *my_ip  = mac_get_ip();

  memcpy(&tx_buf[ETH_DST], &req[ARP_SHA], 6);
  memcpy(&tx_buf[ETH_SRC], my_mac, 6);
  wr16(&tx_buf[ETH_TYPE], ETHERTYPE_ARP);

  wr16(&tx_buf[ARP_BASE + 0], 0x0001); /* HTYPE: Ethernet */
  wr16(&tx_buf[ARP_BASE + 2], 0x0800); /* PTYPE: IPv4     */
  tx_buf[ARP_BASE + 4] = 6;            /* HLEN            */
  tx_buf[ARP_BASE + 5] = 4;            /* PLEN            */
  wr16(&tx_buf[ARP_OPER], 0x0002);     /* reply           */
  memcpy(&tx_buf[ARP_SHA], my_mac, 6);
  memcpy(&tx_buf[ARP_SPA], my_ip, 4);
  memcpy(&tx_buf[ARP_THA], &req[ARP_SHA], 6);
  memcpy(&tx_buf[ARP_TPA], &req[ARP_SPA], 4);

  mac_send(ARP_FRAME_LEN);
  dbg_printf("arp: reply sent\r\n");
}

void arp_announce(void) {
  const uint8_t *my_mac = mac_get_addr();
  const uint8_t *my_ip  = mac_get_ip();

  memset(&tx_buf[ETH_DST], 0xFF, 6);
  memcpy(&tx_buf[ETH_SRC], my_mac, 6);
  wr16(&tx_buf[ETH_TYPE], ETHERTYPE_ARP);

  wr16(&tx_buf[ARP_BASE + 0], 0x0001);
  wr16(&tx_buf[ARP_BASE + 2], 0x0800);
  tx_buf[ARP_BASE + 4] = 6;
  tx_buf[ARP_BASE + 5] = 4;
  wr16(&tx_buf[ARP_OPER], 0x0001); /* request (gratuitous: TPA == SPA) */
  memcpy(&tx_buf[ARP_SHA], my_mac, 6);
  memcpy(&tx_buf[ARP_SPA], my_ip, 4);
  memset(&tx_buf[ARP_THA], 0x00, 6);
  memcpy(&tx_buf[ARP_TPA], my_ip, 4);

  mac_send(ARP_FRAME_LEN);

  int t = 100000;
  while (Ethernet->ETH_TX_EN && t--)
    ;
  dbg_printf("arp: announce  TX_IS=0x%x TX_FAIL=0x%x\r\n",
             Ethernet->ETH_TX_IS,
             Ethernet->ETH_TX_FAIL);
}
