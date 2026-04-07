#include "gpio.h"
#include "GOWIN_M1.h"


void gpio_init(void) {
  GPIO_InitTypeDef GPIO_InitType;

  GPIO_InitType.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitType.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitType.GPIO_Int = GPIO_Int_Disable;
  GPIO_Init(GPIO0, &GPIO_InitType);

#ifdef MEGA_60K
  // multiflex tx, rx, sync, clk are dedicated top-level rtl ports
  // gpio 4-10 are no longer used by firmware
#endif

#ifdef MEGA_138K
  // pins 3-10 are FFC inputs on the 138k side
  GPIO_InitType.GPIO_Pin = GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  |
                           GPIO_Pin_6  | GPIO_Pin_7  | GPIO_Pin_8  |
                           GPIO_Pin_9  | GPIO_Pin_10;
  GPIO_InitType.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitType.GPIO_Int = GPIO_Int_Disable;
  GPIO_Init(GPIO0, &GPIO_InitType);
#endif


  //Initializes output value
  //on:0; 1:off on some boards
  //on:1; 0:off on some boards
/*
  GPIO_SetBit(GPIO0, GPIO_Pin_0 |
		  	  	  	 GPIO_Pin_1 |
					 GPIO_Pin_2 |
					 GPIO_Pin_3 |
					 GPIO_Pin_4 |
					 GPIO_Pin_5 |
					 GPIO_Pin_6);
*/
}

void gpio_toggle(GPIO_TypeDef *port, uint32_t pin) {
  if (port->DATAOUT & pin) {
    port->DATAOUT &= ~pin;
  } else {
    port->DATAOUT |= pin;
  }
}

void gpio_write(GPIO_TypeDef *port, uint32_t pin, pin_state_e state) {
  if (state == PIN_HIGH) {
    port->DATAOUT |= pin;
  } else {
    port->DATAOUT &= ~pin;
  }
}

pin_state_e gpio_read(GPIO_TypeDef *port, uint32_t pin) {
  return (port->DATA & pin) ? PIN_HIGH : PIN_LOW;
}
