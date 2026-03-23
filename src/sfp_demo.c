#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"
#include "GOWIN_M1_ddr3.h"

#include "hw.h"

#include <stdint.h>


//#define USE_AHB1
//#define USE_DDR3

#define GW5AST_138
//#define GW5AT_60 1

// dummy generator
static uint32_t lcg_state = 12345;

static uint32_t lcg_rand(void) {
  lcg_state = lcg_state * 1664525 + 1013904223;
  return lcg_state;
}

/* ========================================================= */

// IDLE THREAD! this should ideally just be main() i think but idk
THREAD_STACK(idle, 256);
THREAD_FUNCTION(idle_fn, arg) {
  while (1) {
    __WFI();
  }
}

THREAD_STACK(uptime, 512);
THREAD_FUNCTION(uptime_fn, arg) {
  while (1) {
    dbg_printf("uptime: %ds\r\n", system_time_ms / 1000);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(blink1_thd, 512);
THREAD_FUNCTION(blink1_fn, arg) {
  while (1) {
    //dbg_printf("pin0\r\n");
    gpio_toggle(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    //dbg_printf("pin1\r\n");
    gpio_toggle(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(fast_thd, 256);
THREAD_FUNCTION(fast_fn, arg) {
  while (1) {
    gpio_toggle(GPIO0, GPIO_Pin_2);
    // thread_sleep_ms(2);
  }
}


THREAD_STACK(sfp_thd, 512);
THREAD_FUNCTION(sfp_fn, arg) {
  // ---- one-shot tx_pattern loopback demo ----
  // Wait for both lanes to be ready before running the demo.
  dbg_printf("[sfp] waiting for link...\r\n");
  while (1) {
    uint32_t s = sfp->stat;
    uint32_t rdy0 = (s & SFP_REGS_STAT_LN0_READY_MASK) >> SFP_REGS_STAT_LN0_READY_SHIFT;
    uint32_t rdy1 = (s & SFP_REGS_STAT_LN1_READY_MASK) >> SFP_REGS_STAT_LN1_READY_SHIFT;
    if (rdy0 && rdy1) {
      break;
    }
    thread_sleep_ms(200);
  }

  #define DEMO_PATTERN 0xDEADBEEF
  dbg_printf("[sfp] link up...running tx_pattern demo (pattern=0x%08X)\r\n", DEMO_PATTERN);
  sfp->tx_pattern = DEMO_PATTERN;
  sfp->ctrl = SFP_REGS_CTRL_TX_MODE_MASK;   // tx_mode = 1
  thread_sleep_ms(150);                        // let snap registers capture new data

  uint32_t snap0 = sfp->ln0_rx_snap;
  uint32_t snap1 = sfp->ln1_rx_snap;
  dbg_printf("[sfp] snap: ln0=0x%08X  ln1=0x%08X  (expected 0x%08X)\r\n",
             snap0, snap1, DEMO_PATTERN);
  dbg_printf("[sfp] ln0 %s  ln1 %s\r\n",
             (snap0 == DEMO_PATTERN) ? "PASS" : "FAIL",
             (snap1 == DEMO_PATTERN) ? "PASS" : "FAIL");

  sfp->ctrl = 0;   // restore tx_mode = 0 (PRBS7)
  dbg_printf("[sfp] tx_mode restored to PRBS7\r\n");
  // ---- end demo ----

  while (1) {
    uint32_t s  = sfp->stat;
    uint32_t s0 = sfp->ln0_rx_snap;
    uint32_t s1 = sfp->ln1_rx_snap;

    // extract each field using the cheby-generated mask+shift constants
    uint32_t ln0_sig   = (s & SFP_REGS_STAT_LN0_SIGNAL_DETECT_MASK)  >> SFP_REGS_STAT_LN0_SIGNAL_DETECT_SHIFT;
    uint32_t ln0_cdr   = (s & SFP_REGS_STAT_LN0_RX_CDR_LOCK_MASK)    >> SFP_REGS_STAT_LN0_RX_CDR_LOCK_SHIFT;
    uint32_t ln0_klock = (s & SFP_REGS_STAT_LN0_K_LOCK_MASK)         >> SFP_REGS_STAT_LN0_K_LOCK_SHIFT;
    uint32_t ln0_align = (s & SFP_REGS_STAT_LN0_WORD_ALIGN_LINK_MASK) >> SFP_REGS_STAT_LN0_WORD_ALIGN_LINK_SHIFT;
    uint32_t ln0_pll   = (s & SFP_REGS_STAT_LN0_PLL_LOCK_MASK)       >> SFP_REGS_STAT_LN0_PLL_LOCK_SHIFT;
    uint32_t ln0_rdy   = (s & SFP_REGS_STAT_LN0_READY_MASK)          >> SFP_REGS_STAT_LN0_READY_SHIFT;
    uint32_t ln0_prbs  = (s & SFP_REGS_STAT_LN0_PRBS_LOCK_MASK)      >> SFP_REGS_STAT_LN0_PRBS_LOCK_SHIFT;
    uint32_t ln0_rxv   = (s & SFP_REGS_STAT_LN0_RX_VALID_MASK)       >> SFP_REGS_STAT_LN0_RX_VALID_SHIFT;
    uint32_t ln0_rxfe  = (s & SFP_REGS_STAT_LN0_RX_FIFO_EMPTY_MASK)  >> SFP_REGS_STAT_LN0_RX_FIFO_EMPTY_SHIFT;
    uint32_t ln0_txaf  = (s & SFP_REGS_STAT_LN0_TX_FIFO_AFULL_MASK)  >> SFP_REGS_STAT_LN0_TX_FIFO_AFULL_SHIFT;
    uint32_t ln0_txf   = (s & SFP_REGS_STAT_LN0_TX_FIFO_FULL_MASK)   >> SFP_REGS_STAT_LN0_TX_FIFO_FULL_SHIFT;

    uint32_t ln1_sig   = (s & SFP_REGS_STAT_LN1_SIGNAL_DETECT_MASK)  >> SFP_REGS_STAT_LN1_SIGNAL_DETECT_SHIFT;
    uint32_t ln1_cdr   = (s & SFP_REGS_STAT_LN1_RX_CDR_LOCK_MASK)    >> SFP_REGS_STAT_LN1_RX_CDR_LOCK_SHIFT;
    uint32_t ln1_klock = (s & SFP_REGS_STAT_LN1_K_LOCK_MASK)         >> SFP_REGS_STAT_LN1_K_LOCK_SHIFT;
    uint32_t ln1_align = (s & SFP_REGS_STAT_LN1_WORD_ALIGN_LINK_MASK) >> SFP_REGS_STAT_LN1_WORD_ALIGN_LINK_SHIFT;
    uint32_t ln1_pll   = (s & SFP_REGS_STAT_LN1_PLL_LOCK_MASK)       >> SFP_REGS_STAT_LN1_PLL_LOCK_SHIFT;
    uint32_t ln1_rdy   = (s & SFP_REGS_STAT_LN1_READY_MASK)          >> SFP_REGS_STAT_LN1_READY_SHIFT;
    uint32_t ln1_prbs  = (s & SFP_REGS_STAT_LN1_PRBS_LOCK_MASK)      >> SFP_REGS_STAT_LN1_PRBS_LOCK_SHIFT;
    uint32_t ln1_rxv   = (s & SFP_REGS_STAT_LN1_RX_VALID_MASK)       >> SFP_REGS_STAT_LN1_RX_VALID_SHIFT;
    uint32_t ln1_rxfe  = (s & SFP_REGS_STAT_LN1_RX_FIFO_EMPTY_MASK)  >> SFP_REGS_STAT_LN1_RX_FIFO_EMPTY_SHIFT;
    uint32_t ln1_txaf  = (s & SFP_REGS_STAT_LN1_TX_FIFO_AFULL_MASK)  >> SFP_REGS_STAT_LN1_TX_FIFO_AFULL_SHIFT;
    uint32_t ln1_txf   = (s & SFP_REGS_STAT_LN1_TX_FIFO_FULL_MASK)   >> SFP_REGS_STAT_LN1_TX_FIFO_FULL_SHIFT;

    dbg_printf("--- SFP stat: 0x%08X ---\r\n", s);
    dbg_printf("  LN0: sig=%d cdr=%d klock=%d align=%d pll=%d rdy=%d prbs=%d rxv=%d rxfe=%d txaf=%d txf=%d\r\n",
               ln0_sig, ln0_cdr, ln0_klock, ln0_align, ln0_pll, ln0_rdy, ln0_prbs,
               ln0_rxv, ln0_rxfe, ln0_txaf, ln0_txf);
    dbg_printf("  LN1: sig=%d cdr=%d klock=%d align=%d pll=%d rdy=%d prbs=%d rxv=%d rxfe=%d txaf=%d txf=%d\r\n",
               ln1_sig, ln1_cdr, ln1_klock, ln1_align, ln1_pll, ln1_rdy, ln1_prbs,
               ln1_rxv, ln1_rxfe, ln1_txaf, ln1_txf);
    dbg_printf("  snap: ln0=0x%08X  ln1=0x%08X\r\n", s0, s1);
    if ((s0 & 0x1FF) == 0x1BC) {
      dbg_printf("s0 K28.5!\r\n");
    }
    if ((s1 & 0x1FF) == 0x1BC) {
      dbg_printf("s1 K28.5!\r\n");
    }
    thread_sleep_ms(1000);
  }
}



/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system
 * and the threads
 */
int main(void) {
  hw_init();

#ifdef MEGA_60K
  dbg_printf("FPGA == MEGA_60K\r\n");
#endif
#ifdef MEGA_138K
  dbg_printf("FPGA == MEGA_138K\r\n");
#endif

  // start the scheduler/kernel
  dbg_printf("initializing kernel...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");

  // idle thread with lowest priority
  mkthd_static(idle, idle_fn, sizeof(idle), PRIO_LOW, NULL);

  mkthd_static(sfp_thd, sfp_fn, sizeof(sfp_thd), PRIO_NORMAL, NULL);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
