#include "GOWIN_M1_uart.h"
#include "debug.h"

// Helper function to print a number
static void print_number(int n) {
    if (n < 0) {
        UART_SendChar(UART1, '-');
        n = -n;
    }
    if (n / 10) {
        print_number(n / 10);
    }
    UART_SendChar(UART1, (n % 10) + '0');
}

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

void dbg_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);

  while (*format) {
    if (*format == '%') {
      format++;
      switch (*format) {
        case 'd': {
          int i = va_arg(args, int);
          print_number(i);
          break;
        }
        case 's': {
          char* s = va_arg(args, char*);
          while (*s) {
            UART_SendChar(UART1, *s++);
          }
          break;
        }
        case 'c': {
          // A 'char' variable will be promoted to 'int'
          char c = (char)va_arg(args, int);
          UART_SendChar(UART1, c);
          break;
        }
        case '%': {
          UART_SendChar(UART1, '%');
          break;
        }
        default:
          // Unsupported format specifier, just print it
          UART_SendChar(UART1, '%');
          UART_SendChar(UART1, *format);
          break;
      }
    } else {
      UART_SendChar(UART1, *format);
    }
    format++;
  }

  va_end(args);
}
