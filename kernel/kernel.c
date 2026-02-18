#include <stdint.h>
#include "kernel.h"


// an array to hold pointers to all the threads managed by the scheduler
static thread_t* threads[MAX_THREADS];
// a counter to keep track of the number of threads that have been created
static uint32_t thread_count = 0;
// this is defined in a header somewhere and incremented in the systick intrpt handler
volatile uint32_t system_time_ms = 0;

thread_t* mkthread(thread_t* wa, size_t size, int prio, void (*func)(void*), void* arg) {
  (void)size; // parameter is not used
  (void)prio; // parameter is not used

  // look for a free slot first
  for (uint32_t i = 0; i < thread_count; i++) {
    if (!threads[i]->active) {
      wa->func      = func;
      wa->arg       = wa;
      wa->wake_time = 0;
      wa->lc        = 0;
      wa->active    = 1;

      threads[i] = wa;
      return wa;
    }
  }

  // check if we have reached the maximum number of threads
  if (thread_count >= MAX_THREADS) {
    return NULL;
  }

  // initialize the thread structure
  wa->func      = func; // set the thread function
  wa->arg       = wa;   // pass the thread structure itself as an argument
  wa->wake_time = 0;    // initialize wake time to 0 so it runs immediately
  wa->lc        = 0;    // initialize line counter to 0 to start at the beginning
  wa->active    = 1;    // mark the thread as active

  // add the new thread to the list of threads
  threads[thread_count++] = wa;

  return wa;
}

void thread_kill(thread_t *t) {
  if (t != NULL) {
    t->active = 0;
  }
}

/**
 * this function continuously iterates through the list of threads in a round-robin fashion.
 * for each thread, it checks if it is active and if its `wake_time` has been reached. if both
 * conditions are met, it calls the thread's function.
 *
 * @note this function never returns. to make err handling better maybe it should... maybe
 * it goes to some system handler...
 */
void kernel_start(void) {
  /* Scheduler loop */
  while (1) {
    // iterate over all created threads
    for (uint32_t i = 0; i < thread_count; i++) {
      thread_t* t = threads[i];

      // check if the thread is active and if it's time for it to run
      if (t->active && system_time_ms >= t->wake_time) {
        // call the thread function, passing the thread structure as an argument
        t->func(t);
      }
    }
  }
}
