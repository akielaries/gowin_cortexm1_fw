#include <stdint.h>

#include "multiflex_regs.h"
#include "mfx.h"

#include "debug.h"


// multiflex base is at APB_M1 + 0x8000 (= 0x60008000)
// ctrl/status at +0x0000, tx_buf at +0x2000, rx_buf at +0x4000
volatile struct multiflex_regs *mfx = (struct multiflex_regs *)(APB_M1 + 0x8000);


void mfx_init(void) {
  dbg_printf("multiflex init....\r\n");
  mfx->ctrl &= ~MULTIFLEX_REGS_CTRL_ENABLE;

  for (volatile uint32_t i = 0; i < 10000; i++) {}

  // 1 tx lane, 1 rx lane, clk_div 15 (~3 MHz wire clock at 100 MHz fabric)
  mfx->ctrl = (1UL << MULTIFLEX_REGS_CTRL_LANES_TX_SHIFT)
            | (1UL << MULTIFLEX_REGS_CTRL_LANES_RX_SHIFT)
            | (15UL << MULTIFLEX_REGS_CTRL_CLK_DIV_SHIFT)
            | MULTIFLEX_REGS_CTRL_ENABLE;
}

// block until tx fifo has room, then enqueue one byte (streaming path)
void mfx_send_byte(uint8_t b) {
  while (mfx->status & MULTIFLEX_REGS_STATUS_TX_FULL) {}
  mfx->tx_data = b;
}

void mfx_send(const uint8_t *buf, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    mfx_send_byte(buf[i]);
  }
}

// bulk TX: copy up to 2048 bytes into tx_buf then trigger drain via tx_len.
// each tx_buf entry is 8-bit but word-addressed on APB, so use 32-bit writes.
void mfx_send_buf(const uint8_t *data, uint32_t len) {
  if (len > 2048) { len = 2048; }
  while (mfx->status & MULTIFLEX_REGS_STATUS_TX_BUSY) {}
  volatile uint32_t *tbuf = (volatile uint32_t *)mfx->tx_buf;
  for (uint32_t i = 0; i < len; i++) {
    tbuf[i] = data[i];
  }
  mfx->tx_len = len & MULTIFLEX_REGS_TX_LEN_LEN_MASK;
}

// bulk RX: drain up to maxlen bytes from rx_buf, clear rx state, return count.
// each rx_buf entry is 8-bit but word-addressed on APB, so use 32-bit reads.
uint32_t mfx_recv_buf(uint8_t *data, uint32_t maxlen) {
  uint32_t avail = mfx->rx_count & MULTIFLEX_REGS_RX_COUNT_COUNT_MASK;
  uint32_t n = (avail < maxlen) ? avail : maxlen;
  volatile uint32_t *rbuf = (volatile uint32_t *)mfx->rx_buf;
  for (uint32_t i = 0; i < n; i++) {
    data[i] = (uint8_t)(rbuf[i] & 0xFF);
  }
  // clr_rx resets the hardware write pointer and rx_count
  mfx->error_clr = MULTIFLEX_REGS_ERROR_CLR_CLR_RX;
  return n;
}

void mfx_clear(void) {
  mfx->ctrl &= ~MULTIFLEX_REGS_CTRL_ENABLE;
  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;
}

static void hexdump(const char *label, const uint8_t *buf, uint32_t len) {
  dbg_printf("%s (%u bytes):\r\n", label, (unsigned)len);
  for (uint32_t i = 0; i < len; i += 16) {
    dbg_printf("  %04x:", (unsigned)i);
    for (uint32_t j = i; j < i + 16 && j < len; j++) {
      dbg_printf(" %02x", (unsigned)buf[j]);
    }
    dbg_printf("\r\n");
  }
}

