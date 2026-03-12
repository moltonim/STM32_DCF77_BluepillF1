/**
 * @file    dcf77_bridge.h
 * @brief   Bridge C → C++: espone la libreria DCF77 (Udo Klein) come API C pura.
 *
 * Include questo header nel tuo codice C (main.c, app.c, ecc.).
 * NON include mai dcf77.h direttamente dal codice C.
 *
 * Struttura bridge:
 *
 *   C code (main.c)
 *       │  #include "dcf77_bridge.h"
 *       │  dcf77_bridge_setup(...)
 *       │  dcf77_bridge_get_time(...)
 *       ▼
 *   dcf77_bridge.cpp   [compilato come C++]
 *       │  #include "dcf77_hal_adapter.h"   ← sostituisce Arduino
 *       │  #include "dcf77.h"               ← libreria originale
 *       │  extern "C" { ... }
 *       ▼
 *   dcf77.cpp / dcf77.h   [libreria originale, invariata]
 *
 * -----------------------------------------------------------------------
 * Uso tipico in main.c:
 *
 *   #include "dcf77_bridge.h"
 *
 *   // Callback chiamata dalla libreria ogni secondo (quando in sync)
 *   void my_output_handler(const DCF77_Time_t *t) {
 *       // usa t->hour, t->minute, t->second, ecc.
 *   }
 *
 *   // Input provider: chiamato ogni ms dalla libreria (dal contesto IRQ TIM2)
 *   uint8_t my_input_provider(void) {
 *       return HAL_GPIO_ReadPin(DCF77_GPIO_Port, DCF77_Pin) ? 1 : 0;
 *   }
 *
 *   int main(void) {
 *       // ... init HAL, clock, periferiche ...
 *       dcf77_bridge_setup(my_input_provider, my_output_handler);
 *       while (1) {
 *           // la libreria è interrupt-driven; qui puoi leggere il tempo
 *           DCF77_Time_t now;
 *           dcf77_bridge_read_current_time(&now);
 *           uint8_t quality = dcf77_bridge_get_overall_quality_factor();
 *           DCF77_ClockState_t state = dcf77_bridge_get_clock_state();
 *       }
 *   }
 *
 *   // In stm32xx_it.c – callback IRQ TIM2 (overflow ogni 1 ms):
 *   void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
 *       if (htim->Instance == TIM2) {
 *           dcf77_bridge_isr_handler();
 *       }
 *   }
 * -----------------------------------------------------------------------
 */

#ifndef DCF77_BRIDGE_H
#define DCF77_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Struttura tempo – specchio piatto di Clock::time_t (BCD → valori interi)
 * ----------------------------------------------------------------------- */
typedef struct {
    uint8_t year;       /**< Anno 0..99 (es. 25 = 2025) */
    uint8_t month;      /**< Mese 1..12 */
    uint8_t day;        /**< Giorno 1..31 */
    uint8_t weekday;    /**< Giorno settimana: Lunedì=1 ... Domenica=7 */
    uint8_t hour;       /**< Ora 0..23 */
    uint8_t minute;     /**< Minuto 0..59 */
    uint8_t second;     /**< Secondo 0..60 (60 = leap second) */
    uint8_t uses_summertime;            /**< 1 = ora legale (CEST) */
    uint8_t timezone_change_scheduled;  /**< 1 = cambio orario imminente */
    uint8_t leap_second_scheduled;      /**< 1 = secondo intercalare imminente */
} DCF77_Time_t;

/* -----------------------------------------------------------------------
 * Stato del clock – specchio di Clock::clock_state_t
 * ----------------------------------------------------------------------- */
typedef enum {
    DCF77_STATE_USELESS  = 0,  /**< Segnale insufficiente */
    DCF77_STATE_DIRTY    = 1,  /**< Ora disponibile ma inaffidabile */
    DCF77_STATE_FREE     = 2,  /**< Liberamente in marcia (era sincronizzato) */
    DCF77_STATE_UNLOCKED = 3,  /**< Pronto a risincronizzarsi */
    DCF77_STATE_LOCKED   = 4,  /**< Phase lock acquisita */
    DCF77_STATE_SYNCED   = 5   /**< Sincronizzato completamente */
} DCF77_ClockState_t;

/* -----------------------------------------------------------------------
 * Tipi dei callback – stessa firma dell'API C++ originale
 * ----------------------------------------------------------------------- */

/**
 * @brief  Output handler: chiamato dalla libreria una volta al secondo
 *         quando il clock è sufficientemente stabile.
 * @param  t  Puntatore alla struttura tempo decodificata (stack temporaneo,
 *            copiare i dati se servono oltre la durata del callback).
 */
typedef void (*DCF77_OutputHandler_t)(const DCF77_Time_t *t);

/**
 * @brief  Input provider: chiamato ogni ms (dal contesto IRQ TIM2).
 * @return Livello logico del segnale DCF77: 1 = alto, 0 = basso.
 */
typedef uint8_t (*DCF77_InputProvider_t)(void);

/* -----------------------------------------------------------------------
 * API pubblica del bridge
 * ----------------------------------------------------------------------- */

/**
 * @brief  Inizializza la libreria DCF77.
 *         Da chiamare dopo che HAL, clock e periferiche sono inizializzate,
 *         ma PRIMA di avviare il timer TIM2.
 *
 * @param  input_provider  Funzione C che legge il pin DCF77 (chiamata ogni ms).
 * @param  output_handler  Funzione C chiamata ogni secondo con l'ora decodificata
 *                         (può essere NULL se si preferisce il polling con
 *                         dcf77_bridge_read_current_time).
 */
void dcf77_bridge_setup(DCF77_InputProvider_t input_provider,
                        DCF77_OutputHandler_t output_handler);

/**
 * @brief  ISR handler del timer 1 kHz.
 *         Da chiamare da HAL_TIM_PeriodElapsedCallback quando htim->Instance == TIM2.
 *         Questa funzione chiama internamente process_1_kHz_tick_data() e gestisce
 *         la correzione di fase.
 */
void dcf77_bridge_isr_handler(void);

/**
 * @brief  Legge l'ora corrente (non bloccante).
 *         Equivale a DCF77_Clock::read_current_time().
 * @param  out  Struttura da riempire con l'ora attuale.
 */
void dcf77_bridge_read_current_time(DCF77_Time_t *out);

/**
 * @brief  Legge l'ora corrente + 1 secondo (non bloccante, per sincronizzazione).
 *         Equivale a DCF77_Clock::read_future_time().
 * @param  out  Struttura da riempire.
 */
void dcf77_bridge_read_future_time(DCF77_Time_t *out);

/**
 * @brief  Restituisce il fattore di qualità globale del segnale DCF77.
 * @return 0..255 (0 = nessun segnale, 255 = qualità massima).
 */
uint8_t dcf77_bridge_get_overall_quality_factor(void);

/**
 * @brief  Restituisce lo stato interno del clock.
 * @return Uno dei valori DCF77_ClockState_t.
 */
DCF77_ClockState_t dcf77_bridge_get_clock_state(void);

/**
 * @brief  Restituisce la qualità della predizione (match del segnale vs. clock locale).
 * @return 0..50 (0xFF = non disponibile).
 */
uint8_t dcf77_bridge_get_prediction_match(void);

/**
 * @brief  Stampa debug sulla UART (usa sprint/sprintln interni).
 *         Equivale a DCF77_Clock::debug().
 */
void dcf77_bridge_debug(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DCF77_BRIDGE_H */
