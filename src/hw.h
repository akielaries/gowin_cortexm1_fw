#pragma once

#include "sysinfo_regs.h"
#include "gpio_regs.h"
#include "sfp_regs.h"

extern volatile struct sysinfo_regs *sysinfo;
extern volatile struct gpio_regs    *gpio;
extern volatile struct sfp_regs     *sfp;

void hw_init(void);

