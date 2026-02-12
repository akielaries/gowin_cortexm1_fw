#include "delay.h"
#include "GOWIN_M1.h"

/**
  * @brief  Initialize delay function
  * @param  None
  * @retval None
  */
void delay_init(void)
{
  /*
   * Configure the SysTick timer to generate an interrupt every 1 millisecond.
   *
   * With a SystemCoreClock of 50MHz, we need to set the reload value to
   * (50,000,000 / 1,000) - 1 = 49,999.
   */
  SysTick_Config(SystemCoreClock / 1000);
}