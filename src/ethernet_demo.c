/*
 * ethernet_demo.c — application layer for the Tang Mega 138K ethernet demo
 *
 * This is the only file you need to modify to change how the device behaves
 * on the network.  The networking/ directory underneath is a reusable library
 * — it has no hardcoded addresses or application logic.
 *
 * What this demo does:
 *   - Comes up as 192.168.86.200 / 02:12:34:56:78:9A
 *   - Responds to ARP and ping
 *   - Sends a UDP heartbeat every ~5 seconds to DST_IP:DEBUG_UDP_PORT
 *     payload: 0xDEADBEEF magic header + 128-byte ramp (Wireshark: udp.port ==
 * 9999)
 */

#include "networking/net_defs.h"
#include "networking/mac.h"
#include "networking/phy.h"
#include "networking/arp.h"
#include "networking/ipv4.h"
#include "networking/icmp.h"
#include "networking/udp.h"

#include "kernel.h"
#include "debug.h"
#include "hw.h"
#include <string.h>


/* =============================================================================
 * APPLICATION CONFIGURATION — edit these for your deployment
 * =============================================================================
 */


/* Who we are */
static const uint8_t MY_MAC[6] = {0x02, 0x12, 0x34, 0x56, 0x78, 0x9A};
static const uint8_t MY_IP[4]  = {192, 168, 86, 200};

/* Where to send outgoing debug UDP packets.
 * Run `ip link show` on your laptop to find the right MAC. */
static const uint8_t DST_MAC[6] =
  {0x04, 0x68, 0x74, 0x29, 0x4a, 0x35}; /* wlp1s0 */
static const uint8_t DST_IP[4] = {192, 168, 86, 114};
#define DEBUG_UDP_PORT                                                         \
  9999 /* Wireshark: udp.port == 9999  |  nc -lu 9999 | xxd */

/* PHY bus address — set by strap pins on the board, see schematic (3'b001 = 1)
 */
#define PHY_ADDR 1

/* =============================================================================
 * RECEIVE DISPATCH — what to do with incoming frames
 *
 * Add cases here to handle new protocols.  The networking/ library gives you:
 *   arp_reply(buf)              — send an ARP reply to whoever asked
 *   icmp_echo_reply(buf, len)   — send a ping reply
 *   udp_send(...)               — send a UDP datagram
 * =============================================================================
 */

static void dispatch(const uint8_t *buf, uint32_t len) {
  if (len < 14)
    return;

  switch (rd16(&buf[ETH_TYPE])) {

    case ETHERTYPE_ARP:
      if (len >= ARP_FRAME_LEN && rd16(&buf[ARP_OPER]) == 0x0001 &&
          memcmp(&buf[ARP_TPA], MY_IP, 4) == 0)
        arp_reply(buf);
      break;

    case ETHERTYPE_IPV4:
      if (len < IP_BASE + 20)
        break;
      if (memcmp(&buf[IP_DST], MY_IP, 4) != 0)
        break;

      if (buf[IP_PROTO] == IP_PROTO_ICMP && len >= (uint32_t)(ICMP_BASE + 8) &&
          buf[ICMP_TYPE] == ICMP_ECHO_REQUEST)
        icmp_echo_reply(buf, len);
      break;
  }
}

/* =============================================================================
 * ETHERNET THREAD
 * =============================================================================
 */

static uint8_t rx_buf[1536];

/* Boot payload: 0xDEADBEEF magic header + 128-byte incrementing ramp.
 * Visible in Wireshark at byte offset 42 (after eth+ip+udp headers).
 * Run: nc -lu 9999 | xxd   to see it in the terminal. */
static uint8_t debug_pkt[4 + 128];

static void build_debug_pkt(void) {
  debug_pkt[0] = 0xDE;
  debug_pkt[1] = 0xAD;
  debug_pkt[2] = 0xBE;
  debug_pkt[3] = 0xEF;
  for (int i = 0; i < 128; i++)
    debug_pkt[4 + i] = (uint8_t)i;
}

THREAD_STACK(eth_thd, 1024);
THREAD_FUNCTION(eth_fn, arg) {
  mac_set_identity(MY_MAC, MY_IP);
  mac_init();
  phy_init(PHY_ADDR);

  dbg_printf("eth: up  %d.%d.%d.%d  %02x:%02x:%02x:%02x:%02x:%02x\r\n",
             MY_IP[0],
             MY_IP[1],
             MY_IP[2],
             MY_IP[3],
             MY_MAC[0],
             MY_MAC[1],
             MY_MAC[2],
             MY_MAC[3],
             MY_MAC[4],
             MY_MAC[5]);

  /* Announce our MAC/IP so the network learns us without waiting for a query */
  //arp_announce();

  build_debug_pkt();
  udp_send(DST_MAC,
           DST_IP,
           DEBUG_UDP_PORT,
           DEBUG_UDP_PORT,
           debug_pkt,
           sizeof(debug_pkt));

  uint32_t frame_len = 14 + 20 + 8 + sizeof(debug_pkt);
  uint8_t DBG_TRIES = 1;
  for (int rep = 0; rep < DBG_TRIES; rep++) {
      // ethernet framing?
      dbg_printf("--- tx frame %d (%u bytes) ---\r\n", rep, frame_len);
      dbg_printf("eth  [  0- 5] dst mac : %02x %02x %02x %02x %02x %02x\r\n",
          tx_buf[0],  tx_buf[1],  tx_buf[2],
          tx_buf[3],  tx_buf[4],  tx_buf[5]);
      dbg_printf("eth  [  6-11] src mac : %02x %02x %02x %02x %02x %02x\r\n",
          tx_buf[6],  tx_buf[7],  tx_buf[8],
          tx_buf[9],  tx_buf[10], tx_buf[11]);
      dbg_printf("eth  [ 12-13] ethertype: %02x %02x\r\n",
          tx_buf[12], tx_buf[13]);
      // ipv4?
      dbg_printf("ipv4 [ 14-33] header  :");
      for (int i = 14; i < 34; i++) {
        dbg_printf(" %02x", tx_buf[i]);
      }
      dbg_printf("\r\n");
      dbg_printf("udp  [ 34-41] header  :");
      for (int i = 34; i < 42; i++) {
        dbg_printf(" %02x", tx_buf[i]);
      }

      dbg_printf("\r\n");

      dbg_printf("udp  [ 42+  ] payload (%u bytes):\r\n", frame_len - 42);
      dbg_hexdump(&tx_buf[42], frame_len - 42);
      thread_sleep_ms(2000);
  }

  //while (1) {}

  while (1) {
    uint32_t len = mac_recv(rx_buf);
    if (len > 0) {
      dbg_printf("rx: len=%d etype=0x%04x\r\n", len, rd16(&rx_buf[ETH_TYPE]));
      dispatch(rx_buf, len);
    }

    thread_yield();
  }
}

/* =============================================================================
 * MAIN
 * =============================================================================
 */

THREAD_STACK(idle_thd, 256);
THREAD_FUNCTION(idle_fn, arg) {
  while (1) {
    __WFI();
  }
}

int main(void) {
  hw_init();
  kernel_init();
  mkthd_static(eth_thd, eth_fn, sizeof(eth_thd), PRIO_NORMAL, NULL);
  mkthd_static(idle_thd, idle_fn, sizeof(idle_thd), PRIO_LOW, NULL);
  kernel_start();
  while (1) {
    __WFI();
  }
  return 0;
}
