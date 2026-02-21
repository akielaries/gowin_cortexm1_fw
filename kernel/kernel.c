#include "GOWIN_M1.h"
#include "kernel.h"

#include "debug.h"


static thread_t *threads[MAX_THREADS];
static uint32_t thread_count = 0;

/*
 * these are needed by the PendSV handler, so they can't be static
 */
thread_t *volatile current_thread = 0;


volatile uint32_t system_time_ms = 0;

/* -------------------------------------------------- */
/* internal helpers */
/* -------------------------------------------------- */

static void init_stack(thread_t *t, void (*entry)(void)) {
  uint32_t *sp = (uint32_t *)(t->stack_mem + t->stack_size);
  sp           = (uint32_t *)((uintptr_t)sp & ~7); // 8-byte align

  // hw exception frame (CPU unstacks these on EXC_RETURN)
  *(--sp) = 0x01000000;      // xPSR — Thumb bit set
  *(--sp) = (uint32_t)entry; // PC
  *(--sp) = 0xFFFFFFFD;      // LR (EXC_RETURN: Thread mode, PSP)
  *(--sp) = 0;               // R12
  *(--sp) = 0;               // R3
  *(--sp) = 0;               // R2
  *(--sp) = 0;               // R1
  *(--sp) = 0;               // R0

  // sw frame (PendSV restore pops these before EXC_RETURN)

  *(--sp) = 0; // R7
  *(--sp) = 0; // R6
  *(--sp) = 0; // R5
  *(--sp) = 0; // R4

  *(--sp) = 0; // R11
  *(--sp) = 0; // R10
  *(--sp) = 0; // R9
  *(--sp) = 0; // R8
  t->sp   = sp;
}

/* -------------------------------------------------- */

void kernel_init(void) { thread_count = 0; }

/* -------------------------------------------------- */

thread_t *thread_create(thread_t *t,
                        void (*func)(void),
                        uint8_t *stack,
                        size_t stack_size,
                        uint8_t prio,
                        void *arg) {
  (void)prio;
  (void)arg;

  if (thread_count >= MAX_THREADS) {
    return NULL;
  }

  t->stack_mem  = stack;
  t->stack_size = stack_size;
  t->wake_time  = 0;
  t->state      = THREAD_READY;

  init_stack(t, func);

  threads[thread_count++] = t;
  return t;
}

/* -------------------------------------------------- */

thread_t *scheduler_next(void) {
  static uint32_t index      = 0;
  static uint8_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    index       = thread_count - 1; /* first increment -> index 0 */
  }

  for (uint32_t i = 0; i < thread_count; i++) {
    index       = (index + 1) % thread_count;
    thread_t *t = threads[index];

    if (t->state == THREAD_SLEEPING && system_time_ms >= t->wake_time) {
      // dbg_printf("waking thread, systime=%d wake_time=%d\r\n",
      //      system_time_ms, t->wake_time);

      t->state = THREAD_READY;
    }

    if (t->state == THREAD_READY) {
      return t;
    }
  }
  return NULL; /* all sleeping */
}
/* -------------------------------------------------- */

void thread_yield(void) { SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; }

/* -------------------------------------------------- */

void thread_sleep_ms(uint32_t ms) {
  // dbg_printf("sleep: current_thread=0x%08X\r\n", (uint32_t)current_thread);
  if (current_thread == NULL) {
    dbg_printf("ERROR: current_thread is NULL!\r\n");
    while (1) {
    }
  }

  current_thread->wake_time = system_time_ms + ms;
  current_thread->state     = THREAD_SLEEPING;
  // dbg_printf("systime: %d waketime: %d\r\n", system_time_ms,
  // current_thread->wake_time);
  thread_yield();
}

/* -------------------------------------------------- */
void kernel_start(void) {
  if (thread_count == 0)
    return;

  NVIC_SetPriority(PendSV_IRQn, 0xFF);  // lowest
  NVIC_SetPriority(SysTick_IRQn, 0x00); // highest

  uint32_t first_sp = (uint32_t)threads[0]->sp;

  __asm volatile("msr psp, %0             \n" // PSP = first thread stack
                 "ldr r0, =current_thread \n"
                 "movs r1, #0             \n"
                 "str r1, [r0]            \n" // current_thread = NULL
                 "movs r0, #2             \n"
                 "msr CONTROL, r0         \n"
                 "isb                     \n"
                 "svc #0                  \n" ::"r"(first_sp)
                 : "r0", "r1");
  while (1) {
  }
}
