/*
 * phy.c -- layer 1: RTL8211FI PHY management via MIIM (MDC/MDIO)
 *
 * MIIM is a 2-wire serial bus: MDC clock + MDIO bidirectional data.
 *
 * Gowin MAC quirk: MIIM_IE must be 0x03 before every operation or
 * MIIM_IS never asserts.  The ISR clears it after capturing the result.
 *
 * RTL8211FI bring-up sequence:
 *   1. EPHY_RST_N held low ~21ms by reset stretcher in top.v
 *   2. PHY internal init takes up to 72ms after reset deasserts
 *   3. Autonegotiation takes ~1-3 seconds
 *   4. Poll BMSR (reg 1) until BMSR_LINK_UP sets
 *   5. Read RTL_REG_PHYSPCS (reg 26) for negotiated speed/duplex
 *   6. 1G forced to 100M FD -- RGMII RX has timing issues at 125 MHz
 *      fix: tune RGMII_INPUT_DELAY_VALUE in triple_speed_mac_param.v
 */

#include "phy.h"
#include "mac.h"
#include "GOWIN_M1_ethernet.h"
#include "debug.h"
#include "kernel.h"

static volatile uint8_t  miim_rd_done;
static volatile uint16_t miim_rd_result;
static volatile uint8_t  miim_wr_done;

void phy_miim_rd_isr(void) {
  if (!(Ethernet->MIIM_IS & MIIM_RD_DATA_VALID_IS))
    return;
  miim_rd_result    = (uint16_t)Ethernet->MIIM_RD_DATA;
  Ethernet->MIIM_IC = Ethernet->MIIM_IS;
  Ethernet->MIIM_IE = 0;
  miim_rd_done      = 1;
}

void phy_miim_wr_isr(void) {
  if (!(Ethernet->MIIM_IS & MIIM_OP_END_IS))
    return;
  Ethernet->MIIM_IC = Ethernet->MIIM_IS;
  Ethernet->MIIM_IE = 0;
  miim_wr_done      = 1;
}

uint16_t phy_miim_read(uint8_t phy, uint8_t reg) {
  miim_rd_done = 0;
  miim_receive(phy, reg); /* Gowin BSP: sets IE, OP_MODE=0, fires OP_EN */
  int timeout = 100000;
  while (!miim_rd_done && timeout--) ;
  return miim_rd_result;
}

void phy_miim_write(uint8_t phy, uint8_t reg, uint16_t val) {
  miim_wr_done = 0;
  miim_write(phy, reg, val); /* Gowin BSP: sets IE, OP_MODE=1, fires OP_EN */
  int timeout = 100000;
  while (!miim_wr_done && timeout--) ;
}

static void print_bmsr(uint16_t bmsr) {
  dbg_printf("phy: BMSR=0x%04x\r\n", bmsr);
  dbg_printf("  [14] 100BASE-TX full duplex capable : %d\r\n", !!(bmsr & BMSR_100TX_FD));
  dbg_printf("  [13] 100BASE-TX half duplex capable : %d\r\n", !!(bmsr & BMSR_100TX_HD));
  dbg_printf("  [11] 10BASE-T   full duplex capable : %d\r\n", !!(bmsr & BMSR_10T_FD));
  dbg_printf("  [10] 10BASE-T   half duplex capable : %d\r\n", !!(bmsr & BMSR_10T_HD));
  dbg_printf("  [ 5] autoneg complete               : %d\r\n", !!(bmsr & BMSR_AN_COMPLETE));
  dbg_printf("  [ 3] link partner supports autoneg  : %d\r\n", !!(bmsr & BMSR_AN_CAPABLE));
  dbg_printf("  [ 2] link up                        : %d\r\n", !!(bmsr & BMSR_LINK_UP));
}

