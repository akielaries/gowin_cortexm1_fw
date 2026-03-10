/*
 * ethernet_demo.c — ARP + ICMP responder for Tang Mega 138K
 *
 * Makes the FPGA discoverable via nmap/ping by responding to:
 *   - ARP requests targeting MY_IP  (allows the host to resolve the MAC)
 *   - ICMP echo requests            (ping)
 *
 * Uses polling (no Ethernet interrupts) to keep things simple.
 *
 * Configure MY_IP to match your local subnet before building.
 * MY_MAC uses a locally-administered unicast prefix (02:xx).
 */

#include "GOWIN_M1.h"
#include "GOWIN_M1_ethernet.h"
#include "kernel.h"
#include "debug.h"
#include "hw.h"

#include <stdint.h>
#include <string.h>

/* =========================================================================
 * Configuration
 * ========================================================================= */

static const uint8_t MY_MAC[6] = { 0x02, 0x12, 0x34, 0x56, 0x78, 0x9A };
static const uint8_t MY_IP[4]  = { 192, 168, 86, 200 };

#define PHY_ADDR  1   // RTL8211FI-CG PHY address (schematic: 3'b001)

/* =========================================================================
 * Frame buffers
 * ========================================================================= */

static uint8_t rx_buf[1536];
static uint8_t tx_buf[1536];

/* =========================================================================
 * Ethernet frame field offsets
 * ========================================================================= */

/* Ethernet header */
#define ETH_DST     0
#define ETH_SRC     6
#define ETH_TYPE    12

/* ARP payload (starts at byte 14) */
#define ARP_BASE    14
#define ARP_OPER    (ARP_BASE + 6)
#define ARP_SHA     (ARP_BASE + 8)   /* sender hw addr  */
#define ARP_SPA     (ARP_BASE + 14)  /* sender IP addr  */
#define ARP_THA     (ARP_BASE + 18)  /* target hw addr  */
#define ARP_TPA     (ARP_BASE + 24)  /* target IP addr  */
#define ARP_FRAME_LEN 42             /* 14 eth + 28 arp */

/* IPv4 header (starts at byte 14) */
#define IP_BASE     14
#define IP_LEN      (IP_BASE + 2)    /* total length    */
#define IP_PROTO    (IP_BASE + 9)
#define IP_CSUM     (IP_BASE + 10)
#define IP_SRC      (IP_BASE + 12)
#define IP_DST      (IP_BASE + 16)

/* ICMP (starts after 20-byte IP header) */
#define ICMP_BASE   (IP_BASE + 20)
#define ICMP_TYPE   (ICMP_BASE + 0)
#define ICMP_CSUM   (ICMP_BASE + 2)

/* =========================================================================
 * Helpers
 * ========================================================================= */

static inline uint16_t rd16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static inline void wr16(uint8_t *p, uint16_t v) {
    p[0] = v >> 8;
    p[1] = v & 0xFF;
}

