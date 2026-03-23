/*
 * gpio.h
 *
 *  Created on: Jan 25, 2026
 *      Author: akiel
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include "GOWIN_M1.h"

typedef enum {
  PIN_LOW  = 0,
  PIN_HIGH = 1
} pin_state_e;

void         gpio_init(void);
void         gpio_toggle(GPIO_TypeDef *port, uint32_t pin);
void         gpio_write(GPIO_TypeDef *port, uint32_t pin, pin_state_e state);
pin_state_e  gpio_read(GPIO_TypeDef *port, uint32_t pin);

#endif /* GPIO_H_ */
