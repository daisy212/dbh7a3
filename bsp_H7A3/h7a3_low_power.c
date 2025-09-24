/* Helper for entering stop mode 2 
 * stop mode 2 : stm32h7b3 only : <50µA


*/

#include "h7a3_low_power.h"
#include "h7a3_rtc.h"
#include "h7a3_kbd.h"
#include "h7a3_usb.h"
#include "h7a3_Sharp_LS.h"

#include "SEGGER_RTT.h" // using SEGGER_RTT_printf


t_POWER_STATE db_power_state = PW_power_on;

extern OS_EVENT      RTC_Event, PB_Event, KBD_Event, USB_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;

// HAL error
void    Error_Handler(int err_no){
   RTT_vprintf_cr_time("\nErr Hal : %d", err_no);
   while (1){
   
   }
}




void init_unused_pins(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

// PA13, PA14, PA15 : debug
// PA11, PA12 : usb

// port B,led1 PB0
// PB3 : debug
   __HAL_RCC_GPIOB_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   //GPIO_InitStruct.Pin = GPIO_PIN_All;
   GPIO_InitStruct.Pin = GPIO_PIN_All & (!GPIO_PIN_0);       //==> conso 150µA !!!!!!!!!
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   __HAL_RCC_GPIOB_CLK_DISABLE();

// port C, button on PC13
   __HAL_RCC_GPIOC_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All & (!GPIO_PIN_13);
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
   __HAL_RCC_GPIOC_CLK_DISABLE();

// port D
   __HAL_RCC_GPIOD_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   __HAL_RCC_GPIOD_CLK_DISABLE();

// port E
   __HAL_RCC_GPIOE_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;
   HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
   __HAL_RCC_GPIOE_CLK_DISABLE();

// port F
   __HAL_RCC_GPIOF_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;
   HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
   __HAL_RCC_GPIOF_CLK_DISABLE();

// port G
   __HAL_RCC_GPIOG_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;
   HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
   __HAL_RCC_GPIOG_CLK_DISABLE();

   // Disable VREFBUF if enabled
   HAL_SYSCFG_DisableVREFBUF();

}


void disable_debug(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   // debug pins
   GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//   GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
//   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

   // Clear DBGMCU_CR register to disable debug in all low power modes, 
   // debug : 1.2mA
   DBGMCU->CR = 0x00000000;

}

void ready_to_stop2_periph(void)
{

// lcd spi1
//   SHARP_SPI1_DeInit(&hlcd);


// sdmmc1
//    __HAL_RCC_SDMMC1_CLK_DISABLE();


   // Disable VREFBUF if enabled
   HAL_SYSCFG_DisableVREFBUF();

// Disable all clocks not needed
__HAL_RCC_LSI_DISABLE();
__HAL_RCC_HSI48_DISABLE();
__HAL_RCC_USART3_CLK_DISABLE();
__HAL_RCC_USART1_CLK_DISABLE();
__HAL_RCC_USART3_CLK_DISABLE();


//        HAL_SuspendTick();
SysTick->CTRL = 0;  // Stops counter and clock
SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;  // Clear pending

// check timer 7, HAL timer
//__HAL_RCC_TIM7_CLK_SLEEP_ENABLE();


   HAL_SuspendTick();


HAL_PWREx_EnableFlashPowerDown();

// enable low power regulator : done
// utile ???
//HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);

}

void ready_to_stop2_gpio(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

// debug : PA13 PA14
// usb vbus detect PA9
// kbd row : PA4, PA6, PA15
   __HAL_RCC_GPIOA_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;

   GPIO_InitStruct.Pin = GPIO_PIN_All & !( GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4 | GPIO_PIN_6|GPIO_PIN_15 | GPIO_PIN_9);    // except debug + gpio
//   GPIO_InitStruct.Pin = GPIO_PIN_All ;    // no conso diff
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//   __HAL_RCC_GPIOA_CLK_DISABLE();

// drivers led  : PB0, PB14 ==> conso 150µA !!!!!!!!!
// kbd col : PB4, PB5, PB8, PB9, PB12, PB15
// kbd row : PB3, PB13
   __HAL_RCC_GPIOB_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_14 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 |GPIO_PIN_11 ;    // conso ok  : 30µA
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   __HAL_RCC_GPIOB_CLK_DISABLE();

// sdmmc1 : PC8, PC9, PC10, PC11, PC12
// button : PC13
// kbd row : PC6, PC7
   __HAL_RCC_GPIOC_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All & !(GPIO_PIN_13|GPIO_PIN_6|GPIO_PIN_7);
//   GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;      
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
   __HAL_RCC_GPIOC_CLK_DISABLE();

// sdmmc1 : PD2
// Card detect sdmmc : PD7
// kbd row : PD15
   __HAL_RCC_GPIOD_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All & !(GPIO_PIN_15 | GPIO_PIN_7);      
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin =  GPIO_PIN_7;      
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   __HAL_RCC_GPIOD_CLK_DISABLE();

// led PE1
   __HAL_RCC_GPIOE_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;      
   HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
   __HAL_RCC_GPIOE_CLK_DISABLE();

   __HAL_RCC_GPIOF_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All;      
   HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
   __HAL_RCC_GPIOF_CLK_DISABLE();

   // port G
// kbd row : PG9
   __HAL_RCC_GPIOG_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_All & !(GPIO_PIN_9);
   HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
   __HAL_RCC_GPIOG_CLK_DISABLE();

// spi lcd

// kbd


}





