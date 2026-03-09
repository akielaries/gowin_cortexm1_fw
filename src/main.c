#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"
#include "GOWIN_M1_ddr3.h"

#include "hw.h"

#include <stdint.h>


#define USE_AHB1
#define USE_DDR3

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
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    //dbg_printf("pin1\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(fast_thd, 256);
THREAD_FUNCTION(fast_fn, arg) {
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_2);
    // thread_sleep_ms(2);
  }
}


THREAD_STACK(sfp_thd, 512);
THREAD_FUNCTION(sfp_fn, arg) {
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
    dbg_printf("    ln0[8:0]=0x%03X (%s)  ln1[8:0]=0x%03X (%s)\r\n",
               s0 & 0x1FF, ((s0 & 0x1FF) == 0x1BC) ? "K28.5!" : "no-comma",
               s1 & 0x1FF, ((s1 & 0x1FF) == 0x1BC) ? "K28.5!" : "no-comma");
    thread_sleep_ms(1000);
  }
}


// basic thread that just yields
// some CPU heavy task that will spit out the result every few seconds or
// something related
THREAD_STACK(compute_thd, 512);
THREAD_FUNCTION(compute_fn, arg) {
  while (1) {
    uint32_t iters  = 500000 + (lcg_rand() % 5000000);
    uint32_t result = 0;

    uint32_t t_start = system_time_ms;

    for (uint32_t i = 0; i < iters; i++) {
      result += i * i;
      if ((i % 1000) == 0)
        thread_yield(); // could remove this manual yield
    }

    uint32_t elapsed = system_time_ms - t_start;
    dbg_printf("compute result: %u, took %d ms (%d iters)\r\n",
               result,
               elapsed,
               iters);
  }
}

#ifdef USE_DDR3
void ddr3_pattern_test(void) {
  dbg_printf("\r\n=== DDR3 Pattern Test ===\r\n");

  uint32_t errors = 0;

  // Make buffers static to avoid stack issues
  static uint32_t write_buf[4];
  static uint32_t read_buf[4];

  // Write pattern to 64 different 16-byte blocks (1KB total)
  dbg_printf("Writing pattern to 64 blocks (1KB)...\r\n");
  for (uint32_t block = 0; block < 64; block++) {
    // Fill buffer with pattern
    write_buf[0] = (block * 4) + 0;
    write_buf[1] = (block * 4) + 1;
    write_buf[2] = (block * 4) + 2;
    write_buf[3] = (block * 4) + 3;

    uint32_t addr = block * 16; // 16-byte aligned addresses
    DDR3_Write(DDR3_BASE + addr, write_buf);

    if ((block % 16) == 0) {
      dbg_printf("  Written %d blocks...\r\n", block);
    }
  }

  dbg_printf("Reading back and verifying...\r\n");
  for (uint32_t block = 0; block < 64; block++) {
    uint32_t addr = block * 16;
    DDR3_Read(DDR3_BASE + addr, read_buf);

    // Verify each word in the block
    for (int word = 0; word < 4; word++) {
      uint32_t expected = (block * 4) + word;
      if (read_buf[word] != expected) {
        dbg_printf(
          "  ERROR at addr 0x%08X[%d]: expected 0x%08X, read 0x%08X\r\n",
          DDR3_BASE + addr,
          word,
          expected,
          read_buf[word]);
        errors++;
        if (errors > 10)
          goto done;
      }
    }

    if ((block % 16) == 0) {
      dbg_printf("  Verified %d blocks...\r\n", block);
    }
  }

done:
  if (errors == 0) {
    dbg_printf("PASS: Pattern test succeeded! All 256 words correct.\r\n");
  } else {
    dbg_printf("FAIL: Pattern test failed with %d errors\r\n", errors);
  }

  // Show first 4 blocks (64 bytes = 16 words)
  dbg_printf("\r\nFirst 4 blocks at DDR3_BASE:\r\n");
  for (uint32_t block = 0; block < 4; block++) {
    DDR3_Read(DDR3_BASE + (block * 16), read_buf);
    dbg_printf("  Block %d [0x%08X]: 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
               block,
               DDR3_BASE + (block * 16),
               read_buf[0],
               read_buf[1],
               read_buf[2],
               read_buf[3]);
  }
}

