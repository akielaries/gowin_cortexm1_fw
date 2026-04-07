#include <stdint.h>

#include "multiflex_regs.h"
#include "mfx.h"

#include "debug.h"


volatile struct multiflex_regs *mfx = (struct multiflex_regs *)(APB_M1 + 0x60);


void mfx_init(void) {
  dbg_printf("multiflex init....\r\n");
  mfx->ctrl &= ~MULTIFLEX_REGS_CTRL_ENABLE;

  for (volatile uint32_t i = 0; i < 10000; i++) {}

  // 1 lane, clk div 49 -> 100mhz / (2 * 50) = 1mhz wire clock, then enable
  mfx->ctrl = (mfx->ctrl & ~MULTIFLEX_REGS_CTRL_LANES_MASK)
              | (1UL << MULTIFLEX_REGS_CTRL_LANES_SHIFT);
  mfx->ctrl = (mfx->ctrl & ~MULTIFLEX_REGS_CTRL_CLK_DIV_MASK)
              | (49UL << MULTIFLEX_REGS_CTRL_CLK_DIV_SHIFT);
  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;
}

// block until tx fifo has room, then enqueue one byte
void mfx_send_byte(uint8_t b) {
  while (mfx->status & MULTIFLEX_REGS_STATUS_TX_FULL) {}
  mfx->tx_data = b;
}

void mfx_send(const uint8_t *buf, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    mfx_send_byte(buf[i]);
  }
}

void mfx_clear(void) {
  mfx->ctrl &= ~MULTIFLEX_REGS_CTRL_ENABLE;
  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;
}
