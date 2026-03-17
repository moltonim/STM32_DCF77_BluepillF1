/*
 * debug.h
 *
 *  Created on: Jan 23, 2023
 *  Author: KVovk
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

/** Enable reception some characters from console for start some test */
//#define ENABLE_DEBUG_RECEPTION

/** Dump values to debug output as HEX values */
#define	DUMP_AS_HEX

/** Type of logging levels used for output messages */
typedef enum
{
    DBG_TRACE,     /**< Only for "tracing" the code and find part of a the function that cause errors */
    DBG_DEBUG,     /**< Information that is diagnostically helpful to people more than just developers */
    DBG_INFO,      /**< Generally useful information to log service start/stop, configuration assumptions, etc). */
    DBG_WARNING,   /**< Anything that can potentially cause application oddities, such as retrying an operation, missing secondary data, etc */
    DBG_ERROR,     /**< Any error which is fatal to the operation but not the service or application (can't open a required file, missing data, etc). */
    DBG_FATAL,     /**< Any error that is forcing a shutdown of the service or application to prevent data loss (or further data loss). */
    DBG_DISABLED   /**< Totally disabled the logging output, value used internally, do not use. Not exposed to users. */
} LOG_TYPE;


// The unique code of all application modules
// Every file in the project that does logging output should have unique id.
// Based on this id, the output from the specific module can be turned On or Of
// Add ids of new modules here:

/** Application logging modules identifiers */
typedef enum
{
    MODULE_APP,			/**< Application module */
    MODULE_MEASURE,		/**< Measurement (metrologic) module */
    MODULE_ESP32,		/**< ESP32 communication module */
    MODULE_SENSOR,		/**< MAF Sensor module */
    // add new modules above this line
    NUM_DBG_MODULES     /**< The number of defined modules */
} LOG_MODULE;


/** Structure defines the KEY/VALUE structure for logging level of certain module */
typedef struct {
    uint16_t	module_Id;	/**< Module ID based on \sa LOG_MODULE */
    uint16_t	log_Level;	/**< Module logging level */
} DBG_MODULE_INFO;

/** Holds the configuration that contains logging level for all modules in firmware */
typedef struct {
    uint32_t		modulesCount;                   // Number of modules that we choose for debug
    DBG_MODULE_INFO	moduleLogCfg[NUM_DBG_MODULES];  // Logging configuration per module
} DBG_CONFIG;

#define ENABLE_COLOR_TERMINAL	0					/**< Enable ESC ASCII codes for color text in terminal */

#define CHAIN_2_STR(STR1,STR2)		STR1##STR2				/**< Concatenate two strings */
#define MERGE2(S1,S2)				CHAIN_2_STR(STR1,STR2)	/**< Concatenate two strings */
#define CHAIN_3_STR(STR1,STR2,STR3)	STR1##STR2##STR2		/**< Concatenate three strings */
#define MERGE3(S1,S2,S3)			CHAIN_3_STR(STR1,STR2,STR3)	/**< Concatenate three strings */

#if (ENABLE_COLOR_TERMINAL)
	#define RED_COLOR(STR)			"\x1b[31"#STR"\x1b[39;49m"
	#define GREEN_COLOR(STR)		"\x1b[32STR\x1b[39;49m"
	#define YELLOW_COLOR(STR)		"\x1b[33STR\x1b[39;49m"
	#define BLUE_COLOR(STR)			"\x1b[34STR\x1b[39;49m"
	#define MAGENTA_COLOR(STR)		"\x1b[35STR\x1b[39;49m"
	#define CYAN_COLOR(STR)			"\x1b[36STR\x1b[39;49m"
	#define WHITE_COLOR(STR)		"\x1b[37STR\x1b[39;49m"
	#define GRAY_COLOR(STR)			"\x1b[90STR\x1b[39;49m"
#else
	#define RED_COLOR(STR)			STR
	#define GREEN_COLOR(STR)		STR
	#define YELLOW_COLOR(STR)		STR
	#define BLUE_COLOR(STR)			STR
	#define MAGENTA_COLOR(STR)		STR
	#define CYAN_COLOR(STR)			STR
	#define WHITE_COLOR(STR)		STR
	#define GRAY_COLOR(STR)			STR
#endif

int DBG_is_enabled(LOG_MODULE module, LOG_TYPE log_lvl);
void DBG_msg_fmt(const char* fmt, ...);
void DBG_msg_fmt_buf(const uint8_t* data, uint32_t size, const char* fmt, ...);
void DBG_msg_fmt_buf16(const uint16_t* data, uint32_t size, const char* fmt, ...);
void DBG_msg_fmt_buf32(const uint32_t* data, uint32_t size, const char* fmt, ...);

// Global macros used for enable debug log's
// If this macros are commented (or definitions are removed) the logging features for module(s) are removed from code and it consume less space */
#define APPLICATION_DEBUG_ENABLE        /**< Enable Application debug messages */
//#define MEASURE_DEBUG_ENABLE			/**< Enable Measurement (metrologic) debug messages */
//#define ESP32_DEBUG_ENABLE				/**< Enable ESP32 communication debug messages */
//#define SENSOR_DEBUG_ENABLE				/**< Enable MAF sensor debug messages */

// Main macros for debug output for APPLICATION module
#ifdef APPLICATION_DEBUG_ENABLE
    #define APP_DEBUG(Level, ...)	            do {if(DBG_is_enabled(MODULE_APP, Level)) DBG_msg_fmt(__VA_ARGS__);} while (0)
    #define APP_DUMP(Level, buf, len, ...)      do {if(DBG_is_enabled(MODULE_APP, Level)) DBG_msg_fmt_buf(buf, len,  __VA_ARGS__);} while (0)
	#define APP_DUMP16(Level, buf, len, ...)    do {if(DBG_is_enabled(MODULE_APP, Level)) DBG_msg_fmt_buf16(buf, len,  __VA_ARGS__);} while (0)
	#define APP_DUMP32(Level, buf, len, ...)	do {if(DBG_is_enabled(MODULE_APP, Level)) DBG_msg_fmt_buf32(buf, len,  __VA_ARGS__);} while (0)
#else
    #define APP_DEBUG(Level, ...)
    #define APP_DUMP(Level, buf, len, ...)
	#define APP_DUMP32(Level, buf, len, ...)
#endif	// APPLICATION_DEBUG_ENABLE

int DBG_init_with_configuration(const DBG_CONFIG* pDbgCfg);
void DBG_TX_Complete_Callback(void);

#ifdef ENABLE_DEBUG_RECEPTION
void DBG_Process_Received_Data(void);
void DBG_RX_Callback(uint16_t Size);
void DBG_RX_Start(void);
#endif // ENABLE_DEBUG_RECEPTION

#endif /* INC_DEBUG_H_ */
