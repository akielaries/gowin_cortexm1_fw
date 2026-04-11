#pragma once
#include <stdint.h>
#include "multiflex_regs.h"

extern volatile struct multiflex_regs *mfx;

void     mfx_init(void);
void     mfx_send_byte(uint8_t b);
void     mfx_send(const uint8_t *buf, uint32_t len);
void     mfx_send_buf(const uint8_t *data, uint32_t len);
uint32_t mfx_recv_buf(uint8_t *data, uint32_t maxlen);
void     mfx_clear(void);
void     mfx_loopback_test(uint32_t num_bytes, uint8_t clk_div);
void     mfx_phys_loopback_test(uint32_t num_bytes, uint8_t clk_div);
