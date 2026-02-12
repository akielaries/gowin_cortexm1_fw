#include "GOWIN_M1_uart.h"

#include "debug.h"


void debug_init(void) {
  UART_InitTypeDef UART_InitStruct;

  UART_InitStruct.UART_BaudRate         = 115200;
  UART_InitStruct.UART_Mode.UARTMode_Tx = ENABLE;
  UART_InitStruct.UART_Mode.UARTMode_Rx = ENABLE;
  UART_InitStruct.UART_Int.UARTInt_Tx   = DISABLE;
  UART_InitStruct.UART_Int.UARTInt_Rx   = DISABLE;
  UART_InitStruct.UART_Ovr.UARTOvr_Tx   = DISABLE;
  UART_InitStruct.UART_Ovr.UARTOvr_Rx   = DISABLE;
  UART_InitStruct.UART_Hstm             = DISABLE;

  UART_Init(UART1, &UART_InitStruct);
}

