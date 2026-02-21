#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

#include "sysinfo_regs.h"
#include "gpio_regs.h"

#include <stdint.h>


#define MFG_ID_MAX_LEN 9

extern uint32_t __StackLimit;
extern uint32_t __StackTop;


// dummy generator
static uint32_t lcg_state = 12345;

static uint32_t lcg_rand(void) {
  lcg_state = lcg_state * 1664525 + 1013904223;
  return lcg_state;
}

// HW access
volatile struct sysinfo_regs *sysinfo = (struct sysinfo_regs *)APB_M1;
volatile struct gpio_regs *gpio       = (struct gpio_regs *)(APB_M1 + 0x20);

void sysinfo_get_mfg(const volatile struct sysinfo_regs *sysinfo,
                     char *buffer,
                     size_t buffer_size) {
  if (buffer == NULL || buffer_size < MFG_ID_MAX_LEN || sysinfo == NULL) {
    return;
  }

  uint32_t msb_val = sysinfo->mfg_code_A;
  uint32_t lsb_val = sysinfo->mfg_code_B;

  buffer[0] = (char)((msb_val >> 24) & 0xFF);
  buffer[1] = (char)((msb_val >> 16) & 0xFF);
  buffer[2] = (char)((msb_val >> 8) & 0xFF);
  buffer[3] = (char)((msb_val >> 0) & 0xFF);
  buffer[4] = (char)((lsb_val >> 24) & 0xFF);
  buffer[5] = (char)((lsb_val >> 16) & 0xFF);
  buffer[6] = (char)((lsb_val >> 8) & 0xFF);
  buffer[7] = (char)((lsb_val >> 0) & 0xFF);
  buffer[8] = '\0';
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
    dbg_printf("pin0\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    dbg_printf("pin1\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

// basic thread that just yields
// some CPU heavy task that will spit out the result every few seconds or
// something related
THREAD_STACK(compute_thd, 256);
THREAD_FUNCTION(compute_fn, arg) {
  while (1) {
    uint32_t iters = 5000000 + (lcg_rand() % 5000000); // 5M–10M iters
    uint32_t result = 0;

    uint32_t t_start = system_time_ms;

    for (uint32_t i = 0; i < iters; i++) {
      result += i * i;
      if ((i % 1000) == 0)
        thread_yield();
    }

    uint32_t elapsed = system_time_ms - t_start;
    dbg_printf("compute result: %d, took %d ms (%d iters)\r\n", result, elapsed, iters);
  }
}


/* ========================================================= */
/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system and the threads
 */
int main(void) {
  // initialize the system, including the debug uart, gpio, and delay timer.
  SystemInit();
  debug_init();
  gpio_init();
  delay_init();

  dbg_printf("SystemCoreClock: %d mHz\r\n", SystemCoreClock / 1000000);

  char mfg_id_buffer[MFG_ID_MAX_LEN];
  sysinfo_get_mfg(sysinfo, mfg_id_buffer, sizeof(mfg_id_buffer));

  dbg_printf("magic: 0x%X\r\n", sysinfo->magic);
  dbg_printf("mfg_id: %s\r\n", mfg_id_buffer);
  dbg_printf("dev version: 0x%08X\r\n", sysinfo->version);
  dbg_printf("dev version: v%d.%d.%d\r\n",
             (sysinfo->version >> SYSINFO_REGS_VERSION_MAJOR_SHIFT) & 0xFF,
             (sysinfo->version >> SYSINFO_REGS_VERSION_MINOR_SHIFT) & 0xFF,
             (sysinfo->version >> SYSINFO_REGS_VERSION_PATCH_SHIFT) & 0xFF);
  dbg_printf("cheby version: v%d.%d.%d\r\n",
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MAJOR_SHIFT) & 0xFF,
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MINOR_SHIFT) & 0xFF,
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_PATCH_SHIFT) & 0xFF);
  dbg_printf("gpio stat: 0x%08X\r\n", gpio->stat);


  // start the scheduler/kernel
  dbg_printf("stackful kernel starting...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");
  mkthd_static(uptime, uptime_fn, sizeof(uptime), PRIO_NORMAL, NULL);
  mkthd_static(blink1_thd, blink1_fn, sizeof(blink1_thd), PRIO_NORMAL, NULL);
  mkthd_static(blink2_thd, blink2_fn, sizeof(blink2_thd), PRIO_NORMAL, NULL);
  mkthd_static(compute_thd, compute_fn, sizeof(compute_thd), PRIO_NORMAL, NULL);


  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);
  dbg_printf("SysTick LOAD: 0x%08X\r\n", SysTick->LOAD);
  dbg_printf("SysTick CTRL: 0x%08X\r\n", SysTick->CTRL);
  dbg_printf("__StackLimit: 0x%08X\r\n", (uint32_t)&__StackLimit);
  dbg_printf("__StackTop:   0x%08X\r\n", (uint32_t)&__StackTop);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
