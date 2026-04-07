#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"

#include "mfx.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"
#include "GOWIN_M1_ddr3.h"

#include "hw.h"

#include <stdint.h>

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

THREAD_STACK(mfx_tx_thd, 512);
THREAD_FUNCTION(mfx_tx_fn, arg) {
  static const uint8_t pattern[] = { 0xde, 0xad, 0xbe, 0xef };
  while (1) {
      dbg_printf("tx: %02x %02x %02x %02x\r\n",
                 pattern[0], pattern[1], pattern[2], pattern[3]);
    mfx_send(pattern, sizeof(pattern));
    thread_sleep_ms(1000);
  }
}


#ifdef MEGA_138K
THREAD_STACK(gpio_status_thd, 512);
THREAD_FUNCTION(gpio_status_fn, arg) {
  while (1) {
    dbg_printf("tx0=%d tx1=%d tx2=%d rx0=%d rx1=%d rx2=%d sync=%d\r\n",
      gpio_read(GPIO0, GPIO_Pin_4),
      gpio_read(GPIO0, GPIO_Pin_5),
      gpio_read(GPIO0, GPIO_Pin_6),
      gpio_read(GPIO0, GPIO_Pin_7),
      gpio_read(GPIO0, GPIO_Pin_8),
      gpio_read(GPIO0, GPIO_Pin_9),
      gpio_read(GPIO0, GPIO_Pin_10));
    thread_sleep_ms(200);
  }
}
#endif


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
  dbg_printf("MULTIFLEX: tx/rx/sync/clk driven by RTL\r\n");
#endif
#ifdef MEGA_138K
  dbg_printf("FPGA == MEGA_138K\r\n");
  dbg_printf("GPIO TOGGLEE...\r\n");
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


  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);


  mfx_init();

  mkthd_static(mfx_tx_thd, mfx_tx_fn, sizeof(mfx_tx_thd), PRIO_NORMAL, NULL);

  /***************************************************************************/
  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
