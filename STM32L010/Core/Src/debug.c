/*
 * debug.c
 *
 *  Created on: Jan 23, 2023
 *  Author: KVovk
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "debug.h"

/** Enable output debug information through UART */
#define DEBUG_UART

#ifdef DEBUG_UART
	extern UART_HandleTypeDef huart2;
	/** STM32 UART used for DEBUG purpose */
	#define DBG_UART	 huart2
#endif // DEBUG_UART

/** Length for the buffer used for console output */
#define CON_BUF_LEN		(unsigned int) 384
/** Console buffer */
char con_buf[CON_BUF_LEN];

/** DEBUG UART is still in sending process if this bit in logic '1' */
#define DBG_TX_IN_PROCESS	1
/** DEBUG UART receive something in buffer -> need to process it if this flag is set to logic '1' */
#define DBG_RX_COMPLETED	2

/** Variable used for inform that transmission from UART completed */
volatile unsigned char dbg_uart_status = 0;

/** Default logging configuration for the firmware modules */
DBG_CONFIG DBG_default_cfg = {
		NUM_DBG_MODULES,
		{
			{MODULE_APP,	DBG_TRACE},
			{MODULE_MEASURE,DBG_TRACE},
			{MODULE_ESP32,	DBG_TRACE},
			{MODULE_SENSOR,	DBG_TRACE},
		},
};

/** Current logging settings for all modules */
DBG_CONFIG log_config;

//-----------------------------------------------------------------------------
/** Checks whether the logging output is enabled for given module and log level.
 *
 * \param module Module for which we check status
 * \param log_lvl Provided logging level
 * \return Nonzero value if logging is enabled for provided module and logging level is less the provided in module as default
 */
int DBG_is_enabled(LOG_MODULE module, LOG_TYPE log_lvl)
{
    return (log_lvl >= log_config.moduleLogCfg[module].log_Level);
}

//-----------------------------------------------------------------------------
/** Check if DEBUG UART is ready to send next debug message
 *
 * \return '0' - is ready to send new message, '-1' - otherwise
 */
int DBG_ready_for_tx(void)
{
	// If we busy now with output something by DMA on UART
	if (dbg_uart_status & DBG_TX_IN_PROCESS) {
		// Return "-1"
		return (-1);
	}
	else {
		__disable_irq();
			// Set that we will busy with output string by DEBUG UART
			dbg_uart_status |= DBG_TX_IN_PROCESS;
		__enable_irq();
		// Return "0"
		return (0);
	}
}

//-----------------------------------------------------------------------------
/** Output debug message to the specified debug interface
 *
 * Function declares internal console buffer \sa CON_BUF_LEN where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param fmt Format string that is used for format message
 * \param ... Text message and message data
 * \return None
 */
