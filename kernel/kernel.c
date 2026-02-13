#include <stdint.h>
#include "kernel.h"

/* =================================================================================================
 *
 * this file implements the core functionality of the cooperative kernel.
 * it includes the thread creation function and the main scheduler loop.
 *
 * =================================================================================================
 */

// an array to hold pointers to all the threads managed by the scheduler.
static thread_t* threads[MAX_THREADS];
// a counter to keep track of the number of threads that have been created.
static uint32_t thread_count = 0;

// definition of the global system time variable. it is declared as `extern` in `kernel.h`.
volatile uint32_t system_time_ms = 0;

/**
 * @brief initializes a new thread and adds it to the scheduler's list.
 *
 * this function sets up the initial state of a `thread_t` structure and adds it to the `threads`
 * array. the thread is then ready to be run by the scheduler.
 *
 * @note the `size` and `prio` parameters are not currently used in this simple implementation,
 * but are included for future compatibility with more advanced schedulers.
 */
thread_t* mkthread(thread_t* wa, size_t size, int prio, void (*func)(void*), void* arg) {
  (void)size; // parameter is not used
  (void)prio; // parameter is not used

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


/**
 * @brief starts the main scheduler loop.
 *
 * this function continuously iterates through the list of threads in a round-robin fashion.
 * for each thread, it checks if it is active and if its `wake_time` has been reached. if both
 * conditions are met, it calls the thread's function.
 *
 * @note this function never returns. it is the heart of the kernel's execution.
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
