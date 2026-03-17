/**
 * @file    dcf77_bridge.cpp
 * @brief   Implementazione del bridge C → C++ per la libreria DCF77 (Udo Klein).
 *
 * Questo file DEVE essere compilato come C++ (estensione .cpp).
 * CubeIDE lo gestisce automaticamente in base all'estensione.
 *
 * Dipendenze di build (Core/Src e Core/Inc devono contenere):
 *   - dcf77_hal_adapter.h   ← include PRIMA di dcf77.h
 *   - dcf77.h / dcf77.cpp   ← libreria originale (aggiungere dcf77.cpp al build)
 *   - dcf77_bridge.h        ← header pubblico C
 *
 * Configurazione timer assunta (da CubeMX / main.c):
 *   - TIM2: APB1 timer clock = 8 MHz
 *           Prescaler = 8-1  → tick clock = 1 MHz
 *           Period    = 1000-1 → overflow ogni 1 ms  → IRQ @ 1 kHz
 *   - HAL_TIM_Base_Start_IT(&htim2) chiamato in main() dopo dcf77_bridge_setup()
 *
 * Correzione di fase:
 *   La libreria originale implementa la correzione di fase modificando il
 *   periodo del timer di ±1 µs. Con TIM2 a 1 MHz, 1 conteggio = 1 µs, quindi:
 *     - aggiungi 1 conteggio → periodo = 1001 µs  (rallenta)
 *     - togli 1 conteggio   → periodo = 999  µs  (accelera)
 *   Si usa __HAL_TIM_SET_AUTORELOAD() per modificare ARR al volo.
 *
 *   inverse_timer_resolution = sysclock_Hz / 1000 / ticks_per_ms_adjust
 *   Con sysclock = 8 MHz e aggiustamento a ±1 µs:
 *     inverse_timer_resolution = 8000
 *   (la libreria usa 16000 per 16 MHz; noi scalamo a 8000 per 8 MHz)
 */

/* -----------------------------------------------------------------------
 * ORDINE DI INCLUDE CRITICO:
 *   1. dcf77_hal_adapter.h  → definisce sprint/sprintln/F()/CRITICAL_SECTION
 *   2. dcf77.h              → usa quelle macro, NON include Arduino.h
 * ----------------------------------------------------------------------- */
#include "dcf77_hal_adapter.h"
#include "dcf77.h"
#include "dcf77_bridge.h"

/* -----------------------------------------------------------------------
 * Handle timer esterno (definito in tim.c da CubeMX)
 * ----------------------------------------------------------------------- */
extern TIM_HandleTypeDef htim2;

/* -----------------------------------------------------------------------
 * Costanti legate all'hardware
 *
 * Con APB1 timer clock = 8 MHz, prescaler = 8-1:
 *   timer tick = 1 µs, overflow @ 1000 tick = 1 ms
 *
 * inverse_timer_resolution: quante unità di adjust_pp16m corrispondono
 * a un passo di correzione di 1 µs.
 *
 *   Formula:  INVERSE_TIMER_RES = SYSCLK_Hz / 1000
 *
 *   SYSCLK =  8 MHz  →  8000   (HSE diretto, configurazione base)
 *   SYSCLK = 16 MHz  → 16000   (come il BluePill originale)
 *   SYSCLK = 32 MHz  → 32000   (max L010RB con PLL)
 *
 * Se cambi SYSCLK, aggiorna INVERSE_TIMER_RES e il Prescaler di TIM2
 * in CubeMX (Prescaler = SYSCLK_MHz - 1) per mantenere tick = 1 µs.
 * ARR = 999 rimane invariato in tutti i casi (overflow sempre a 1 ms).
 *
 * STM32L010RB: Cortex-M0+, 128 KB Flash, 20 KB RAM → modalità ms OK.
 * ----------------------------------------------------------------------- */
static const uint16_t TIMER_PERIOD_NOMINAL = 999U;   /* ARR per 1 ms esatto */
static const uint16_t TIMER_PERIOD_FAST    = 998U;   /* ARR - 1 µs */
static const uint16_t TIMER_PERIOD_SLOW    = 1000U;  /* ARR + 1 µs */
static const int32_t  INVERSE_TIMER_RES    = 8000;   /* pp16m per 1 µs @ 8 MHz */

/* -----------------------------------------------------------------------
 * Stato interno del bridge
 * ----------------------------------------------------------------------- */
namespace {
    /* Accumulatore di deriva di fase (stesso algoritmo del codice STM32F1
     * originale, adattato a HAL puro senza HardwareTimer di Arduino) */
    volatile int32_t cumulated_phase_deviation = 0;

    /* Puntatori ai callback forniti dall'utente C */
    DCF77_InputProvider_t user_input_provider = nullptr;
    DCF77_OutputHandler_t user_output_handler = nullptr;

    /* -----------------------------------------------------------------------
     * Output handler interno: converte Clock::time_t → DCF77_Time_t (C)
     * e chiama il callback utente.
     * ----------------------------------------------------------------------- */
    void internal_output_handler(const Clock::time_t &t)
    {
        if (user_output_handler == nullptr) return;

        DCF77_Time_t out;
        out.year     = BCD::bcd_to_int(t.year);
        out.month    = BCD::bcd_to_int(t.month);
        out.day      = BCD::bcd_to_int(t.day);
        out.weekday  = BCD::bcd_to_int(t.weekday);
        out.hour     = BCD::bcd_to_int(t.hour);
        out.minute   = BCD::bcd_to_int(t.minute);
        out.second   = BCD::bcd_to_int(t.second);
        out.uses_summertime           = t.uses_summertime           ? 1u : 0u;
        out.timezone_change_scheduled = t.timezone_change_scheduled ? 1u : 0u;
        out.leap_second_scheduled     = t.leap_second_scheduled     ? 1u : 0u;

        user_output_handler(&out);
    }

