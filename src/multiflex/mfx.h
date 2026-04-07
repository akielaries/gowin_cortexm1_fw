#pragma once
#include <stdint.h>
#include "multiflex_regs.h"

extern volatile struct multiflex_regs *mfx;

void mfx_init(void);
void mfx_send_byte(uint8_t b);
void mfx_send(const uint8_t *buf, uint32_t len);
void mfx_clear(void);
