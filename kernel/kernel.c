#include "GOWIN_M1.h"
#include "kernel.h"

#include "debug.h"


static thread_t *threads[MAX_THREADS];
static uint32_t thread_count = 0;

/*
 * these are needed by the PendSV handler, so they can't be static
 */
thread_t *current_thread = 0;

volatile uint32_t system_time_ms = 0;

/* -------------------------------------------------- */
/* internal helpers */
/* -------------------------------------------------- */

static void init_stack(thread_t *t, void (*entry)(void)) {
  uint32_t *sp = (uint32_t *)(t->stack_mem + t->stack_size);
  sp           = (uint32_t *)((uintptr_t)sp & ~7); // 8-byte align

  // decending stack duh
  // Hardware-stacked registers (pushed by CPU automatically on exception return)
  // this is in the cortex M1 4.5 pre-emption section too
  *(--sp) = 0x01000000;      // xPSR
  *(--sp) = (uint32_t)entry; // PC
  *(--sp) = 0xFFFFFFFD;      // LR (EXC_RETURN)
  *(--sp) = 0;               // R12
  *(--sp) = 0;               // R3
  *(--sp) = 0;               // R2
  *(--sp) = 0;               // R1
  *(--sp) = 0;               // R0

  t->sp = sp;
}

/* -------------------------------------------------- */

void kernel_init(void) { thread_count = 0; }

/* -------------------------------------------------- */

thread_t *thread_create(thread_t *t, void (*func)(void), uint8_t *stack, size_t stack_size) {
  if (thread_count >= MAX_THREADS)
    return NULL;

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
  dbg_printf("next thread??\r\n");

  static uint32_t index = 0;

  thread_t *next = current_thread;

  dbg_printf("thd cnt: %d\r\n", thread_count);

  for (uint32_t i = 0; i < thread_count; i++) {
    index       = (index + 1) % thread_count;
    dbg_printf("index: %d\r\n", index);

    thread_t *t = threads[index];

    if (t->state == THREAD_SLEEPING && system_time_ms >= t->wake_time) {
      t->state = THREAD_READY;
    }

    if (t->state == THREAD_READY) {
      return t;
    }
  }
  dbg_printf("fallback\r\n");
  return next; // fallback
}

/* -------------------------------------------------- */

void thread_yield(void) {
  dbg_printf("enter yld\r\n");
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  dbg_printf("exit yld\r\n");
}

/* -------------------------------------------------- */

void thread_sleep_ms(uint32_t ms) {
  current_thread->wake_time = system_time_ms + ms;
  current_thread->state     = THREAD_SLEEPING;
  thread_yield();
}

/* -------------------------------------------------- */
void kernel_start(void) {
    if (thread_count == 0) return;

    current_thread = threads[0];

    __asm volatile(
        "ldr r0, =current_thread  \n" // r0 = &current_thread
        "ldr r1, [r0]             \n" // r1 = current_thread
        "ldr r0, [r1]             \n" // r0 = current_thread->sp
        "msr psp, r0              \n" // PSP = first thread SP
        "movs r0, #2              \n"
        "msr CONTROL, r0          \n" // Switch to PSP, Thread mode
        "isb                      \n"
        "ldr r0, =0xFFFFFFFD      \n" // use r0, not lr
        "mov lr, r0               \n"
        "bx lr                    \n"
    );
    /* Now trigger PendSV to do the first proper context load */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