void ddr3_rw_test(void) {
  dbg_printf("DDR3 test (16-byte aligned)\r\n");

  static uint32_t write_data[6][4] = {
    {0x91234567, 0x89abcdef, 0xfedcba98, 0x76543210},
    {0x66666666, 0x88888888, 0xeeeeeeee, 0xffffffff},
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12},
    {13, 14, 15, 16}};

  static uint32_t read_data[6][4];

  unsigned int addr = 0x00;

  dbg_printf("Writing 6 blocks with 16-byte increment...\r\n");
  for (int i = 0; i < 6; i++) {
    DDR3_Write(addr + i * 16, write_data[i]); // Changed to i*16
  }

  dbg_printf("Reading back...\r\n");
  for (int i = 0; i < 6; i++) {
    DDR3_Read(addr + i * 16, read_data[i]); // Changed to i*16
  }

  dbg_printf("Results:\r\n");
  for (int i = 0; i < 6; i++) {
    dbg_printf("Block %d: 0x%08X 0x%08X 0x%08X 0x%08X ",
               i,
               read_data[i][0],
               read_data[i][1],
               read_data[i][2],
               read_data[i][3]);

    // Check if correct
    if (read_data[i][0] == write_data[i][0] &&
        read_data[i][1] == write_data[i][1] &&
        read_data[i][2] == write_data[i][2] &&
        read_data[i][3] == write_data[i][3]) {
      dbg_printf("OK\r\n");
    } else {
      dbg_printf("MISMATCH\r\n");
    }
  }
}

