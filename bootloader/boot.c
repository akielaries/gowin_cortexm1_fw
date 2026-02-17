#include "GOWIN_M1.h"

#include "kernel.h"
#include "gpio.h"
#include "delay.h"
#include "debug.h"

#include "sys_defs.h"

#include "sysinfo_regs.h"
#include "gpio_regs.h"

#include <stdint.h>


#define MFG_ID_MAX_LEN 9


volatile struct sysinfo_regs *sysinfo = (struct sysinfo_regs *) APB_M1;
volatile struct gpio_regs *gpio = (struct gpio_regs *) (APB_M1 + 0x20);


void sysinfo_get_mfg(const volatile struct sysinfo_regs *sysinfo, char *buffer, size_t buffer_size);

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

// print thread for uptime
static THD_WORKING_AREA(print_thread_wa, 256);
static THD_FUNCTION(print_thd, arg) {
  thread_t *thread = (thread_t *)arg;
  THD_BEGIN();
  while (1) {
    dbg_printf("uptime thread: %ds\r\n", system_time_ms / 1000);
    THD_SLEEP_MS(1000);
  }
  THD_END();
}

// blinky thread
static THD_WORKING_AREA(blinker_thread_wa, 256);
static THD_FUNCTION(blinker_thd, arg) {
  thread_t *thread = (thread_t *)arg; // needed for macros
  THD_BEGIN();
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    THD_SLEEP_MS(100); // Fixed time
  }
  THD_END();
}

int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured,
     this is done through SystemInit() function which is called from startup
     file (startup_GOWIN_M1.s) before to branch to application main.
     To reconfigure the default setting of SystemInit() function, refer to
     system_GOWIN_M1.c file
   */
  //SystemInit();
  debug_init();
  gpio_init();
  delay_init();

  dbg_printf("mini 'bootloader'....\r\n");

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


  /* Create threads */
  mkthread(&blinker_thread_wa,
           sizeof(blinker_thread_wa),
           0,
           blinker_thd,
           NULL);
  mkthread(&print_thread_wa,
           sizeof(print_thread_wa),
           0,
           print_thd,
           NULL);


  // start the scheduler!
  kernel_start();

  return 0;
}

