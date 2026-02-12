#include "GOWIN_M1.h"

#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

/* ========================================================= */
/* ================= MINI COOP KERNEL ====================== */
/* ========================================================= */

#define MAX_THREADS 8

typedef struct {
  void (*func)(void *);
  void *arg;
  uint32_t wake_time;
  uint16_t lc; // line counter (state)
  uint8_t active;
} thread_t;

static thread_t threads[MAX_THREADS];
static uint32_t thread_count = 0;

volatile uint32_t system_time_ms = 0;


/* ---- Cooperative thread macros ---- */

#define THD_WORKING_AREA(name, size) thread_t name

#define THD_FUNCTION(name, arg) void name(void *arg)

#define THD_BEGIN()                                                                                \
  switch (thread->lc) {                                                                            \
    case 0:

#define THD_END()                                                                                  \
  }                                                                                                \
  thread->lc = 0;

#define THD_SLEEP_MS(ms)                                                                           \
  do {                                                                                             \
    thread->wake_time = system_time_ms + (ms);                                                     \
    thread->lc        = __LINE__;                                                                  \
    return;                                                                                        \
    case __LINE__:;                                                                                \
  } while (0)

/* ---- Thread creation ---- */

thread_t *mkthread(thread_t *wa, size_t size, int prio, void (*func)(void *), void *arg) {
  (void)size;
  (void)prio;

  if (thread_count >= MAX_THREADS)
    return NULL;

  wa->func      = func;
  wa->arg       = arg;
  wa->wake_time = 0;
  wa->lc        = 0;
  wa->active    = 1;

  threads[thread_count++] = *wa;

  return wa;
}

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
  while (1) {
    for (uint32_t i = 0; i < thread_count; i++) {
      thread_t *t = &threads[i];

      if (t->active && system_time_ms >= t->wake_time) {
        t->func(t);
      }
    }
  }
}
