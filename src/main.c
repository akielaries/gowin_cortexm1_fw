#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"


/*
 * LED blinker "thread"
 */
static THD_WORKING_AREA(blinker_thread_wa, 256);
static THD_FUNCTION(blinker_thread, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    bool key_pressed = ((GPIO_ReadBits(GPIO0) & GPIO_Pin_2) == 0);
    uint32_t time    = key_pressed ? 250 : 500;

    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);

    THD_SLEEP_MS(time);
  }
  THD_END();
}

// second blinker thread
static THD_WORKING_AREA(blinker_thread_wa2, 256);
static THD_FUNCTION(blinker_thread2, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    bool key_pressed = ((GPIO_ReadBits(GPIO0) & GPIO_Pin_2) == 0);
    uint32_t time    = key_pressed ? 50 : 100;

    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);

    THD_SLEEP_MS(time);
  }
  THD_END();
}

// second blinker thread
static THD_WORKING_AREA(print_thread_wa, 256);
static THD_FUNCTION(print_thread, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    UART_SendString(UART1, "print thread\r\n");

    THD_SLEEP_MS(1000);
  }
  THD_END();
}

/* ========================================================= */
/* ========================== MAIN ========================= */
/* ========================================================= */

int main(void) {
  SystemInit();
  debug_init();
  gpio_init();
  delay_init();


  UART_SendString(UART1, "Cooperative scheduler??\r\n");

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

  /* Scheduler loop */
    UART_SendString(UART1, "systime: \r\n");
  kernel_start();
}