void DBG_msg_fmt(const char* fmt, ...)
{
    va_list arglist;
    int pos;

#ifdef DEBUG_UART
    // Wait for UART is ready for start new transmission
    while (DBG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = vsnprintf(&con_buf[0], CON_BUF_LEN, fmt, arglist);

    va_end(arglist);

    // Check if we have buffer overflow
    if (pos >= CON_BUF_LEN - 3) {
    	// Truncate and output CR+LF at the end of string
        snprintf(&con_buf[CON_BUF_LEN - 3], 3, "\r\n");
        pos = CON_BUF_LEN - 1;
    }
    // Output CR+LF at the end of string
    else {
    	pos += snprintf(&con_buf[pos], 3, "\r\n");
    }

#ifdef DEBUG_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
//	app_params.dont_go_stop |= APP_DEBUG_ACTIVE;
    // Output data to UART
//    HAL_UART_Transmit_IT(&DBG_UART, (uint8_t *) con_buf, pos);
    HAL_UART_Transmit_DMA(&DBG_UART, (uint8_t *) con_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa CON_BUF_LEN where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the byte data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DBG_msg_fmt_buf(const uint8_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_UART
    // Wait for UART is ready for start new transmission
    while (DBG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = vsnprintf(&con_buf[0], CON_BUF_LEN, fmt, arglist);

    va_end(arglist);

    // If we have a space in console buffer
    while ((pos < CON_BUF_LEN) && (i < size)) {
    	// Output supplied array as HEX characters with spaces
        pos += snprintf(&con_buf[pos], (CON_BUF_LEN - pos), "%02X ", data[i++]);
    }

    if (pos >= (CON_BUF_LEN - 3)) {
        snprintf(&con_buf[CON_BUF_LEN-3], 3, "\r\n");
        pos = CON_BUF_LEN - 1;
    }
    else {
        pos += snprintf(&con_buf[pos], CON_BUF_LEN - pos, "\r\n");
    }

#ifdef DEBUG_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
//	app_params.dont_go_stop |= APP_DEBUG_ACTIVE;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DBG_UART, (uint8_t *) con_buf, pos);
//    HAL_UART_Transmit_IT(&DBG_UART, (uint8_t *) con_buf, pos);
#endif
}


//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa CON_BUF_LEN where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the short data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DBG_msg_fmt_buf16(const uint16_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_UART
    // Wait for UART is ready for start new transmission
    while (DBG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = vsnprintf(&con_buf[0], CON_BUF_LEN, fmt, arglist);

    va_end(arglist);


    // If we have a space in console buffer
    while ((pos < CON_BUF_LEN) && (i < size)) {
    	// Output supplied array as HEX characters with spaces
        pos += snprintf(&con_buf[pos], (CON_BUF_LEN - pos), "%04X ", data[i++]);
    }

    if (pos >= (CON_BUF_LEN - 3)) {
        snprintf(&con_buf[CON_BUF_LEN-3], 3, "\r\n");
        pos = CON_BUF_LEN - 1;
    }
    else {
        pos += snprintf(&con_buf[pos], CON_BUF_LEN - pos, "\r\n");
    }

#ifdef DEBUG_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
//	app_params.dont_go_stop |= APP_DEBUG_ACTIVE;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DBG_UART, (uint8_t *) con_buf, pos);
//    HAL_UART_Transmit_IT(&DBG_UART, (uint8_t *) con_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa CON_BUF_LEN where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the word data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DBG_msg_fmt_buf32(const uint32_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_UART
    // Wait for UART is ready for start new transmission
    while (DBG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = vsnprintf(&con_buf[0], CON_BUF_LEN, fmt, arglist);

    va_end(arglist);


    // If we have a space in console buffer
    while ((pos < CON_BUF_LEN) && (i < size)) {
#ifdef DUMP_AS_HEX
    	// Output supplied array as HEX characters with spaces
        pos += snprintf(&con_buf[pos], (CON_BUF_LEN - pos), "%08lX ", data[i++]);
#else
    	pos += snprintf(&con_buf[pos], (CON_BUF_LEN - pos), "%08ld ", data[i++]);
#endif
    }

    if (pos >= (CON_BUF_LEN - 3)) {
        snprintf(&con_buf[CON_BUF_LEN-3], 3, "\r\n");
        pos = CON_BUF_LEN - 1;
    }
    else {
        pos += snprintf(&con_buf[pos], CON_BUF_LEN - pos, "\r\n");
    }

#ifdef DEBUG_UART
    // Output data to UART
    HAL_UART_Transmit_DMA(&DBG_UART, (uint8_t *) con_buf, pos);
//    HAL_UART_Transmit_IT(&DBG_UART, (uint8_t *) con_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Initialize the modules debug configuration by supplied structure or set to default state
 *
 *  Function will initialize memory structure \sa DBG_CONFIG with supplied configuration
 *  or reset to default state if supplid configuration pointer is zero
 *
 *  \param  pDbgCfg Ponter to the \sa DBG_CONFIG logging configuration structure
 *  \return         At this time always return 0
 */
int DBG_init_with_configuration(const DBG_CONFIG* pDbgCfg)
{
	// Structures has compatible types and do not contain strings
	// So copy structures to the memory via simple "memcpy"

	// If supplied configuration is empty
    if (pDbgCfg == 0) {
    	// Copy default logging setting structure to the memory
    	memcpy(&log_config, &DBG_default_cfg, sizeof(DBG_CONFIG));
    }
    else {
    	// Copy supplied logging structure to the memory
        memcpy(&log_config, pDbgCfg, sizeof(DBG_CONFIG));
    }

    return 0;
}

//-----------------------------------------------------------------------------
/** Callback for TX complete event
 *
 * This callback is called when all data transfered by the debug UART
 */
void DBG_TX_Complete_Callback(void)
{
	// Reset flag that UART still in the transmission process
	dbg_uart_status &= (~DBG_TX_IN_PROCESS);
	// Now DEBUG control logic allow application to put MCU into the STOP mode
//	app_params.dont_go_stop &= ~APP_DEBUG_ACTIVE;
}
