#include "GOWIN_M1.h"

#include "kernel.h"
#include "gpio.h"
#include "delay.h"
#include "debug.h"

#include "sys_defs.h"


int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured,
     this is done through SystemInit() function which is called from startup
     file (startup_GOWIN_M1.s) before to branch to application main.
     To reconfigure the default setting of SystemInit() function, refer to
     system_GOWIN_M1.c file
   */
  SystemInit();	//Initializes system clock
  debug_init();
  gpio_init();		//Initializes GPIO0
  delay_init();

  //UART_SendString(UART1, "Hello from the cortex M1 soft core...\r\n");
  dbg_printf("mini bootloader....\r\n");


	while(1) {
    dbg_printf("uptime: %ds\r\n", system_time_ms / 1000);
		// Create an array of the pins you want to toggle
		const uint32_t pins[] = {
			GPIO_Pin_0,
			GPIO_Pin_1/*,
			GPIO_Pin_2,
			GPIO_Pin_3,
			GPIO_Pin_4,
			GPIO_Pin_5,
			GPIO_Pin_6*/
		};
		const int num_pins = sizeof(pins) / sizeof(pins[0]);


		// Loop forward through the pins
		for (int i = 0; i < num_pins; i++) {
			GPIO_ResetBit(GPIO0, pins[i]);
			delay_ms(100);
			GPIO_SetBit(GPIO0, pins[i]);
		}
	}
}

