#pragma once

#include <stdint.h>
#include <stddef.h>

/* =================================================================================================
 *
 * This is a simple cooperative (non-preemptive) kernel. It allows for "asynchronous" programming
 * by running multiple "threads" of execution. However, it's important to understand the following:
 *
 *
 * capabilities:
 * - lightweight: very small code size and memory footprint.
 * - portable: written in standard C, with no architecture-specific code.
 * - simple: easy to understand and use.
 *
 *
 * limitations:
 * - cooperative: threads are not preempted. a thread will run until it explicitly yields control.
 *   this means that a long-running thread will block all other threads.
 * - no priorities: all threads are treated equally.
 * - not for real-time: the timing of thread execution is not guaranteed.
 * - stackless: these are not true threads with their own stacks. they are more like state
 *   machines that use a line-counter (`lc`) to save and restore their state. this means that
 *   you cannot use blocking calls or have deep call stacks within a thread.
 *
 *
 * how it works:
 * the kernel uses a simple round-robin scheduler to run the threads. each thread is a function
 * that is called repeatedly by the scheduler. the thread's state is saved in the `thread_t`
 * struct, which includes a "line counter" (`lc`). the `THD_` macros use this line counter to
 * jump to the correct location in the function when it's called again.
 *
 * =================================================================================================
 */

#define MAX_THREADS 8

/**
 * @brief represents a thread in the system.
 *
 * each thread is defined by this structure, which holds all the necessary information for the
 * scheduler to manage it.
 */
typedef struct {
  void (*func)(void*);      // a pointer to the function that implements the thread's behavior.
  void* arg;                // an argument that is passed to the thread function.
  uint32_t wake_time;       // the system time at which the thread should wake up after sleeping.
  uint16_t lc;              // the "line counter" or state of the thread.
  uint8_t active;           // a flag to indicate if the thread is active and should be run by the scheduler.
} thread_t;

// this is a global variable that holds the system time in milliseconds.
// it is incremented by a systick timer interrupt
extern volatile uint32_t system_time_ms;


/* ========================================================= */
/* =========== COOPERATIVE THREAD MACROS =================== */
/* ========================================================= */

// a macro to define a working area for a thread. in this simple kernel, it just defines a
// `thread_t` variable.
#define THD_WORKING_AREA(name, size) thread_t name

// a macro to define a thread function. it's just a standard function definition.
#define THD_FUNCTION(name, arg) void name(void* arg)

// this macro marks the beginning of a thread's execution. it uses a switch statement on the
// thread's line counter (`lc`) to restore the state of the thread.
#define THD_BEGIN()                                                                                \
  switch (thread->lc) {                                                                            \
    case 0:

// this macro marks the end of a thread's execution. it resets the line counter to 0, so the
// thread will start from the beginning the next time it's called.
#define THD_END()                                                                                  \
  }                                                                                                \
  thread->lc = 0;

/**
 * @brief a macro that allows a thread to sleep for a specified number of milliseconds.
 *
 * this is the core of the cooperative multitasking. when a thread calls this macro, it:
 * 1. sets the `wake_time` for the thread.
 * 2. saves the current line number in the thread's `lc`.
 * 3. returns from the function, yielding control to the scheduler.
 *
 * the next time the scheduler runs this thread (after the wake_time has passed), the `THD_BEGIN`
 * macro will use the saved `lc` to jump to the `case __LINE__:` and resume execution from there.
 */
#define THD_SLEEP_MS(ms)                                                                           \
  do {                                                                                             \
    thread->wake_time = system_time_ms + (ms);                                                     \
    thread->lc        = __LINE__;                                                                  \
    return;                                                                                        \
    case __LINE__:;                                                                                \
  } while (0)


/* ---- Thread creation & Scheduler ---- */

/**
 * @brief creates a new thread and adds it to the scheduler.
 *
 * @param wa a pointer to the `thread_t` structure for the new thread.
 * @param size the size of the thread's working area (currently unused).
 * @param prio the priority of the thread (currently unused).
 * @param func a pointer to the function that implements the thread's behavior.
 * @param arg an argument that is passed to the thread function.
 * @return a pointer to the new thread, or NULL if the maximum number of threads has been reached.
 */
thread_t *mkthread(thread_t *wa, size_t size, int prio, void (*func)(void *), void *arg);

void thread_kill(thread_t *t);


/**
 * @brief starts the kernel's scheduler.
 *
 * this function contains the main loop of the scheduler. it continuously iterates through the list
 * of threads and runs them if they are active and not sleeping. this function never returns.
 */
void kernel_start(void);
