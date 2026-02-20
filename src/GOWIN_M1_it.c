/*
 ******************************************************************************************
 * @file      GOWIN_M1_it.c
 * @author    GowinSemicoductor
 * @device    Gowin_EMPU_M1
 * @brief     Main Interrupt Service Routines.
 *            This file provides template for all exceptions handler and
 *            peripherals interrupt service routine.
 ******************************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "GOWIN_M1_it.h"
#include "GOWIN_M1.h"
#include "debug.h"


/* Definitions ---------------------------------------------------------------*/
/** @addtogroup GOWIN_M1_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

extern thread_t *current_thread;
//extern thread_t *scheduler_next(void);

/******************************************************************************/
/*            Cortex-M1 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  none
  * @retval none
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  none
  * @retval none
  */
void HardFault_Handler(void)
{
  /*
  dbg_printf("\r\nhit hard fault...\r\n");
    __asm volatile(
        "mrs r0, msp\n"
        "mrs r1, psp\n"
        "bkpt #0\n"   //halt so GDB can catch it cleanly
    );
  */
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  none
  * @retval none
  */
void SVC_Handler(void)
{
    /*
     * kernel_start initiates the first context switch by calling svc.
     * this handler just needs to pend the pendsv interrupt to get
     * the scheduler running.
     */
  dbg_printf("SVC_Handler\r\n");
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  none
  * @retval none
  */
/*
__attribute__((naked)) void PendSV_Handler(void)
{
  dbg_printf("sv handler\r\n");
    __asm volatile(
        "cpsid i           \n"
        "mrs r0, psp       \n"      // current PSP
        "ldr r1, =current_thread \n"
        "ldr r2, [r1]      \n"
        "cmp r2, #0        \n"
        "beq 1f            \n"      // skip save if NULL

        // save r4-r7
        "sub r0, #16       \n"
        "stm r0, {r4-r7}   \n"

        // save r8-r11 via r4 temporary
        "mov r4, r8        \n"
        "str r4, [r0,#0]   \n"
        "mov r4, r9        \n"
        "str r4, [r0,#4]   \n"
        "mov r4, r10       \n"
        "str r4, [r0,#8]   \n"
        "mov r4, r11       \n"
        "str r4, [r0,#12]  \n"

        // update thread SP
        "str r0, [r2]      \n"
        "1:                \n"

        // get next thread
        "bl scheduler_next \n"
        "str r0, [r1]      \n"
        "ldr r0, [r0]      \n"       // PSP of next thread

        // restore r4-r7
        "ldm r0!, {r4-r7}  \n"

        // restore r8-r11 via r4 temporary
        "ldr r4, [r0,#0]   \n"
        "mov r8, r4        \n"
        "ldr r4, [r0,#4]   \n"
        "mov r9, r4        \n"
        "ldr r4, [r0,#8]   \n"
        "mov r10, r4       \n"
        "ldr r4, [r0,#12]  \n"
        "mov r11, r4       \n"

        "msr psp, r0       \n"
        "cpsie i           \n"
        "bx lr             \n"
    );
}

*/
/**
  * @brief  This function handles SysTick Handler.
  * @param  none
  * @retval none
  */
void SysTick_Handler(void)
{
	system_time_ms++;
}

/******************************************************************************/
/*                 GOWIN_M1 Peripherals Interrupt Handlers                     */
/*  Add here the Interrupt Handler for the used peripheral(s) (XXX), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_GOWIN_M1.s).                                                 */
/******************************************************************************/

/**
  * @brief  This function handles UART0 interrupt request.
  * @param  none
  * @retval none
  */
void UART0_Handler(void)
{
}

/**
  * @brief  This function handles UART1 interrupt request.
  * @param  none
  * @retval none
  */
void UART1_Handler(void)
{
}

/**
  * @brief  This function handles TIMER0 interrupt request.
  * @param  none
  * @retval none
  */
void TIMER0_Handler(void)
{
}

/**
  * @brief  This function handles TIMER1 interrupt request.
  * @param  none
  * @retval none
  */
void TIMER1_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_0~15 combine interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_Handler(void)
{
}

/**
  * @brief  This function handles RTC interrupt request.
  * @param  none
  * @retval none
  */
void RTC_Handler(void)
{
}

/**
  * @brief  This function handles I2C interrupt request.
  * @param  none
  * @retval none
  */
void I2C_Handler(void)
{
}

/**
  * @brief  This function handles CAN interrupt request.
  * @param  none
  * @retval none
  */
void CAN_Handler(void)
{
}

/**
  * @brief  This function handles Ethernet interrupt request.
  * @param  none
  * @retval none
  */
void ENT_Handler(void)
{
}

/**
  * @brief  This function handles DualTimer interrupt request.
  * @param  none
  * @retval none
  */
void DTimer_Handler(void)
{
}

/**
  * @brief  This function handles TRNG interrupt request.
  * @param  none
  * @retval none
  */
void TRNG_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_0 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_0_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_1 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_1_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_2 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_2_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_3 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_3_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_4 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_4_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_5 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_5_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_6 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_6_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_7 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_7_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_8 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_8_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_9 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_9_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_10 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_10_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_11 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_11_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_12 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_12_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_13 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_13_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_14 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_14_Handler(void)
{
}

/**
  * @brief  This function handles GPIO0_15 interrupt request.
  * @param  none
  * @retval none
  */
void GPIO0_15_Handler(void)
{
}

/**
  * @brief  This function handles external 0 interrupt request.
  * @param  none
  * @retval none
  */
void EXTINT_0_Handler(void)
{
}

/**
  * @brief  This function handles external 1 interrupt request.
  * @param  none
  * @retval none
  */
void EXTINT_1_Handler(void)
{
}

/**
  * @brief  This function handles external 2 interrupt request.
  * @param  none
  * @retval none
  */
void EXTINT_2_Handler(void)
{
}

/**
  * @brief  This function handles external 3 interrupt request.
  * @param  none
  * @retval none
  */
void EXTINT_3_Handler(void)
{
}

/**
  * @}
  */