void    Error_Handler(int err_no);





/**
  * @brief System Clock Configuration
  * @retval None
  */


void SystemClock_Config_P64(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /*AXI clock gating */
  RCC->CKGAENR = 0xE003FFFF;

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
//  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(12);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler(13);
  }
}

void SystemClock_Config_P120(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /*AXI clock gating */
  RCC->CKGAENR = 0xE003FFFF;

  /** Supply configuration update enable
  */
//  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 15;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(1012);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler(1013);
  }
}




void SystemClock_Config_P280(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /*AXI clock gating */
  RCC->CKGAENR = 0xE003FFFF;

  /** Supply configuration update enable
  */
 // HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);



  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;

  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 35;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
   RTT_vprintf_cr_time( "SysClk setting 280 : error");

//    Error_Handler(12);
  }



  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
   RTT_vprintf_cr_time( "SysClk setting 280 apb : error");
//    Error_Handler(13);
  }

}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config_P16_R64(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDMMC|RCC_PERIPHCLK_SPI2;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 8;
  PeriphClkInitStruct.PLL2.PLL2P = 8;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler(14);
  }
}





void Restore_After_STOP2(void)
{
   HAL_Init();

   if (Usb_Detect())
      SystemClock_Config_P280();
   else
      SystemClock_Config_P120();

   PeriphCommonClock_Config_P16_R64();

   // low power, to do
//   SystemClock_Config_P64();
//   PeriphCommonClock_Config_P16_R0();

//   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOD_CLK_ENABLE();
   __HAL_RCC_GPIOG_CLK_ENABLE();


   HAL_SuspendTick();

}


uint32_t EnterSTOP2(void)
{
   uint32_t sleeping_time = rtc_read_ticks();
   SHARP_SPI1_DeInit(&hlcd);

   ready_to_stop2_gpio();
   ready_to_stop2_periph();
   init_button_pc13();

#if SIMULATE_STOP2
   // simulation stop mode 2
   OS_EVENT_GetTimed(&KBD_Event, 9999999);
   SystemClock_Config_P64();        // cpu using HSI 64Mhz

#else
   #if STOP2_ENABLE_DEBUG
      HAL_DBGMCU_EnableDBGStopMode();
   #else 
      disable_debug();
   #endif // STOP2_ENABLE_DEBUG

   HAL_PWREx_EnterSTOP2Mode( PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);  // 30µA
#endif // SIMULATE_STOP2

   HAL_PWREx_DisableFlashPowerDown();
   Restore_After_STOP2();
   return rtc_elapsed_ticks(sleeping_time);      
}




/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef        htim7;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function configures the TIM7 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock, uwAPB1Prescaler;
  uint32_t              uwPrescalerValue;
  uint32_t              pFLatency;

  /*Configure the TIM7 IRQ priority */
  if (TickPriority < (1UL << __NVIC_PRIO_BITS))
   {
     HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority ,0);

     /* Enable the TIM7 global Interrupt */
     HAL_NVIC_EnableIRQ(TIM7_IRQn);
     uwTickPrio = TickPriority;
    }
  else
  {
    return HAL_ERROR;
  }

  /* Enable TIM7 clock */
  __HAL_RCC_TIM7_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* Get APB1 prescaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;
  /* Compute TIM7 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2UL * HAL_RCC_GetPCLK1Freq();
  }

  /* Compute the prescaler value to have TIM7 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

  /* Initialize TIM7 */
  htim7.Instance = TIM7;

  /* Initialize TIMx peripheral as follow:
   * Period = [(TIM7CLK/1000) - 1]. to have a (1/1000) s time base.
   * Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
   * ClockDivision = 0
   * Counter direction = Up
   */
  htim7.Init.Period = (1000000U / 1000U) - 1U;
  htim7.Init.Prescaler = uwPrescalerValue;
  htim7.Init.ClockDivision = 0;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;

  if(HAL_TIM_Base_Init(&htim7) == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    return HAL_TIM_Base_Start_IT(&htim7);
  }

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM7 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_SuspendTick(void)
{
  /* Disable TIM7 update Interrupt */
  __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM7 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /* Enable TIM7 Update interrupt */
  __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
}

/**
  * @brief This function handles TIM7 global interrupt.
  */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  /* USER CODE END TIM7_IRQn 1 */
}




