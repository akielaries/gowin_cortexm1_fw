/*
 * mac.c -- layer 2: gowin triple-speed MAC wrapper
 *
 * RX is interrupt-driven: ENT_Handler calls mac_rx_isr() which copies the
 * frame from hardware into mac_rx_buf and sets eth_rx_pending.  mac_recv()
 * checks that flag instead of polling ETH_RX_IS directly.  The eth thread
 * can therefore call thread_yield() when mac_recv() returns 0 and still be
 * woken on the next 1ms SysTick tick rather than spinning.
 *
 * If a second frame arrives before the first has been consumed it is dropped
 * (ETH_RX_IC cleared, frame discarded).  A queue can be added later.
 *
 * TX is fire-and-forget: mac_send() writes to hardware and returns.
 * mac_tx_isr() just clears the TX interrupt status.
 */

#include "mac.h"
#include "GOWIN_M1_ethernet.h"
#include "GOWIN_M1.h"
#include <string.h>

uint8_t tx_buf[1536];

static uint8_t s_mac[6];
static uint8_t s_ip[4];

static uint8_t mac_rx_buf[1536];
static volatile uint32_t eth_rx_pending_len;
static volatile uint8_t eth_rx_pending;

void mac_set_identity(const uint8_t *mac, const uint8_t *ip) {
  memcpy(s_mac, mac, 6);
  memcpy(s_ip, ip, 4);
}

const uint8_t *mac_get_addr(void) { return s_mac; }
const uint8_t *mac_get_ip(void) { return s_ip; }

void mac_init(void) {
  Ethernet->ETH_TX_LENGTH = 0;
  Ethernet->ETH_TX_EN     = 0;
  Ethernet->ETH_TX_FAIL   = 0;
  Ethernet->ETH_TX_IE     = 1;
  Ethernet->ETH_RX_IE     = 1; /* required: gates ETH_RX_IS */
  Ethernet->MIIM_IE       = 0;
  Ethernet->MIIM_OP_EN    = 0;
  Ethernet->ETH_MODE      = ETH_FULL_DUPLEX_100M;
  NVIC_EnableIRQ(ENT_IRQn);
}

void mac_send(uint32_t len) { eth_tx(tx_buf, len); }

void mac_rx_isr(void) {
  if (!(Ethernet->ETH_RX_IS & BIT0))
    return;

  uint32_t len = Ethernet->ETH_RX_LENGTH;

  if (eth_rx_pending) {
    /* previous frame not yet consumed -- drop this one */
    Ethernet->ETH_RX_IC = BIT0;
    return;
  }

  uint8_t *src = (uint8_t *)(Ethernet->ETH_RX_DATA);
  for (uint32_t i = 0; i < len; i++)
    mac_rx_buf[i] = src[i];

  Ethernet->ETH_RX_IC = BIT0;
  eth_rx_pending_len  = len;
  eth_rx_pending      = 1; /* publish after copy completes */
}

void mac_tx_isr(void) {
  if (Ethernet->ETH_TX_IS & BIT0)
    Ethernet->ETH_TX_IC = BIT0;
}

uint8_t mac_rx_ready(void) { return eth_rx_pending; }

uint32_t mac_recv(uint8_t *buf) {
  if (!eth_rx_pending)
    return 0;
  uint32_t len = eth_rx_pending_len;
  memcpy(buf, mac_rx_buf, len);
  eth_rx_pending = 0; /* re-arm after copy */
  return len;
}
