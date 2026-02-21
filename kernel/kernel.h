#pragma once

#include <stdint.h>
#include <stddef.h>

#define MAX_THREADS 8

typedef enum {
  THREAD_READY = 0,
  THREAD_SLEEPING,
  THREAD_UNUSED
} thread_state_t;

typedef struct thread {
  uint32_t *sp;            // saved stack pointer

  volatile uint32_t wake_time;
  thread_state_t state;

  uint8_t *stack_mem;
  size_t stack_size;
} thread_t;

/* global system time (incremented elsewhere) */
extern volatile uint32_t system_time_ms;

/*
 * these are needed by the PendSV handler, so they can't be static
 */
extern thread_t *volatile current_thread;
thread_t *scheduler_next(void);

/* kernel lifecycle */
void kernel_init(void);
void kernel_start(void);

/* thread API */
thread_t *thread_create(thread_t *t,
                        void (*func)(void),
                        uint8_t *stack,
                        size_t stack_size);

void thread_yield(void);
void thread_sleep_ms(uint32_t ms);

