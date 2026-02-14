#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

/**
 * this is the main entry point for the application. it initializes the system and the threads,
 * and then starts the cooperative scheduler.
 */

// this thread blinks an led at a fixed rate of 500ms.
static THD_WORKING_AREA(blinker_thread_wa, 256);
static THD_FUNCTION(blinker_thread, arg) {
  thread_t *thread = (thread_t *)arg; // needed for macros
  THD_BEGIN();
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    THD_SLEEP_MS(500); // Fixed time
  }
  THD_END();
}

// this thread blinks another led at a fixed rate of 100ms.
static THD_WORKING_AREA(blinker_thread_wa2, 256);
static THD_FUNCTION(blinker_thread2, arg) {
  thread_t *thread = (thread_t *)arg; // needed for macros
  THD_BEGIN();
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    THD_SLEEP_MS(100); // Fixed time
  }
  THD_END();
}

// This thread prints the system time to the console every second.
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

/* ========================================================= */
/* ========================== MAIN ========================= */
/* ========================================================= */

int main(void) {
  // initialize the system, including the debug uart, gpio, and delay timer.
  SystemInit();
  debug_init();
  gpio_init();
  delay_init();


  dbg_printf("Cooperative scheduler started!\r\n");

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

  // start the scheduler. never returns from here
  kernel_start();
}
