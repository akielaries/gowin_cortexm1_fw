/*
 * phy.h — Layer 1: RTL8211FI PHY management via MIIM (MDC/MDIO)
 *
 * The PHY chip converts between the RJ45 electrical interface (100BASE-TX)
 * and the RGMII digital bus that the FPGA's MAC reads.  We configure and
 * monitor it over the MIIM management bus (MDC clock + MDIO data).
 *
 * phy_init() blocks until the link comes up, then programs the MAC to match
 * the negotiated speed.  PHY_ADDR is the bus address set by strapping pins
 * on the board — pass the value from your schematic.
 */

#ifndef PHY_H
#define PHY_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * IEEE 802.3 standard PHY registers (same on every PHY ever made)
 * ------------------------------------------------------------------------- */
#define PHY_REG_BMCR     0   /* Basic Mode Control Register */
#define PHY_REG_BMSR     1   /* Basic Mode Status Register  */
#define PHY_REG_PHYSID1  2   /* PHY Identifier 1 (OUI high) */
#define PHY_REG_PHYSID2  3   /* PHY Identifier 2 (OUI low + model) */

/* BMCR bits -- what you write to command the PHY */
#define BMCR_RESET       (1 << 15) /* software reset, self-clears       */
#define BMCR_LOOPBACK    (1 << 14) /* loopback mode                     */
#define BMCR_SPEED_100   (1 << 13) /* 1=100M, 0=10M (if AN disabled)    */
#define BMCR_AUTONEG_EN  (1 << 12) /* 1=autonegotiation enabled         */
#define BMCR_POWERDOWN   (1 << 11) /* 1=power down                      */
#define BMCR_ISOLATE     (1 << 10) /* 1=isolate PHY from MII            */
#define BMCR_RESTART_AN  (1 <<  9) /* restart autonegotiation           */
#define BMCR_DUPLEX_FULL (1 <<  8) /* 1=full duplex (if AN disabled)    */
#define BMCR_SPEED_1000  (1 <<  6) /* 1=1G (paired with SPEED_100=0)    */

/* force 100M FD: AN disabled, 100M, full duplex */
#define BMCR_FORCE_100FD (BMCR_SPEED_100 | BMCR_DUPLEX_FULL)
/* force 1G FD:   AN disabled, 1G, full duplex */
#define BMCR_FORCE_1GFD  (BMCR_SPEED_1000 | BMCR_DUPLEX_FULL)

/* BMSR bits -- what you read to see PHY status */
#define BMSR_100TX_FD    (1 << 14) /* capable of 100BASE-TX full duplex */
#define BMSR_100TX_HD    (1 << 13) /* capable of 100BASE-TX half duplex */
#define BMSR_10T_FD      (1 << 11) /* capable of 10BASE-T full duplex   */
#define BMSR_10T_HD      (1 << 10) /* capable of 10BASE-T half duplex   */
#define BMSR_AN_COMPLETE (1 <<  5) /* autonegotiation finished          */
#define BMSR_AN_CAPABLE  (1 <<  3) /* link partner supports autoneg     */
#define BMSR_LINK_UP     (1 <<  2) /* link is up                        */

/* -------------------------------------------------------------------------
 * RTL8211FI vendor-specific registers
 * ------------------------------------------------------------------------- */
#define RTL_REG_PHYSPCS  26  /* PHY-Specific Status (speed/duplex result) */

/* RTL_REG_PHYSPCS bits */
#define PHYSPCS_SPEED_MASK   (0x3 << 4)
#define PHYSPCS_SPEED_10     (0x0 << 4)
#define PHYSPCS_SPEED_100    (0x1 << 4)
#define PHYSPCS_SPEED_1000   (0x2 << 4)
#define PHYSPCS_DUPLEX_FULL  (1   << 3)

/* RTL8211FI RGMII internal delay -- accessed via paged registers.
 *
 * The PHY can add ~2ns of delay to RXC before driving it to the FPGA.
 * With RX delay enabled, RXD is center-aligned on RXC at the FPGA input,
 * giving the MAC a clean setup/hold window at 1G (125 MHz DDR).
 *
 * Access sequence:
 *   phy_miim_write(addr, 31, RTL_PAGE_RGMII_DELAY)  -- select vendor page
 *   val = phy_miim_read(addr, RTL_REG_RXDLY)
 *   phy_miim_write(addr, RTL_REG_RXDLY, val | RTL_RXDLY_EN)
 *   phy_miim_write(addr, 31, 0)                      -- back to page 0
 */
#define RTL_PAGE_RGMII_DELAY  0x0d08  /* vendor page for RGMII delay regs */
#define RTL_REG_TXDLY         0x11    /* TX delay control register         */
#define RTL_REG_RXDLY         0x15    /* RX delay control register         */
#define RTL_TXDLY_EN          (1 << 8)
#define RTL_RXDLY_EN          (1 << 3)

/* -------------------------------------------------------------------------
 * Read/write a PHY register over MIIM.  phy = bus address, reg = register.
 * Both functions start the operation then wait for ENT_Handler to signal
 * completion via phy_miim_rd_isr() / phy_miim_wr_isr(). */
uint16_t phy_miim_read(uint8_t phy, uint8_t reg);
void phy_miim_write(uint8_t phy, uint8_t reg, uint16_t val);

/* Wait for link, detect speed, program MAC ETH_MODE accordingly.
 * phy_addr: MIIM bus address of the PHY (from schematic strap pins). */
void phy_init(uint8_t phy_addr);

/* -------------------------------------------------------------------------
 * ISR entry points -- called from ENT_Handler via mac_ent_handler()
 * ------------------------------------------------------------------------- */
void phy_miim_rd_isr(void); /* handles MIIM_RD_DATA_VALID_IS */
void phy_miim_wr_isr(void); /* handles MIIM_OP_END_IS        */

#endif /* PHY_H */
