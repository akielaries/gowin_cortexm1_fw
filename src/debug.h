#pragma once

#include <stdarg.h>
#include <stdint.h>
#include "GOWIN_M1_uart.h"

void debug_init(void);
void dbg_printf(const char* format, ...);
void dbg_hexdump(const uint8_t *buf, uint32_t len);

