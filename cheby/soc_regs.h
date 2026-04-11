#ifndef __CHEBY__SOC__H__
#define __CHEBY__SOC__H__


#include "sysinfo_regs.h"
#include "gpio_regs.h"
#include "multiflex_regs.h"
#include "sfp_regs.h"
#define SOC_SIZE 65568 /* 0x10020 */

/* REG sysinfo */
#define SOC_SYSINFO 0x0UL
#define ADDR_MASK_SOC_SYSINFO 0x1ffe0UL
#define ADDR_FMASK_SOC_SYSINFO 0x1ffe0UL
#define SOC_SYSINFO_SIZE 32 /* 0x20 */

/* REG gpio */
#define SOC_GPIO 0x20UL
#define ADDR_MASK_SOC_GPIO 0x1fff8UL
#define ADDR_FMASK_SOC_GPIO 0x1fff8UL
#define SOC_GPIO_SIZE 8 /* 0x8 */

/* REG multiflex */
#define SOC_MULTIFLEX 0x8000UL
#define ADDR_MASK_SOC_MULTIFLEX 0x18000UL
#define ADDR_FMASK_SOC_MULTIFLEX 0x18000UL
#define SOC_MULTIFLEX_SIZE 32768 /* 0x8000 = 32KB */

/* REG sfp */
#define SOC_SFP 0x10000UL
#define ADDR_MASK_SOC_SFP 0x1ffe0UL
#define ADDR_FMASK_SOC_SFP 0x1ffe0UL
#define SOC_SFP_SIZE 32 /* 0x20 */

#ifndef __ASSEMBLER__
struct soc {
  /* [0x0]: SUBMAP */
  struct sysinfo_regs sysinfo;

  /* padding to: 32 Bytes */
  uint32_t __padding_0[3];

  /* [0x20]: SUBMAP */
  struct gpio_regs gpio;

  /* padding to: 32768 Bytes */
  uint32_t __padding_1[8182];

  /* [0x8000]: SUBMAP */
  struct multiflex_regs multiflex;

  /* padding to: 65536 Bytes */
  uint32_t __padding_2[2048];

  /* [0x10000]: SUBMAP */
  struct sfp_regs sfp;

  /* padding to: 65568 Bytes */
  uint32_t __padding_3[3];
};
#endif /* !__ASSEMBLER__*/

#endif /* __CHEBY__SOC__H__ */
