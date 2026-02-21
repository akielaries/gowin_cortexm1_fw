#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

#include "sysinfo_regs.h"
#include "gpio_regs.h"

#include <stdint.h>


#define MFG_ID_MAX_LEN 9

extern uint32_t __StackLimit;
extern uint32_t __StackTop;



// dummy generator
/*
static uint32_t prng_state = 0x12345678;
static uint32_t prng_next(void) {
  prng_state = prng_state * 1664525 + 1013904223;
  return prng_state;
}
*/
// HW access
volatile struct sysinfo_regs *sysinfo = (struct sysinfo_regs *) APB_M1;
volatile struct gpio_regs *gpio = (struct gpio_regs *) (APB_M1 + 0x20);

void sysinfo_get_mfg(const volatile struct sysinfo_regs *sysinfo, char *buffer, size_t buffer_size) {
  if (buffer == NULL || buffer_size < MFG_ID_MAX_LEN || sysinfo == NULL) {
    return;
  }

  uint32_t msb_val = sysinfo->mfg_code_A;
  uint32_t lsb_val = sysinfo->mfg_code_B;

  buffer[0] = (char)((msb_val >> 24) & 0xFF);
  buffer[1] = (char)((msb_val >> 16) & 0xFF);
  buffer[2] = (char)((msb_val >> 8) & 0xFF);
  buffer[3] = (char)((msb_val >> 0) & 0xFF);
  buffer[4] = (char)((lsb_val >> 24) & 0xFF);
  buffer[5] = (char)((lsb_val >> 16) & 0xFF);
  buffer[6] = (char)((lsb_val >> 8) & 0xFF);
  buffer[7] = (char)((lsb_val >> 0) & 0xFF);
  buffer[8] = '\0';
}


#define STACK_SIZE 1024



/* thread objects */
static thread_t blink1_thread;
static thread_t blink2_thread;

/* stacks */
static uint8_t blink1_stack[STACK_SIZE];
static uint8_t blink2_stack[STACK_SIZE];

void blink1(void)
{
  while (1) {
    dbg_printf("pin0\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

void blink2(void)
{
  while (1) {
    dbg_printf("pin1\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

/*
static uint8_t idle_stack[256];
static thread_t idle_thread_obj;
void idle_task(void) {
    while(1) {
        __WFI();  //sleep until next interrupt
    }
}
*/

/* ========================================================= */
/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system and the threads,
 * and then starts the cooperative scheduler.
 */
int main(void) {
  // initialize the system, including the debug uart, gpio, and delay timer.
  SystemInit();
  debug_init();
  gpio_init();
  delay_init();

  dbg_printf("SystemCoreClock: %d mHz\r\n", SystemCoreClock / 1000000);

  char mfg_id_buffer[MFG_ID_MAX_LEN];
  sysinfo_get_mfg(sysinfo, mfg_id_buffer, sizeof(mfg_id_buffer));

  dbg_printf("magic: 0x%X\r\n", sysinfo->magic);
  dbg_printf("mfg_id: %s\r\n", mfg_id_buffer);
  dbg_printf("dev version: 0x%08X\r\n", sysinfo->version);
  dbg_printf("dev version: v%d.%d.%d\r\n",
    (sysinfo->version >> SYSINFO_REGS_VERSION_MAJOR_SHIFT) & 0xFF,
    (sysinfo->version >> SYSINFO_REGS_VERSION_MINOR_SHIFT) & 0xFF,
    (sysinfo->version >> SYSINFO_REGS_VERSION_PATCH_SHIFT) & 0xFF);
  dbg_printf("cheby version: v%d.%d.%d\r\n",
    (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MAJOR_SHIFT) & 0xFF,
    (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MINOR_SHIFT) & 0xFF,
    (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_PATCH_SHIFT) & 0xFF);
  dbg_printf("gpio stat: 0x%08X\r\n", gpio->stat);


  // start the scheduler/kernel
  dbg_printf("stackful kernel starting...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");
  //thread_create(&idle_thread_obj, idle_task, idle_stack, 256);

  thread_create(&blink1_thread,
                blink1,
                blink1_stack,
                STACK_SIZE);
  thread_create(&blink2_thread,
                blink2,
                blink2_stack,
                STACK_SIZE);

  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);
  dbg_printf("SysTick LOAD: 0x%08X\r\n", SysTick->LOAD);
  dbg_printf("SysTick CTRL: 0x%08X\r\n", SysTick->CTRL);

  dbg_printf("blink1_stack: 0x%08X - 0x%08X\r\n",
    (uint32_t)blink1_stack,
    (uint32_t)blink1_stack + STACK_SIZE);
  dbg_printf("__StackLimit: 0x%08X\r\n", (uint32_t)&__StackLimit);
  dbg_printf("__StackTop:   0x%08X\r\n", (uint32_t)&__StackTop);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