/* Standard one's-complement checksum (IP/ICMP) */
static uint16_t inet_checksum(const uint8_t *data, int len) {
    uint32_t sum = 0;
    for (int i = 0; i + 1 < len; i += 2)
        sum += ((uint32_t)data[i] << 8) | data[i + 1];
    if (len & 1)
        sum += (uint32_t)data[len - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

/* =========================================================================
 * MAC init — polling mode, no interrupts
 * ========================================================================= */

static void eth_mac_init(void) {
    Ethernet->ETH_TX_LENGTH = 0;
    Ethernet->ETH_TX_EN     = 0;
    Ethernet->ETH_TX_FAIL   = 0;
    Ethernet->ETH_TX_IE     = 0;
    Ethernet->ETH_RX_IE     = 1;   /* must be 1 for ETH_RX_IS to assert */
    Ethernet->MIIM_IE       = 0;
    Ethernet->MIIM_OP_EN    = 0;
    Ethernet->ETH_MODE      = ETH_FULL_DUPLEX_100M;  /* 0 — default, matches eth_init() */
}

/* =========================================================================
 * MIIM (MDC/MDIO) polling helpers
 * ========================================================================= */

static uint16_t miim_read(uint8_t phy, uint8_t reg) {
    Ethernet->MIIM_IE       = 0x03; /* required: IS won't assert without IE */
    Ethernet->MIIM_OP_MODE  = 0;
    Ethernet->MIIM_PHY_ADDR = phy;
    Ethernet->MIIM_REG_ADDR = reg;
    Ethernet->MIIM_OP_EN    = 1;
    int timeout = 100000;
    while (!(Ethernet->MIIM_IS & MIIM_RD_DATA_VALID_IS) && timeout--)
        ;
    uint16_t val = (uint16_t)Ethernet->MIIM_RD_DATA;
    Ethernet->MIIM_IC = Ethernet->MIIM_IS;
    Ethernet->MIIM_IE = 0;
    return val;
}

static void miim_write_reg(uint8_t phy, uint8_t reg, uint16_t val) {
    Ethernet->MIIM_IE       = 0x03; /* required: IS won't assert without IE */
    Ethernet->MIIM_OP_MODE  = 1;
    Ethernet->MIIM_PHY_ADDR = phy;
    Ethernet->MIIM_REG_ADDR = reg;
    Ethernet->MIIM_WR_DATA  = val;
    Ethernet->MIIM_OP_EN    = 1;
    int timeout = 100000;
    while (!(Ethernet->MIIM_IS & MIIM_OP_END_IS) && timeout--)
        ;
    Ethernet->MIIM_IC = Ethernet->MIIM_IS;
    Ethernet->MIIM_IE = 0;
}

/* =========================================================================
 * PHY bring-up — wait for link, report speed
 * ========================================================================= */

static void phy_init(void) {
    /* RTL8211FI needs ~72 ms after EPHY_RST_N deasserts before MDIO is ready.
     * Wait generously to cover the PHY internal boot sequence. */
    thread_sleep_ms(200);

    dbg_printf("eth: waiting for PHY link...\r\n");

    /* IEEE 802.3 Basic Status Register (reg 1), bit 2 = link status.
     * Read twice — first read of BMSR may return stale latched value. */
    int retries = 50;
    while (retries-- > 0) {
        miim_read(PHY_ADDR, 1);                   /* discard latched copy */
        uint16_t bmsr = miim_read(PHY_ADDR, 1);
        dbg_printf("eth: BMSR=0x%04x\r\n", bmsr);
        /* 0xFFFF / 0x0000 = no PHY response (MDIO floating) — keep waiting */
        if (bmsr == 0xFFFF || bmsr == 0x0000) {
            thread_sleep_ms(50);
            continue;
        }
        if (bmsr & (1 << 2)) {
            /* RTL8211FI PHY-specific status reg 26 (0x1A):
             *   bits[15:14] = speed  10=1G  01=100M  00=10M
             *   bit[13]     = duplex 1=full */
            uint16_t physpec = miim_read(PHY_ADDR, 26);
            dbg_printf("eth: PHY_SPEC=0x%04x\r\n", physpec);
            /* RTL8211FI reg 26: speed at bits[5:4], duplex at bit[3] */
            uint8_t speed  = (physpec >> 4) & 0x3;
            uint8_t duplex = (physpec >> 3) & 0x1;
            if (speed == 2 && duplex) {
                /* 1G RGMII RX has timing issues at 125 MHz — force 100M FD.
                 * BMCR reg 0: bit13=speed100M, bit8=FD, AN disabled. */
                dbg_printf("eth: 1000M detected, forcing 100M FD (1G RGMII timing)\r\n");
                miim_write_reg(PHY_ADDR, 0, 0x2100);
                thread_sleep_ms(500);
                eth_set_mode(ETH_FULL_DUPLEX_100M);
            } else if (speed == 1 && duplex) {
                dbg_printf("eth: 100M FD\r\n");
                eth_set_mode(ETH_FULL_DUPLEX_100M);
            } else {
                dbg_printf("eth: speed=%d duplex=%d\r\n", speed, duplex);
                eth_set_mode(ETH_FULL_DUPLEX_100M);
            }
            return;
        }
        thread_sleep_ms(100);
    }
    dbg_printf("eth: link timeout\r\n");
}

/* =========================================================================
 * Receive — poll ETH_RX_IS, return frame length or 0
 * ========================================================================= */

static uint32_t eth_recv(uint8_t *buf) {
    if (!(Ethernet->ETH_RX_IS & BIT0))
        return 0;

    uint32_t len = Ethernet->ETH_RX_LENGTH;
    uint8_t *src = (uint8_t *)(Ethernet->ETH_RX_DATA);
    for (uint32_t i = 0; i < len; i++)
        buf[i] = src[i];

    Ethernet->ETH_RX_IC = BIT0;
    return len;
}

/* =========================================================================
 * ARP reply
 * ========================================================================= */

static void send_arp_reply(const uint8_t *req) {
    /* Ethernet header */
    memcpy(&tx_buf[ETH_DST],  &req[ARP_SHA], 6); /* dst = requester MAC */
    memcpy(&tx_buf[ETH_SRC],  MY_MAC,        6);
    tx_buf[ETH_TYPE]   = 0x08;
    tx_buf[ETH_TYPE+1] = 0x06;

    /* ARP header */
    tx_buf[ARP_BASE+0] = 0x00; tx_buf[ARP_BASE+1] = 0x01; /* HTYPE: Ethernet */
    tx_buf[ARP_BASE+2] = 0x08; tx_buf[ARP_BASE+3] = 0x00; /* PTYPE: IPv4     */
    tx_buf[ARP_BASE+4] = 6;                                /* HLEN            */
    tx_buf[ARP_BASE+5] = 4;                                /* PLEN            */
    tx_buf[ARP_OPER]   = 0x00; tx_buf[ARP_OPER+1] = 0x02; /* OPER: reply     */

    memcpy(&tx_buf[ARP_SHA], MY_MAC,         6); /* our MAC */
    memcpy(&tx_buf[ARP_SPA], MY_IP,          4); /* our IP  */
    memcpy(&tx_buf[ARP_THA], &req[ARP_SHA],  6); /* requester MAC */
    memcpy(&tx_buf[ARP_TPA], &req[ARP_SPA],  4); /* requester IP  */

    eth_tx(tx_buf, ARP_FRAME_LEN);
    dbg_printf("eth: ARP reply sent\r\n");
}

/* =========================================================================
 * ICMP echo reply
 * ========================================================================= */

static void send_icmp_reply(const uint8_t *req, uint32_t frame_len) {
    memcpy(tx_buf, req, frame_len);

    /* Swap MACs */
    memcpy(&tx_buf[ETH_DST], &req[ETH_SRC], 6);
    memcpy(&tx_buf[ETH_SRC], MY_MAC,        6);

    /* Swap IPs */
    memcpy(&tx_buf[IP_DST], &req[IP_SRC], 4);
    memcpy(&tx_buf[IP_SRC], MY_IP,        4);

    /* Recompute IP header checksum */
    wr16(&tx_buf[IP_CSUM], 0);
    wr16(&tx_buf[IP_CSUM], inet_checksum(&tx_buf[IP_BASE], 20));

    /* ICMP type 0 = echo reply, recompute ICMP checksum */
    tx_buf[ICMP_TYPE] = 0;
    wr16(&tx_buf[ICMP_CSUM], 0);
    uint16_t ip_total = rd16(&req[IP_LEN]);
    uint16_t icmp_len = ip_total - 20;
    wr16(&tx_buf[ICMP_CSUM], inet_checksum(&tx_buf[ICMP_BASE], icmp_len));

    eth_tx(tx_buf, frame_len);
    dbg_printf("eth: ICMP reply sent\r\n");
}

/* =========================================================================
 * Frame dispatcher
 * ========================================================================= */

static void handle_frame(const uint8_t *buf, uint32_t len) {
    if (len < 14) return;

    uint16_t etype = rd16(&buf[ETH_TYPE]);

    if (etype == 0x0806) {
        /* ARP — respond only to requests targeting our IP */
        if (len >= ARP_FRAME_LEN &&
            rd16(&buf[ARP_OPER]) == 0x0001 &&
            memcmp(&buf[ARP_TPA], MY_IP, 4) == 0) {
            send_arp_reply(buf);
        }
    } else if (etype == 0x0800) {
        /* IPv4 — only handle frames addressed to us */
        if (len < IP_BASE + 20) return;
        if (memcmp(&buf[IP_DST], MY_IP, 4) != 0) return;

        if (buf[IP_PROTO] == 0x01 &&                 /* ICMP */
            len >= (uint32_t)(ICMP_BASE + 8) &&
            buf[ICMP_TYPE] == 8) {                   /* echo request */
            send_icmp_reply(buf, len);
        }
    }
}

/* =========================================================================
 * Ethernet thread
 * ========================================================================= */

THREAD_STACK(eth_thd, 1024);
/* =========================================================================
 * Gratuitous ARP — announces our MAC/IP on the network at startup.
 * Lets Wireshark / arp -n confirm the TX path is working.
 * ========================================================================= */

static void send_gratuitous_arp(void) {
    memset(&tx_buf[ETH_DST], 0xFF, 6);        /* broadcast */
    memcpy(&tx_buf[ETH_SRC],  MY_MAC, 6);
    tx_buf[ETH_TYPE]   = 0x08;
    tx_buf[ETH_TYPE+1] = 0x06;

    tx_buf[ARP_BASE+0] = 0x00; tx_buf[ARP_BASE+1] = 0x01;
    tx_buf[ARP_BASE+2] = 0x08; tx_buf[ARP_BASE+3] = 0x00;
    tx_buf[ARP_BASE+4] = 6;
    tx_buf[ARP_BASE+5] = 4;
    tx_buf[ARP_OPER]   = 0x00; tx_buf[ARP_OPER+1] = 0x01; /* ARP request */

    memcpy(&tx_buf[ARP_SHA], MY_MAC, 6);
    memcpy(&tx_buf[ARP_SPA], MY_IP,  4);
    memset(&tx_buf[ARP_THA], 0x00,   6);
    memcpy(&tx_buf[ARP_TPA], MY_IP,  4);  /* target = ourselves = gratuitous */

    eth_tx(tx_buf, ARP_FRAME_LEN);

    /* Wait for TX to complete (ETH_TX_EN auto-clears on completion) */
    int t = 100000;
    while (Ethernet->ETH_TX_EN && t--) ;
    dbg_printf("eth: gratuitous ARP  TX_EN=%d TX_IS=0x%x TX_FAIL=0x%x\r\n",
        Ethernet->ETH_TX_EN, Ethernet->ETH_TX_IS, Ethernet->ETH_TX_FAIL);
}

THREAD_FUNCTION(eth_fn, arg) {
    eth_mac_init();
    phy_init();

    dbg_printf("eth: ready  IP=%d.%d.%d.%d  MAC=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
        MY_IP[0],  MY_IP[1],  MY_IP[2],  MY_IP[3],
        MY_MAC[0], MY_MAC[1], MY_MAC[2], MY_MAC[3], MY_MAC[4], MY_MAC[5]);

    /* TX smoke-test: gratuitous ARP so Wireshark/arp can confirm TX path */
    send_gratuitous_arp();

    uint32_t dbg_tick = 0;
    while (1) {
        uint32_t len = eth_recv(rx_buf);
        if (len > 0) {
            dbg_printf("eth: frame len=%d etype=0x%02x%02x\r\n",
                len, rx_buf[12], rx_buf[13]);
            handle_frame(rx_buf, len);
        } else {
            if (++dbg_tick % 100000 == 0)
                dbg_printf("eth: poll RX_IS=0x%x\r\n", Ethernet->ETH_RX_IS);
        }
    }
}

/* =========================================================================
 * Main
 * ========================================================================= */

THREAD_STACK(idle_thd, 256);
THREAD_FUNCTION(idle_fn, arg) {
    while (1) { __WFI(); }
}

int main(void) {
    hw_init();
    kernel_init();

    mkthd_static(eth_thd,  eth_fn,  sizeof(eth_thd),  PRIO_NORMAL, NULL);
    mkthd_static(idle_thd, idle_fn, sizeof(idle_thd),  PRIO_LOW,   NULL);

    kernel_start();
    while (1) { __WFI(); }
    return 0;
}
