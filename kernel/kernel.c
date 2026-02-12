#include <stdint.h>

#include "kernel.h"


/* ========================================================= */
/* ================= MINI COOP KERNEL ====================== */
/* ========================================================= */

static thread_t *threads[MAX_THREADS];
static uint32_t thread_count = 0;

volatile uint32_t system_time_ms = 0;

/* ---- Thread creation ---- */

thread_t *mkthread(thread_t *wa, size_t size, int prio, void (*func)(void *), void *arg) {
  (void)size;
  (void)prio;

  if (thread_count >= MAX_THREADS) {
    return NULL;
  }

  wa->func      = func;
  wa->arg       = wa;
  wa->wake_time = 0;
  wa->lc        = 0;
  wa->active    = 1;

  threads[thread_count++] = wa;

  return wa;
}


/* ---- Scheduler ---- */

void kernel_start(void) {
  /* Scheduler loop */
  while (1) {
    for (uint32_t i = 0; i < thread_count; i++) {
      thread_t *t = threads[i];

      if (t->active && system_time_ms >= t->wake_time) {
        t->func(t);
      }
    }
  }
}