    /* -----------------------------------------------------------------------
     * Input provider interno: adattatore per la firma C++ della libreria
     * ----------------------------------------------------------------------- */
    uint8_t internal_input_provider(void)
    {
        if (user_input_provider == nullptr)
        	return 0;
        return user_input_provider();
    }

    /* -----------------------------------------------------------------------
     * Helper: converte Clock::time_t → DCF77_Time_t senza callback
     * ----------------------------------------------------------------------- */
    void clock_time_to_bridge(const Clock::time_t &src, DCF77_Time_t *dst)
    {
        dst->year     = BCD::bcd_to_int(src.year);
        dst->month    = BCD::bcd_to_int(src.month);
        dst->day      = BCD::bcd_to_int(src.day);
        dst->weekday  = BCD::bcd_to_int(src.weekday);
        dst->hour     = BCD::bcd_to_int(src.hour);
        dst->minute   = BCD::bcd_to_int(src.minute);
        dst->second   = BCD::bcd_to_int(src.second);
        dst->uses_summertime           = src.uses_summertime           ? 1u : 0u;
        dst->timezone_change_scheduled = src.timezone_change_scheduled ? 1u : 0u;
        dst->leap_second_scheduled     = src.leap_second_scheduled     ? 1u : 0u;
    }
} /* namespace anonimo */

/* -----------------------------------------------------------------------
 * Implementazione API pubblica bridge  (extern "C")
 * ----------------------------------------------------------------------- */

extern "C" {

/* ---------------------------------------------------------------------- */
void dcf77_bridge_setup(DCF77_InputProvider_t input_provider,
                        DCF77_OutputHandler_t output_handler)
{
    user_input_provider = input_provider;
    user_output_handler = output_handler;

    cumulated_phase_deviation = 0;

    /* Inizializza la libreria DCF77 con i wrapper interni */
    DCF77_Clock::setup(internal_input_provider,
                       (output_handler != nullptr) ? internal_output_handler : nullptr);

    /*
     * NON avviamo TIM2 qui: lo fa il codice C dell'utente dopo questa
     * chiamata con HAL_TIM_Base_Start_IT(&htim2).
     * Questo permette di configurare ulteriori periferiche prima che
     * inizino gli interrupt del timer.
     */
}

/* ---------------------------------------------------------------------- */
void dcf77_bridge_isr_handler(void)
{
    /*
     * Correzione di fase: uguale all'algoritmo STM32F1 originale ma
     * usando __HAL_TIM_SET_AUTORELOAD() invece di systick_init().
     *
     * adjust_pp16m è letto tramite Generic_1_kHz_Generator::read_adjustment()
     * che è thread-safe (usa CRITICAL_SECTION internamente).
     *
     * Nota: questa funzione è chiamata dal contesto IRQ (TIM2), quindi
     * non si usano lock aggiuntivi qui.
     */
    const int16_t adjust_pp16m = Internal::Generic_1_kHz_Generator::read_adjustment();

    cumulated_phase_deviation += adjust_pp16m;

    if (cumulated_phase_deviation >= INVERSE_TIMER_RES) {
        cumulated_phase_deviation -= INVERSE_TIMER_RES;
        /* Deriva accumulata > 1 µs → accorcia il prossimo periodo di 1 µs */
        __HAL_TIM_SET_AUTORELOAD(&htim2, TIMER_PERIOD_FAST);
    } else if (cumulated_phase_deviation <= -INVERSE_TIMER_RES) {
        cumulated_phase_deviation += INVERSE_TIMER_RES;
        /* Deriva accumulata < -1 µs → allunga il prossimo periodo di 1 µs */
        __HAL_TIM_SET_AUTORELOAD(&htim2, TIMER_PERIOD_SLOW);
    } else {
        /* Nessuna correzione */
        __HAL_TIM_SET_AUTORELOAD(&htim2, TIMER_PERIOD_NOMINAL);
    }

    /* Alimenta il tick alla libreria DCF77 */
    Internal::Generic_1_kHz_Generator::Clock_Controller::process_1_kHz_tick_data(internal_input_provider());
}

/* ---------------------------------------------------------------------- */
void dcf77_bridge_read_current_time(DCF77_Time_t *out)
{
    if (out == nullptr) return;
    Clock::time_t t;
    DCF77_Clock::read_current_time(t);
    clock_time_to_bridge(t, out);
}

/* ---------------------------------------------------------------------- */
void dcf77_bridge_read_future_time(DCF77_Time_t *out)
{
    if (out == nullptr) return;
    Clock::time_t t;
    DCF77_Clock::read_future_time(t);
    clock_time_to_bridge(t, out);
}

/* ---------------------------------------------------------------------- */
uint8_t dcf77_bridge_get_overall_quality_factor(void)
{
    return DCF77_Clock::get_overall_quality_factor();
}

/* ---------------------------------------------------------------------- */
DCF77_ClockState_t dcf77_bridge_get_clock_state(void)
{
    return static_cast<DCF77_ClockState_t>(DCF77_Clock::get_clock_state());
}

/* ---------------------------------------------------------------------- */
uint8_t dcf77_bridge_get_prediction_match(void)
{
    return DCF77_Clock::get_prediction_match();
}

/* ---------------------------------------------------------------------- */
void dcf77_bridge_debug(void)
{
    DCF77_Clock::debug();
}

} /* extern "C" */
