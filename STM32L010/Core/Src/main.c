/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "dcf77_bridge.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Imposta a 1 se il tuo modulo DCF77 ha uscita invertita (0=portante, 1=silenzio)
 * Imposta a 0 se uscita normale (1=portante, 0=silenzio)
 * Il modulo HKW/Canaduino tipicamente usa 0 (non invertito) su STM32 */
#define DCF77_INVERTED_SAMPLES  0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t	timer_1ms = 0;
/* --- Variabile globale per il polling (opzionale) --- */
static volatile DCF77_Time_t g_current_time;
static volatile uint8_t      g_time_updated = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Versione con flag: sostituisce dcf77_output_handler sopra se preferisci
 * processare il tempo nel main loop invece che nell'IRQ */
void dcf77_output_handler_with_flag(const DCF77_Time_t *t)
{
    g_current_time = *t;       /* copia atomica per tipi piccoli su Cortex-M */
    g_time_updated = 1;
}

uint8_t dcf77_input_provider(void)
{
	/* Legge il pin DCF77, applica eventuale inversione, aggiorna il LED monitor */
	    uint8_t sampled = (uint8_t)HAL_GPIO_ReadPin(DCF77_GPIO_Port, DCF77_Pin);
	    sampled ^= DCF77_INVERTED_SAMPLES;

	    /* LED monitor: ON quando il segnale è alto (portante presente)
	     * Adatta GPIO_PIN_x e GPIOx al tuo LED monitor */
	    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, sampled ? GPIO_PIN_SET : GPIO_PIN_RESET);

	    return sampled;
}

/* --- Stampa ora su UART (equivalente di paddedPrint + loop) --- */
static void dcf77_print_time(const DCF77_Time_t *t)
{
    char buf[64];

    /* Stato del clock */
    const char *state_str;
    switch (dcf77_bridge_get_clock_state()) {
        case DCF77_STATE_USELESS:  state_str = "useless  "; break;
        case DCF77_STATE_DIRTY:    state_str = "dirty    "; break;
        case DCF77_STATE_FREE:     state_str = "free     "; break;
        case DCF77_STATE_UNLOCKED: state_str = "unlocked "; break;
        case DCF77_STATE_LOCKED:   state_str = "locked   "; break;
        case DCF77_STATE_SYNCED:   state_str = "synced   "; break;
        default:                   state_str = "unknown  "; break;
    }

    /* Formato: "synced   2025-06-15 14:32:01+02" */
    snprintf(buf, sizeof(buf),
             "%s 20%02u-%02u-%02u %02u:%02u:%02u+0%c",
             state_str,
             t->year, t->month, t->day,
             t->hour, t->minute, t->second,
             t->uses_summertime ? '2' : '1');

//    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    APP_DEBUG(DBG_TRACE, buf);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */


  // Messaggio di avvio
  APP_DEBUG(DBG_TRACE,"\r\nSimple DCF77 Clock");
  HAL_Delay(10);
  APP_DEBUG(DBG_TRACE,"Initializing...");

  // Inizializza il bridge DCF77 PRIMA di avviare TIM2
  //dcf77_bridge_setup(dcf77_input_provider, dcf77_output_handler_with_flag);
  dcf77_bridge_setup(dcf77_input_provider, NULL);  // NULL = niente output handler, usiamo polling

  // Avvia TIM2 in modalità interrupt (1 kHz)
  HAL_TIM_Base_Start_IT(&htim2);

  APP_DEBUG(DBG_TRACE, "By MLT using Udo Klein library");

	  uint32_t dot_count = 0;
  {
	  DCF77_ClockState_t state;
	  do {
		  HAL_Delay(1000);
		  HAL_UART_Transmit(&huart2, (uint8_t*)".", 1, HAL_MAX_DELAY);
		  if (!((++dot_count)%60)) {
			  APP_DEBUG(DBG_TRACE,"");
		  }
		  state = dcf77_bridge_get_clock_state();
	  } while (state == DCF77_STATE_USELESS || state == DCF77_STATE_DIRTY);

  }
  APP_DEBUG(DBG_TRACE,"Synced! seconds = %d", dot_count);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  DCF77_Time_t now;
	  dcf77_bridge_read_current_time(&now);

	  if (now.month > 0) {
		  dcf77_print_time(&now);
	  }

	  HAL_Delay(1000);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// Tipycal output redirection. Here I use dam and/or Interrupt
/*
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}
*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        dcf77_bridge_isr_handler();
        timer_1ms++;
    }
}


//--------------------------------------------------------------------
/** UART TX complete interrupt callback processing
 *
 * \param	huart	Pointer to the UART handle for which this callback is executed
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	// If all data from UART output completely to ESP32
	if (huart == &huart2) {

		// Call the TX complete callback from DEBUG communication module
		DBG_TX_Complete_Callback();
	}
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
