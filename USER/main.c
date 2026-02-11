/*
 ******************************************************************************************
 * @file      main.c
 * @author    GowinSemicoductor
 * @device    Gowin_EMPU_M1
 * @brief     Main function.
 ******************************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include "GOWIN_M1.h"

#include "gpio.h"
#include "delay.h"

#include "sys_defs.h"
#include "GOWIN_M1_uart.h"

void dbg_init(void)
{
	UART_InitTypeDef UART_InitStruct;

	UART_InitStruct.UART_BaudRate = 115200;
	UART_InitStruct.UART_Mode.UARTMode_Tx = ENABLE;
	UART_InitStruct.UART_Mode.UARTMode_Rx = ENABLE;
	UART_InitStruct.UART_Int.UARTInt_Tx = DISABLE;
	UART_InitStruct.UART_Int.UARTInt_Rx = DISABLE;
	UART_InitStruct.UART_Ovr.UARTOvr_Tx = DISABLE;
	UART_InitStruct.UART_Ovr.UARTOvr_Rx = DISABLE;
	UART_InitStruct.UART_Hstm = DISABLE;

	UART_Init(UART1, &UART_InitStruct);
}



//delay
void delay(__IO uint32_t nCount)
{
	for(; nCount != 0; nCount--);
}


/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured,
     this is done through SystemInit() function which is called from startup
     file (startup_GOWIN_M1.s) before to branch to application main.
     To reconfigure the default setting of SystemInit() function, refer to
     system_GOWIN_M1.c file
   */
  SystemInit();	//Initializes system clock
  dbg_init();
  gpio_init();		//Initializes GPIO0
  delay_init();

  UART_SendString(UART1, "Hello from the cortex M1 soft core...\r\n");


	while(1) {
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


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */
