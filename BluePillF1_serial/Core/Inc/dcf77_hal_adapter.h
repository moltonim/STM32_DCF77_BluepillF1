/**
 * @file    dcf77_hal_adapter.h
 * @brief   Compatibility layer: sostituisce le dipendenze Arduino della libreria
 *          DCF77 (Udo Klein) con chiamate HAL STM32.
 *
 * Approccio: funzioni con nomi univoci, zero overloading, zero _Generic.
 * I file dcf77.h e dcf77.cpp usano queste funzioni direttamente dopo la
 * sostituzione testuale delle chiamate sprint/sprintln originali con:
 *   dcf_putc(v)         -> stampa un carattere  (int -> cast a char)
 *   dcf_puts(s)         -> stampa una stringa   (const char*)
 *   dcf_putn(v, base)   -> stampa un numero     (int32_t, base DEC/HEX/BIN)
 *   dcf_putnl()         -> stampa solo \r\n
 *   dcf_putsln(s)       -> stampa stringa + \r\n
 *   dcf_putnln(v, base) -> stampa numero  + \r\n
 */

#ifndef DCF77_HAL_ADAPTER_H
#define DCF77_HAL_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"      /* HAL, periferiche generate da CubeMX */
#include "usart.h"
#include "tim.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

/* -----------------------------------------------------------------------
 * UART di debug - handle generato da CubeMX (tipicamente huart2)
 * ----------------------------------------------------------------------- */
extern UART_HandleTypeDef huart2;

/* -----------------------------------------------------------------------
 * Costanti base numerica (come in Arduino)
 * ----------------------------------------------------------------------- */
#define DEC 10
#define HEX 16
#define BIN  2
#define OCT  8

/* -----------------------------------------------------------------------
 * F("stringa") - no-op su ARM (le stringhe sono gia in Flash/RAM)
 * ----------------------------------------------------------------------- */
#define F(s) (s)

/* -----------------------------------------------------------------------
 * Funzioni primitive di output
 * ----------------------------------------------------------------------- */
static inline void _dcf_tx(const uint8_t *buf, uint16_t len)
{
    //HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);
	APP_DEBUG(DBG_TRACE, (const char*) buf);
}

static inline void _dcf_crlf(void)
{
    const uint8_t crlf[2] = {'\r', '\n'};
    _dcf_tx(crlf, 2);
}

/* -----------------------------------------------------------------------
 * dcf_putc - stampa un carattere
 * La libreria chiama: dcf_putc('x')  oppure  dcf_putc(expr ? 'a' : 'b')
 * In C++ i char literal hanno tipo int; il cast a char e sicuro.
 * ----------------------------------------------------------------------- */
static inline void dcf_putc(int c)
{
    uint8_t ch = (uint8_t)(char)c;
    _dcf_tx(&ch, 1);
}

/* -----------------------------------------------------------------------
 * dcf_puts - stampa una stringa C (usata con F("..."))
 * ----------------------------------------------------------------------- */
static inline void dcf_puts(const char *s)
{
    if (s) _dcf_tx((const uint8_t*)s, (uint16_t)strlen(s));
}

/* -----------------------------------------------------------------------
 * dcf_putn - stampa un intero nella base indicata
 * ----------------------------------------------------------------------- */
static inline void dcf_putn(int32_t v, int base)
{
    char buf[36];
    if (base == 16) {
        snprintf(buf, sizeof(buf), "%lX", (unsigned long)(uint32_t)v);
    } else if (base == 2) {
        int i = 0;
        uint32_t u = (uint32_t)v;
        if (u == 0) {
            buf[i++] = '0';
        } else {
            int bit;
            for (bit = 31; bit >= 0 && !((u >> bit) & 1u); --bit) {}
            for (; bit >= 0; --bit) buf[i++] = (char)('0' + ((u >> bit) & 1u));
        }
        buf[i] = '\0';
    } else if (base == 8) {
        snprintf(buf, sizeof(buf), "%lo", (unsigned long)(uint32_t)v);
    } else {
        snprintf(buf, sizeof(buf), "%ld", (long)v);
    }
    _dcf_tx((const uint8_t*)buf, (uint16_t)strlen(buf));
}

/* -----------------------------------------------------------------------
 * Varianti con newline
 * ----------------------------------------------------------------------- */
static inline void dcf_putnl(void)              { _dcf_crlf(); }
static inline void dcf_putsln(const char *s)    { dcf_puts(s); _dcf_crlf(); }
static inline void dcf_putnln(int32_t v, int b) { dcf_putn(v, b); _dcf_crlf(); }

/* -----------------------------------------------------------------------
 * CRITICAL_SECTION - disabilita/riabilita IRQ su ARM Cortex-M0+/M3/M4
 * Gia definita in dcf77.h per __arm__; ridefinita qui come fallback.
 * ----------------------------------------------------------------------- */
#ifndef CRITICAL_SECTION
static inline int __dcf_disable_irq(void) {
    int primask;
    __asm volatile("mrs %0, PRIMASK\n" : "=r"(primask));
    __asm volatile("cpsid i\n");
    return primask & 1;
}
static inline void __dcf_restore_irq(int *primask) {
    if (!(*primask)) {
        __asm volatile("" ::: "memory");
        __asm volatile("cpsie i\n");
    }
}
#define CRITICAL_SECTION \
    for (int _ps __attribute__((__cleanup__(__dcf_restore_irq))) \
             = __dcf_disable_irq(), _n = 1; _n; _n = 0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DCF77_HAL_ADAPTER_H */
