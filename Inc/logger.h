/*
  Brian Khuu 2019
  Written for stm32h7

  stm32h7 tip: https://www.st.com/content/st_com/en/support/learning/stm32-education/stm32-online-training/stm32h7-online-training.html
*/


// Enable/Disable
#define SEMIHOST_FEATURE_ENABLED_SWO
//#define SEMIHOST_FEATURE_ENABLED_SEMIHOSTING
#define SEMIHOST_FEATURE_ENABLED_STM32_UART

#ifdef SEMIHOST_FEATURE_ENABLED_STM32_UART
#include "stm32h7xx_hal.h"
#endif

typedef enum log_type_t
{
  LOGGER_LOG_PANIC  = -2,
  LOGGER_LOG_FATAL  = -1,
  LOGGER_LOG_ERROR  =  0,
  LOGGER_LOG_WARN   =  1,
  LOGGER_LOG_INFO   =  2,
  LOGGER_LOG_DEBUG  =  3,
  LOGGER_LOG_TRACE  =  4,
  LOGGER_LOG_RAW    =  5
} log_type_t;


/***********************
  LOGGER SETUP
************************/

/* INIT Logger TX mode */
#ifdef SEMIHOST_FEATURE_ENABLED_SWO
void log_init_swo();
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_SEMIHOSTING
void log_init_semihosting();
#endif
#ifdef SEMIHOST_FEATURE_ENABLED_STM32_UART
void log_init_stm32_uart(UART_HandleTypeDef *hwuart);
#endif

void log_set_level(log_type_t level);


/***********************
  LOGGER EXTERNAL MACRO
************************/

#ifdef LOGGER_DISABLE

#define log_raw(...)
#define log_trace(...)
#define log_debug(...)
#define log_info(...)
#define log_err(...)
#define log_warn(...)
#define log_hex_dump_raw(...)
#define log_hex_dump_trace(...)
#define log_hex_dump_debug(...)
#define log_hex_dump_info(...)
#define log_hex_dump_warn(...)
#define log_hex_dump_err(...)

#else

/* recording method */
void log_record(const int type, const char *format, ...);
void log_hex_dump(const int type, const char *annotate, const void *addr, const unsigned nBytes);

/* log_record */
#define log_raw(   MSG, ...)  log_record(LOGGER_LOG_RAW,   MSG     , ##__VA_ARGS__)
#define log_trace( MSG, ...)  log_record(LOGGER_LOG_TRACE, MSG "\r\n", ##__VA_ARGS__)
#define log_debug( MSG, ...)  log_record(LOGGER_LOG_DEBUG, MSG "\r\n", ##__VA_ARGS__)
#define log_info(  MSG, ...)  log_record(LOGGER_LOG_INFO,  MSG "\r\n", ##__VA_ARGS__)
#define log_warn(  MSG, ...)  log_record(LOGGER_LOG_WARN,  MSG "\r\n", ##__VA_ARGS__)
#define log_err(   MSG, ...)  log_record(LOGGER_LOG_ERROR, MSG "\r\n", ##__VA_ARGS__)

/* log_hex_dump */
#define log_hex_dump_raw(   MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_RAW,   MSG, addr, nBytes)
#define log_hex_dump_trace( MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_TRACE, MSG, addr, nBytes)
#define log_hex_dump_debug( MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_DEBUG, MSG, addr, nBytes)
#define log_hex_dump_info(  MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_INFO,  MSG, addr, nBytes)
#define log_hex_dump_warn(  MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_WARN,  MSG, addr, nBytes)
#define log_hex_dump_err(   MSG, addr, nBytes)  log_hex_dump(LOGGER_LOG_ERROR, MSG, addr, nBytes)

#endif