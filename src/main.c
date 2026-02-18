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


// dummy generator
static uint32_t prng_state = 0x12345678;
static uint32_t prng_next(void) {
  prng_state = prng_state * 1664525 + 1013904223;
  return prng_state;
}

// HW access
volatile struct sysinfo_regs *sysinfo = (struct sysinfo_regs *) APB_M1;
volatile struct gpio_regs *gpio = (struct gpio_regs *) (APB_M1 + 0x20);

void sysinfo_get_mfg(const volatile struct sysinfo_regs *sysinfo, char *buffer, size_t buffer_size) {
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


// this thread blinks an led at a fixed rate of 500ms
static THD_WORKING_AREA(blinker_thread_wa, 256);
static THD_FUNCTION(blinker_thread, arg) {
  thread_t *thread = (thread_t *)arg; // needed for macros?
  THD_BEGIN();
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    THD_SLEEP_MS(500);
  }
  THD_END();
}

// this thread blinks another led at a fixed rate of 100ms
static THD_WORKING_AREA(blinker_thread_wa2, 256);
static THD_FUNCTION(blinker_thread2, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    THD_SLEEP_MS(100);
  }
  THD_END();
}

// print the uptime to the debug port at 1hz. system_time_ms gets
// incremented in the systick handler
static THD_WORKING_AREA(print_thread_wa, 256);
static THD_FUNCTION(print_thread, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    dbg_printf("uptime thread: %ds\r\n", system_time_ms / 1000);
    THD_SLEEP_MS(1000);
  }
  THD_END();
}

// worker thread that will delete itself
static THD_WORKING_AREA(worker_thread_wa, 256);
static THD_FUNCTION(worker_thread, arg) {
  thread_t *thread = (thread_t*)arg;
  THD_BEGIN();

  uint32_t id = prng_next() & 0xFFFF;

  dbg_printf("WORKER %d: started\r\n", id);

  // simulate variable workload
  uint32_t work_time = (prng_next() % 2000) + 500;  // 500–2500 ms
  THD_SLEEP_MS(work_time);

  dbg_printf("WORKER %d: finished after %dms\r\n", id, work_time);

  thread_kill(thread);
  return;

  THD_END();
}

// supervisor thread that will spawn the temp thread
static THD_WORKING_AREA(supervisor_wa, 256);
static THD_FUNCTION(supervisor_thread, arg) {
  thread_t *thread = (thread_t*)arg;
  THD_BEGIN();
  while (1) {
    uint32_t interval = (prng_next() % 5000) + 1000; // 1–6 sec

    dbg_printf("SUPERVISOR: next spawn in %dms\r\n", interval);

    THD_SLEEP_MS(interval);

    dbg_printf("SUPERVISOR: spawning worker\r\n");

    mkthread(&worker_thread_wa,
             sizeof(worker_thread_wa),
             0,
             worker_thread,
             NULL);
  }
  THD_END();
}

/* ========================================================= */
/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system and the threads,
 * and then starts the cooperative scheduler.
 */
int main(void) {
  // initialize the system, including the debug uart, gpio, and delay timer.
  SystemInit();
  debug_init();
  gpio_init();
  delay_init();

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

  dbg_printf("creating threads...\r\n");

  /* Create thread */
  mkthread(&blinker_thread_wa,
           sizeof(blinker_thread_wa),
           0,
           blinker_thread,
           NULL);
  mkthread(&blinker_thread_wa2,
           sizeof(blinker_thread_wa2),
           0,
           blinker_thread2,
           NULL);
  mkthread(&print_thread_wa,
           sizeof(print_thread_wa),
           0,
           print_thread,
           NULL);
  mkthread(&supervisor_wa,
         sizeof(supervisor_wa),
         0,
         supervisor_thread,
         NULL);

  // start the scheduler. never returns from here
  kernel_start();
}