static void print_physpcs(uint16_t physpcs) {
  uint16_t speed  = physpcs & PHYSPCS_SPEED_MASK;
  uint8_t  duplex = !!(physpcs & PHYSPCS_DUPLEX_FULL);
  const char *speed_str =
      (speed == PHYSPCS_SPEED_1000) ? "1000M" :
      (speed == PHYSPCS_SPEED_100)  ? "100M"  :
      (speed == PHYSPCS_SPEED_10)   ? "10M"   : "unknown";
  dbg_printf("phy: PHY_SPEC=0x%04x  %s %s\r\n",
      physpcs, speed_str, duplex ? "FD" : "HD");
}

void phy_init(uint8_t phy_addr) {
  /* RTL8211FI needs ~72ms after EPHY_RST_N deasserts before MDIO responds */
  thread_sleep_ms(200);
  dbg_printf("phy: waiting for link...\r\n");

  /* read twice -- first read clears any latched edge bits */
  for (int retries = 50; retries-- > 0;) {
    phy_miim_read(phy_addr, PHY_REG_BMSR);
    uint16_t bmsr = phy_miim_read(phy_addr, PHY_REG_BMSR);

    if (bmsr == 0xFFFF || bmsr == 0x0000) {
      /* MDIO bus floating -- PHY not ready yet */
      thread_sleep_ms(50);
      continue;
    }

    print_bmsr(bmsr);

    if (!(bmsr & BMSR_LINK_UP)) {
      thread_sleep_ms(100);
      continue;
    }

    /* enable RTL8211FI internal RGMII RX delay (~2ns) via paged MDIO.
     * this shifts RXC so RXD is center-aligned at the FPGA input,
     * giving the MAC a clean setup/hold window at 1G (125MHz DDR). */
    phy_miim_write(phy_addr, 31, RTL_PAGE_RGMII_DELAY);
    uint16_t rxdly = phy_miim_read(phy_addr, RTL_REG_RXDLY);
    dbg_printf("phy: RTL_REG_RXDLY before=0x%04x\r\n", rxdly);
    phy_miim_write(phy_addr, RTL_REG_RXDLY, rxdly | RTL_RXDLY_EN);
    rxdly = phy_miim_read(phy_addr, RTL_REG_RXDLY);
    dbg_printf("phy: RTL_REG_RXDLY after =0x%04x\r\n", rxdly);
    phy_miim_write(phy_addr, 31, 0);

    /* force 1G FD unconditionally -- bypass autoneg result */
    dbg_printf("phy: forcing 1G FD via BMCR\r\n");
    phy_miim_write(phy_addr, PHY_REG_BMCR, BMCR_FORCE_1GFD);
    thread_sleep_ms(500);

    /* re-read PHYSPCS after force to confirm what speed the PHY settled on */
    uint16_t physpcs = phy_miim_read(phy_addr, RTL_REG_PHYSPCS);
    print_physpcs(physpcs);

    uint16_t speed = physpcs & PHYSPCS_SPEED_MASK;
    if (speed == PHYSPCS_SPEED_1000) {
      eth_set_mode(ETH_FULL_DUPLEX_1000M);
      dbg_printf("phy: 1G FD -- MAC ready, testing RX...\r\n");

      /* wait up to 5s for any incoming frame to confirm RX is working.
       * at 1G any broadcast on the LAN should arrive within ~1s. */
      int rx_ok = 0;
      for (int i = 0; i < 5 && !rx_ok; i++) {
        dbg_printf("try %d...\r\n", i);
        thread_sleep_ms(1000);
        if (mac_rx_ready()) { 
          rx_ok = 1;
        }
      }

      if (rx_ok) {
        dbg_printf("phy: 1G RX confirmed\r\n");
        return;
      }

      /* no frames received -- RGMII RX timing failure, fall back to 100M */
      dbg_printf("phy: 1G RX dead...falling back to 100M FD\r\n");
      phy_miim_write(phy_addr, PHY_REG_BMCR, BMCR_FORCE_100FD);
      thread_sleep_ms(500);
    }

    eth_set_mode(ETH_FULL_DUPLEX_100M);
    dbg_printf("phy: 100M FD -- MAC ready\r\n");
    return;
  }
  dbg_printf("phy: link timeout\r\n");
}
