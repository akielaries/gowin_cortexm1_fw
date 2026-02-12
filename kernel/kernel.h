#pragma once

#include <stdint.h>
#include <stddef.h>

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

extern volatile uint32_t system_time_ms;


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


/* ---- Thread creation & Scheduler ---- */

thread_t *mkthread(thread_t *wa, size_t size, int prio, void (*func)(void *), void *arg);
void kernel_start(void);