// physical loopback test: TX pins must be wired to RX pins externally.
// uses bulk path (mfx_send_buf) so both tx_buf and rx_buf are populated
// and can be read back for validation.
void mfx_phys_loopback_test(uint32_t num_bytes, uint8_t clk_div) {
  if (num_bytes > 256) { num_bytes = 256; }

  uint32_t ctrl = mfx->ctrl;
  ctrl &= ~(MULTIFLEX_REGS_CTRL_ENABLE
          | MULTIFLEX_REGS_CTRL_LOOPBACK_MASK
          | MULTIFLEX_REGS_CTRL_CLK_DIV_MASK);
  ctrl |= ((uint32_t)clk_div << MULTIFLEX_REGS_CTRL_CLK_DIV_SHIFT);
  mfx->ctrl = ctrl;

  mfx->error_clr = MULTIFLEX_REGS_ERROR_CLR_CLR_RX
                 | MULTIFLEX_REGS_ERROR_CLR_CLR_TX;

  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;

  // build incrementing pattern and send via bulk path so tx_buf is populated
  uint8_t txdata[256];
  for (uint32_t i = 0; i < num_bytes; i++) {
    txdata[i] = (uint8_t)(i & 0xFF);
  }
  mfx_send_buf(txdata, num_bytes);

  // poll status during transmission to catch rx_locked going high
  uint32_t locked_seen = 0;
  uint32_t status_snap = 0;
  while (mfx->status & MULTIFLEX_REGS_STATUS_TX_BUSY) {
    uint32_t s = mfx->status;
    if (s & MULTIFLEX_REGS_STATUS_RX_LOCKED) {
      locked_seen = 1;
      status_snap = s;
    }
  }

  uint32_t err_cnt  = (mfx->error_cnt & MULTIFLEX_REGS_ERROR_CNT_RX_ERROR_COUNT_MASK)
                      >> MULTIFLEX_REGS_ERROR_CNT_RX_ERROR_COUNT_SHIFT;
  uint32_t locked   = (mfx->status & MULTIFLEX_REGS_STATUS_RX_LOCKED)    ? 1 : 0;
  uint32_t syn_lost = (mfx->status & MULTIFLEX_REGS_STATUS_RX_SYNC_LOST) ? 1 : 0;
  uint32_t rx_cnt   = mfx->rx_count & MULTIFLEX_REGS_RX_COUNT_COUNT_MASK;

  dbg_printf("phys_loopback clk_div=%u bytes=%u locked=%u sync_lost=%u rx_errors=%u rx_count=%u locked_during_tx=%u\r\n",
             (unsigned)clk_div, (unsigned)num_bytes,
             (unsigned)locked, (unsigned)syn_lost, (unsigned)err_cnt, (unsigned)rx_cnt,
             (unsigned)locked_seen);
  if (!locked_seen) {
    dbg_printf("  WARNING: rx_locked never went high -- RX engine not decoding (check jumper/clock)\r\n");
  } else {
    dbg_printf("  rx_locked seen during tx (status_snap=0x%08x) -- RX engine active\r\n",
               (unsigned)status_snap);
  }

  // read back tx_buf via APB to confirm what was written
  uint8_t tbuf_rb[256];
  volatile uint32_t *tbuf = (volatile uint32_t *)mfx->tx_buf;
  for (uint32_t i = 0; i < num_bytes; i++) {
    tbuf_rb[i] = (uint8_t)(tbuf[i] & 0xFF);
  }
  hexdump("tx_buf", tbuf_rb, num_bytes);

  // read back rx_buf
  uint8_t rbuf[256];
  uint32_t n = mfx_recv_buf(rbuf, rx_cnt < num_bytes ? rx_cnt : num_bytes);
  hexdump("rx_buf", rbuf, n);

  // compare
  uint32_t mismatches = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (rbuf[i] != txdata[i]) { mismatches++; }
  }
  dbg_printf("  mismatches=%u\r\n", (unsigned)mismatches);

  mfx->ctrl &= ~MULTIFLEX_REGS_CTRL_ENABLE;
  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;
}

// fabric loopback BER test: send incrementing bytes, count RX mismatches.
void mfx_loopback_test(uint32_t num_bytes, uint8_t clk_div) {
  uint32_t ctrl = mfx->ctrl;
  ctrl &= ~(MULTIFLEX_REGS_CTRL_ENABLE
          | MULTIFLEX_REGS_CTRL_LOOPBACK_MASK
          | MULTIFLEX_REGS_CTRL_CLK_DIV_MASK);
  ctrl |= MULTIFLEX_REGS_CTRL_LOOPBACK
        | ((uint32_t)clk_div << MULTIFLEX_REGS_CTRL_CLK_DIV_SHIFT);
  mfx->ctrl = ctrl;

  mfx->error_clr = MULTIFLEX_REGS_ERROR_CLR_CLR_RX
                 | MULTIFLEX_REGS_ERROR_CLR_CLR_TX;

  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;

  for (uint32_t i = 0; i < num_bytes; i++) {
    mfx_send_byte((uint8_t)(i & 0xFF));
  }

  uint32_t locked_seen = 0;
  while (mfx->status & MULTIFLEX_REGS_STATUS_TX_BUSY) {
    if (mfx->status & MULTIFLEX_REGS_STATUS_RX_LOCKED) {
      locked_seen = 1;
    }
  }

  uint32_t err_cnt  = (mfx->error_cnt & MULTIFLEX_REGS_ERROR_CNT_RX_ERROR_COUNT_MASK)
                      >> MULTIFLEX_REGS_ERROR_CNT_RX_ERROR_COUNT_SHIFT;
  uint32_t locked   = (mfx->status & MULTIFLEX_REGS_STATUS_RX_LOCKED)    ? 1 : 0;
  uint32_t syn_lost = (mfx->status & MULTIFLEX_REGS_STATUS_RX_SYNC_LOST) ? 1 : 0;
  uint32_t rx_cnt   = mfx->rx_count & MULTIFLEX_REGS_RX_COUNT_COUNT_MASK;

  dbg_printf("loopback clk_div=%u bytes=%u locked=%u sync_lost=%u rx_errors=%u rx_count=%u locked_during_tx=%u\r\n",
             (unsigned)clk_div, (unsigned)num_bytes,
             (unsigned)locked, (unsigned)syn_lost, (unsigned)err_cnt, (unsigned)rx_cnt,
             (unsigned)locked_seen);
  if (!locked_seen) {
    dbg_printf("  WARNING: rx_locked never went high -- RX engine not decoding even in fabric loopback\r\n");
  }

  mfx->ctrl &= ~(MULTIFLEX_REGS_CTRL_ENABLE | MULTIFLEX_REGS_CTRL_LOOPBACK_MASK);
  mfx->ctrl |= MULTIFLEX_REGS_CTRL_ENABLE;
}