void ddr3_rw_magic_test(void) {
  dbg_printf("\r\n=== DDR3 Magic Value RW Test ===\r\n");

  uint32_t errors = 0;
  static uint32_t write_buf[4];
  static uint32_t read_buf[4];

  // Test bottom of memory
  uint32_t bottom_addr = 0x00000000;
  dbg_printf("Testing bottom of memory: 0x%08X\r\n", bottom_addr);

  // Prepare magic value
  write_buf[0] = 0xDEADBEEF;
  write_buf[1] = 0xCAFEF00D;
  write_buf[2] = 0x12345678;
  write_buf[3] = 0x9ABCDEF0;

  // Write and read back
  DDR3_Write(bottom_addr, write_buf);
  DDR3_Read(bottom_addr, read_buf);

  dbg_printf("  Read at 0x%08X: 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
             bottom_addr, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

  // Verify
  for (int i = 0; i < 4; i++) {
    if (read_buf[i] != write_buf[i]) {
      dbg_printf("  ERROR at 0x%08X[%d]: expected 0x%08X, read 0x%08X\r\n",
                 bottom_addr, i, write_buf[i], read_buf[i]);
      errors++;
    }
  }

  // Test top of memory
  uint32_t top_addr = 0xFFFFFFF0; // Last 16-byte boundary
  dbg_printf("Testing top of memory: 0x%08X\r\n", top_addr);

  // Prepare different magic value
  write_buf[0] = 0xA5A5A5A5;
  write_buf[1] = 0x5A5A5A5A;
  write_buf[2] = 0xFF00FF00;
  write_buf[3] = 0x00FF00FF;

  // Write and read back
  DDR3_Write(top_addr, write_buf);
  DDR3_Read(top_addr, read_buf);

  dbg_printf("  Read at 0x%08X: 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
             top_addr, read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

  // Verify
  for (int i = 0; i < 4; i++) {
    if (read_buf[i] != write_buf[i]) {
      dbg_printf("  ERROR at 0x%08X[%d]: expected 0x%08X, read 0x%08X\r\n",
                 top_addr, i, write_buf[i], read_buf[i]);
      errors++;
    }
  }

  if (errors == 0) {
    dbg_printf("PASS: DDR3 Magic Value RW Test succeeded!\r\n");
  } else {
    dbg_printf("FAIL: DDR3 Magic Value RW Test failed with %d errors.\r\n", errors);
  }
}

void ddr3_size_test(void) {
    dbg_printf("\r\n=== DDR3 Size Test ===\r\n");
    
    static uint32_t write_buf[4];
    static uint32_t read_buf[4];
    
    // Write unique pattern at start
    write_buf[0] = 0xAAAAAAAA;
    write_buf[1] = 0xBBBBBBBB;
    write_buf[2] = 0xCCCCCCCC;
    write_buf[3] = 0xDDDDDDDD;
    DDR3_Write(0x00000000, write_buf);
    
    // Test boundaries to find actual size
    uint32_t test_addrs[] = {
        0x1FFFFFF0,  // 512MB - 16 bytes (actual chip size)
        0x2FFFFFF0,  // 768MB
        0x3FFFFFF0,  // 1GB
        0x7FFFFFF0,  // 2GB
        0xFFFFFFF0   // 4GB
    };
    
    for (int i = 0; i < 5; i++) {
        uint32_t addr = test_addrs[i];
        
        // Write unique pattern
        write_buf[0] = 0x10000000 + i;
        write_buf[1] = 0x20000000 + i;
        write_buf[2] = 0x30000000 + i;
        write_buf[3] = 0x40000000 + i;
        DDR3_Write(addr, write_buf);
        
        // Read back
        DDR3_Read(addr, read_buf);
        
        dbg_printf("Addr 0x%08X (%dMB): ", addr, addr >> 20);
        if (read_buf[0] == write_buf[0]) {
            dbg_printf("OK - Unique data\r\n");
        } else {
            dbg_printf("MIRROR - reads 0x%08X (might wrap to start)\r\n", read_buf[0]);
        }
    }
    
    // Check if 512MB boundary wrapped to start
    DDR3_Read(0x00000000, read_buf);
    dbg_printf("\nAddress 0x00000000 now reads: 0x%08X\r\n", read_buf[0]);
    if (read_buf[0] == 0xAAAAAAAA) {
        dbg_printf("Start unchanged - no wraparound\r\n");
    } else {
        dbg_printf("Start changed - memory aliases/wraps!\r\n");
    }
}
#endif

#ifdef USE_AHB1
void test_ahb1_rw(void) {
    dbg_printf("\r\n=== Testing AHB1 R/W ===\r\n");

    // Try writing/reading via direct pointer access
    volatile uint32_t *ahb_m1 = (volatile uint32_t *)0x80000000;

    dbg_printf("Writing 0xDEADBEEF to 0x80000000...\r\n");
    ahb_m1[0] = 0xDEADBEEF;

    dbg_printf("Reading back from 0x80000000...\r\n");
    uint32_t read_val = ahb_m1[0];
    dbg_printf("Read: 0x%08X\r\n", read_val);

    if (read_val == 0xDEADBEEF) {
        dbg_printf("SUCCESS! AHB window is memory-mapped!\r\n");

        // Try writing a pattern
        dbg_printf("Writing pattern...\r\n");
        for (int i = 0; i < 16; i++) {
            ahb_m1[i] = i * 0x11111111;
        }

        dbg_printf("reading pattern back...\r\n");
        for (int i = 0; i < 16; i++) {
            dbg_printf("  [0x%08X] = 0x%08X\r\n",
                       0x80000000 + (i*4), ahb_m1[i]);
        }
    } else {
        dbg_printf("FAILED: read 0x%08X, not memory-mapped or no DDR3 there\r\n", read_val);
    }
}

void test_ahb1_16kb(void) {
    dbg_printf("\r\n=== Testing AHB1 16KB RAM Boundaries ===\r\n");

    volatile uint32_t *base = (uint32_t *)0x80000000;
    volatile uint32_t *last = (uint32_t *)(0x80000000 + 16384 - 4);   // 0x80003FFC

    // 1. First word
    dbg_printf("First word (0x80000000): ");
    base[0] = 0xDEADBEEF;
    uint32_t val = base[0];
    dbg_printf("write 0xDEADBEEF, read 0x%08X %s\r\n", val,
               (val == 0xDEADBEEF) ? "OK" : "FAIL");

    // 2. Last word
    dbg_printf("Last word  (0x80003FFC): ");
    *last = 0xCAFEBABE;
    val = *last;
    dbg_printf("write 0xCAFEBABE, read 0x%08X %s\r\n", val,
               (val == 0xCAFEBABE) ? "OK" : "FAIL");

    // 3. Middle word (2 KB offset)
    volatile uint32_t *mid = (uint32_t *)(0x80000000 + 2048);   // 0x80000800
    dbg_printf("Middle word (0x80000800): ");
    *mid = 0x12345678;
    val = *mid;
    dbg_printf("write 0x12345678, read 0x%08X %s\r\n", val,
               (val == 0x12345678) ? "OK" : "FAIL");

    // --- Reset base word to a known pattern for byte/halfword tests ---
    base[0] = 0xA5A5A5A5;   // known pattern: 0xA5A5A5A5

    // 4. Byte access at first address
    volatile uint8_t *byte_ptr = (uint8_t *)0x80000000;
    dbg_printf("Byte access at 0x80000000: ");
    byte_ptr[0] = 0xAA;                 // write LSB
    uint8_t bval = byte_ptr[0];
    val = base[0];
    dbg_printf("write 0xAA, read 0x%02X ", bval);
    // Expect LSB = 0xAA, rest unchanged (0xA5A5A5)
    dbg_printf("(word now 0x%08X) %s\r\n", val,
               (bval == 0xAA && val == 0xA5A5A5AA) ? "OK" : "FAIL");

    // 5. Halfword access at first address (after resetting again)
    base[0] = 0xA5A5A5A5;   // reset
    volatile uint16_t *half_ptr = (uint16_t *)0x80000000;
    dbg_printf("Halfword access at 0x80000000: ");
    half_ptr[0] = 0xBEEF;                // write lower halfword
    uint16_t hval = half_ptr[0];
    val = base[0];
    dbg_printf("write 0xBEEF, read 0x%04X ", hval);
    // Expect lower halfword = 0xBEEF, upper half unchanged (0xA5A5)
    dbg_printf("(word now 0x%08X) %s\r\n", val,
               (hval == 0xBEEF && val == 0xA5A5BEEF) ? "OK" : "FAIL");

    // 6. Optional: test byte at the top of the block
    volatile uint8_t *top_byte = (uint8_t *)(0x80000000 + 16384 - 1);
    dbg_printf("Byte at top (0x80003FFF): ");
    *top_byte = 0x55;
    uint8_t tval = *top_byte;
    dbg_printf("write 0x55, read 0x%02X %s\r\n", tval,
               (tval == 0x55) ? "OK" : "FAIL");

    dbg_printf("=== AHB1 16KB test completed ===\r\n");
}
#endif // USE_AHB1

/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system
 * and the threads
 */
int main(void) {
  hw_init();

#ifdef USE_DDR3
  dbg_printf("DDR3 init\r\n");
  uint8_t status = DDR3_Init();
  dbg_printf("DDR3 init status: %d\r\n", status);

  ddr3_rw_test();
  ddr3_rw_magic_test();
  ddr3_size_test();
  // ddr3_pattern_test();
#endif
#ifdef USE_AHB1
  test_ahb1_rw();
  test_ahb1_16kb();
#endif

  // start the scheduler/kernel
  dbg_printf("initializing kernel...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");

  // idle thread with lowest priority
  mkthd_static(idle, idle_fn, sizeof(idle), PRIO_LOW, NULL);
  //mkthd_static(uptime, uptime_fn, sizeof(uptime), PRIO_NORMAL, NULL);

  mkthd_static(blink1_thd, blink1_fn, sizeof(blink1_thd), PRIO_NORMAL, NULL);
  mkthd_static(blink2_thd, blink2_fn, sizeof(blink2_thd), PRIO_NORMAL, NULL);

  mkthd_static(sfp_thd, sfp_fn, sizeof(sfp_thd), PRIO_NORMAL, NULL);
  //mkthd_static(fast_thd, fast_fn, sizeof(fast_thd), PRIO_LOW, NULL);
  //mkthd_static(compute_thd, compute_fn, sizeof(compute_thd), PRIO_LOW, NULL);


  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
