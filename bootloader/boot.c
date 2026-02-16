#include "GOWIN_M1.h"

#include "kernel.h"
#include "gpio.h"
#include "delay.h"
#include "debug.h"

#include "sys_defs.h"

#include <stdint.h>

#define MFG_ID_MAX_LEN 9

#define APB_REGS_BASE_SYSINFO 0x60000000

#define APB_REGS_SYSINFO_VERSION_MAJOR_SHIFT 0
#define APB_REGS_SYSINFO_VERSION_MINOR_SHIFT 8
#define APB_REGS_SYSINFO_VERSION_PATCH_SHIFT 16

typedef struct {
  uint32_t magic;
  uint32_t mfg_msb;
  uint32_t mfg_lsb;
  uint32_t version;
} ahb_regs_sysinfo_t;

volatile ahb_regs_sysinfo_t *sysinfo_regs = (ahb_regs_sysinfo_t *) APB_REGS_BASE_SYSINFO;


void sysinfo_get_mfg(const volatile ahb_regs_sysinfo_t *sysinfo, char *buffer, size_t buffer_size);

void sysinfo_get_mfg(const volatile ahb_regs_sysinfo_t *sysinfo, char *buffer, size_t buffer_size) {
  if (buffer == NULL || buffer_size < MFG_ID_MAX_LEN || sysinfo == NULL) {
    return;
  }

  uint32_t msb_val = sysinfo->mfg_msb;
  uint32_t lsb_val = sysinfo->mfg_lsb;

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

  dbg_printf("mini bootloader....\r\n");

  char mfg_id_buffer[MFG_ID_MAX_LEN];
  sysinfo_get_mfg(sysinfo_regs, mfg_id_buffer, sizeof(mfg_id_buffer));
  dbg_printf("magic: 0x%X\r\n", sysinfo_regs->magic);
  dbg_printf("mfg_id: %s\r\n", mfg_id_buffer);
  dbg_printf("version: 0x%08X\r\n", sysinfo_regs->version);
  dbg_printf("version: v%d.%d.%d\r\n",
    (sysinfo_regs->version >> APB_REGS_SYSINFO_VERSION_MAJOR_SHIFT) & 0xFF,
    (sysinfo_regs->version >> APB_REGS_SYSINFO_VERSION_MINOR_SHIFT) & 0xFF,
    (sysinfo_regs->version >> APB_REGS_SYSINFO_VERSION_PATCH_SHIFT) & 0xFF);


	while(1) {
    dbg_printf("uptime: %ds\r\n", system_time_ms / 1000);
		const uint32_t pins[] = {
			GPIO_Pin_0,
			GPIO_Pin_1,
		};
		const int num_pins = sizeof(pins) / sizeof(pins[0]);

		for (int i = 0; i < num_pins; i++) {
			GPIO_ResetBit(GPIO0, pins[i]);
			delay_ms(1000);
			GPIO_SetBit(GPIO0, pins[i]);
		}
	}
}

