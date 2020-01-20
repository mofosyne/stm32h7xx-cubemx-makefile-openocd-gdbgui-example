/*
  Brian Khuu 2019
  Written for stm32h7

  stm32h7 tip: https://www.st.com/content/st_com/en/support/learning/stm32-education/stm32-online-training/stm32h7-online-training.html
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h> //isprint

#include "logger.h"

#if defined(SEMIHOST_FEATURE_ENABLED_SWO) || defined(SEMIHOST_FEATURE_ENABLED_STM32_UART)
#include "stm32h7xx_hal.h"
#endif

/***********************
  LOGGER Globals
************************/

// Generic global var
static log_type_t log_level = LOGGER_LOG_RAW;

/* Tx Mode Enabled */
#ifdef SEMIHOST_FEATURE_ENABLED_SWO
bool log_enable_swo = false;
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_SEMIHOSTING
bool log_enable_semihost = false;
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_STM32_UART
bool log_enable_stm32_uart = false;
UART_HandleTypeDef *ghwuart_ptr = NULL;
#endif

/***********************
  LOGGER TX INIT
************************/
#ifdef SEMIHOST_FEATURE_ENABLED_SWO
void log_init_swo()
{
  // Note: Not great for production. Semihosting is slow and can crash if debug is not connected.
  log_enable_swo = true;
};
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_SEMIHOSTING
void log_init_semihosting()
{
  // Note: Not great for production. Semihosting is slow and can crash if debug is not connected.
  log_enable_semihost = true;
};
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_STM32_UART
void log_init_stm32_uart(UART_HandleTypeDef *hwuart)
{
  // Note: Currently using HAL_UART_Transmit() which is a blocking Tx. You may need to mod this to use ISR if needed.
  log_enable_stm32_uart = true;
  ghwuart_ptr = hwuart;
};
#endif

void log_set_level(log_type_t level)
{
  log_level = level;
}

/***********************
  LOGGER
************************/

static void log_tx(const char *str, const int len)
{
#ifdef SEMIHOST_FEATURE_ENABLED_SWO
  if (log_enable_swo)
  {
    for (int i = 0 ; i < len ; i++)
    {
      ITM_SendChar(str[i]);
    }
  }
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_STM32_UART
  if (log_enable_stm32_uart)
  {
    HAL_UART_Transmit(ghwuart_ptr, (uint8_t *)str, len, 0xFF);
  }
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_SEMIHOSTING
  if (log_enable_semihost)
  {
    /* Check if debugger is attached */
    if (!((CoreDebug->DHCSR & 1) == 1 ))
    {
      return;
    }
    /* Call Semihost */
    int args[3] = {2 /*stderr*/, (int) str, (int)len};
    asm volatile (
      " mov r0, %[reason]  \n"
      " mov r1, %[arg]  \n"
      " bkpt %[swi] \n"
      : /* Output */
      : [reason] "r" (0x05), [arg] "r" (args), [swi] "i" (0xAB) /* Inputs */
      : "r0", "r1", "r2", "r3", "ip", "lr", "memory", "cc"
    );
  }
#endif
}


static void log_tx_str(const char *str)
{
  /* Calculate String Length */
  int len = 0;
  for (len = 0; str[len]; (len)++);

  /* Tx */
  log_tx(str, len);
}

void log_record(const int type, const char *format, ...)
{
  char buff[400] = {0};
  size_t buff_size = sizeof(buff);

  if (log_level < type)
  {
    return;
  }

  /* Format String */
  va_list argptr;
  va_start(argptr, format);
  vasnprintf(buff, &buff_size, format, argptr);
  va_end(argptr);

  /* Print Log Type */
  switch (type)
  {
    case LOGGER_LOG_PANIC :
      log_tx_str("P: ");
      break;
    case LOGGER_LOG_FATAL :
      log_tx_str("F: ");
      break;
    case LOGGER_LOG_ERROR :
      log_tx_str("E: ");
      break;
    case LOGGER_LOG_WARN  :
      log_tx_str("W: ");
      break;
    case LOGGER_LOG_INFO  :
      log_tx_str("I: ");
      break;
    case LOGGER_LOG_DEBUG :
      log_tx_str("D: ");
      break;
    case LOGGER_LOG_TRACE :
      log_tx_str("T: ");
      break;
  }

  log_tx_str(buff);
}

void log_hex_dump(const int type, const char *annotate, const void *addr, const unsigned nBytes)
{
  const uint8_t byte_per_row = 16;
  const uint8_t *addr_ptr = addr;

  if (log_level < type)
    return;

  /* Print Log Type */
  switch (type)
  {
    case LOGGER_LOG_PANIC :
      log_tx_str("P: ");
      break;
    case LOGGER_LOG_FATAL :
      log_tx_str("F: ");
      break;
    case LOGGER_LOG_ERROR :
      log_tx_str("E: ");
      break;
    case LOGGER_LOG_WARN  :
      log_tx_str("W: ");
      break;
    case LOGGER_LOG_INFO  :
      log_tx_str("I: ");
      break;
    case LOGGER_LOG_DEBUG :
      log_tx_str("D: ");
      break;
    case LOGGER_LOG_TRACE :
      log_tx_str("T: ");
      break;
  }

  log_tx_str(annotate);
  log_tx_str(" :\n");

  /* Empty Check */
  for (uint32_t i = 0 ; i < nBytes ; i++)
  {
    const uint8_t *addr8 = addr;
    if (i == 0)
    {
      continue;
    }
    else if (addr8[i - 1] != addr8[i])
    {
      break;
    }
    else if ( (i + 1) == nBytes )
    {
      char buff[400] = {0}; // Line Buff
      char *buff_ptr = buff;
      const char *buff_end = buff + sizeof(buff) - 1;
      buff_ptr += snprintf(buff_ptr, buff_end - buff_ptr, " 0 | All %02X (%u Bytes)", (unsigned) addr8[0], (unsigned) nBytes);
      log_tx_str(buff);
      log_tx_str("\n");
      return;
    }
  }

  /* Print Hex */
  uint32_t offset = 0;
  while (offset < nBytes)
  {
    char buff[400] = {0}; // Line Buff
    char *buff_ptr = buff;
    const char *buff_end = buff + sizeof(buff) - 1;

    // HEX
    buff_ptr += snprintf(buff_ptr, buff_end - buff_ptr, " %4u | ", (unsigned) offset);
    for (uint8_t i = 0 ; i < byte_per_row ; i++)
    {
      if ((offset + i) >= nBytes)
        break;
      buff_ptr += snprintf(buff_ptr, buff_end - buff_ptr, " %02x", addr_ptr[offset + i]);
    }

#if 0
    // PRINTABLE
    buff_ptr += snprintf(buff_ptr, buff_end - buff_ptr, "  :  ");
    for (uint8_t i = 0 ; i < byte_per_row ; i++)
    {
      if ((offset + i) >= nBytes)
        break;
      char c = isprint(addr_ptr[offset + i]) ? addr_ptr[offset + i] : '.';
      buff_ptr += snprintf(buff_ptr, buff_end - buff_ptr, "%c", c);
    }
#endif

    log_tx_str(buff);
    log_tx_str("\n");

    // INCREMENT
    offset += byte_per_row;
  }

  log_tx_str("\n");
}
