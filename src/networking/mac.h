/*
 * mac.h — Layer 2: Gowin triple-speed MAC wrapper
 *
 * Owns the TX/RX frame buffers and manages the MAC hardware registers.
 * Identity (MY_MAC, MY_IP) is registered once via mac_set_identity() so
 * upper layers (arp, icmp, udp) can retrieve it without knowing anything
 * about the application that configured it.
 */

#ifndef MAC_H
#define MAC_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Shared TX buffer — upper layers (arp, icmp, udp) write a frame into this
 * then call mac_send().  1536 = max Ethernet payload (1500) + header + pad.
 * ------------------------------------------------------------------------- */
extern uint8_t tx_buf[1536];

/* -------------------------------------------------------------------------
 * Identity — set once by the application, read by upper layers
 * ------------------------------------------------------------------------- */
void mac_set_identity(const uint8_t *mac, const uint8_t *ip);
const uint8_t *mac_get_addr(void); /* returns pointer to 6-byte MAC */
const uint8_t *mac_get_ip(void);   /* returns pointer to 4-byte IP  */

/* -------------------------------------------------------------------------
 * MAC control
 * ------------------------------------------------------------------------- */

/* Initialise the Gowin MAC registers and enable ENT_IRQn in the NVIC.
 * Must be called after mac_set_identity() and before any tx/rx. */
void mac_init(void);

/* Transmit the first len bytes of tx_buf. */
void mac_send(uint32_t len);

/* Check for a received frame.  Copies into buf, returns frame length or 0.
 * Non-blocking — returns 0 immediately if no frame is pending. */
uint32_t mac_recv(uint8_t *buf);

/* Returns 1 if a frame is waiting to be consumed, 0 otherwise.
 * Used by phy_init to test whether RX is actually working after a speed change. */
uint8_t mac_rx_ready(void);

/* -------------------------------------------------------------------------
 * ISR entry points — called from ENT_Handler in GOWIN_M1_it.c
 * ------------------------------------------------------------------------- */

/* Handles ETH_RX_IS: copies the received frame into an internal buffer
 * and sets a pending flag for mac_recv() to pick up. */
void mac_rx_isr(void);

/* Handles ETH_TX_IS: clears the TX interrupt status. */
void mac_tx_isr(void);

/* -------------------------------------------------------------------------
 * Inline helpers — big-endian 16-bit read/write (network byte order)
 * Used by every layer when building or parsing frames.
 * ------------------------------------------------------------------------- */
static inline uint16_t rd16(const uint8_t *p) {
  return ((uint16_t)p[0] << 8) | p[1];
}
static inline void wr16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)(v & 0xFF);
}

#endif /* MAC_H */
