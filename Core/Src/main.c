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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define LOG_TAG "WIRELESS"

#include "elog.h"
#include <stdio.h>
#include <string.h>
#include "dht11.h"
#include "cmsis_os.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t rx_data[100];
volatile uint8_t uart1_rev_finish;
typedef struct {
    uint8_t temp;
    uint8_t hu;
} htsensor_t;

htsensor_t g_htsensor = {0};

typedef struct htsensorvalue_t {
    uint8_t temp;
    uint8_t humi;
} htsensorvalue_t;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_DMA_Init(void);

static void MX_USART1_UART_Init(void);

static void MX_TIM4_Init(void);

/* USER CODE BEGIN PFP */
extern uint32_t os_time;

uint32_t HAL_GetTick(void) {
    return os_time;
}//支持DHT11所需tick功能
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len)//Clion用(setvbuf禁用缓冲)
{
    HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, 100);
    return len;
}

//int fputc(int ch, FILE *f)//Keil5用(use microlib)
//{
//    HAL_UART_Transmit(&huart1, (uint8_t *) &ch, 1, 10);
//    return ch;
//}

void LEDShow_Thread(void const *arg);

osThreadDef (LEDShow_Thread, osPriorityNormal, 1, 0);
osThreadId tid_led_show_thread;

void UART1_Send_Thread(void const *arg);

osThreadDef (UART1_Send_Thread, osPriorityNormal, 1, 0);
osThreadId tid_uart_send_thread;

void TempHumi_Collect_Thread(void const *arg);

osThreadDef (TempHumi_Collect_Thread, osPriorityNormal, 1, 0);
osThreadId tid_temphumi_collect_thread;

osMessageQDef(msgq_LEDShow_Thread, 3, uint32_t);
osMessageQId (msgq_LEDShow_Thread_id);

osMessageQDef(msgq_UART1_Send_Thread, 3, uint32_t);
osMessageQId (msgq_UART1_Send_Thread_id);

osMailQDef(htsensorvalue_uart1_q, 10, htsensorvalue_t);
osMailQId(htsensorvalue_uart1_q_id);

osMailQDef(htsensorvalue_led_q, 10, htsensorvalue_t);
osMailQId(htsensorvalue_led_q_id);

osMutexDef (uart_mutex);
osMutexId (uart_mutex_id);

void collect_temphumi_timer_callback(void const *arg);

osTimerDef(collect_temphumi_timer, collect_temphumi_timer_callback);
osTimerId collect_temphumi_timer_id;

void LEDShow_Thread(void const *arg) {
    osEvent wairtresult;
    htsensorvalue_t *temphumi;
    while (1) {
#if 0
        wairtresult = osMessageGet(msgq_LEDShow_Thread_id, osWaitForever);
        if (wairtresult.status == osEventMessage) {
            if (wairtresult.value.v == 0x01) {
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                osDelay(1000);
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            } else if (wairtresult.value.v == 0x02) {
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                osDelay(200);
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            }
        }
#endif
        wairtresult = osMailGet(htsensorvalue_led_q_id, osWaitForever);
        if (wairtresult.status == osEventMail) {
            temphumi = (htsensorvalue_t *) wairtresult.value.p;
            if (temphumi->temp <= 29) {
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                osDelay(600);
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                osDelay(150);
                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            }
            osMailFree(htsensorvalue_led_q_id, temphumi);
        }
    }
}

void UART1_Send_Thread(void const *arg) {
    osEvent wairtresult;
    htsensorvalue_t *temphumi;
    uint8_t i;
    while (1) {
//        osDelay(100);
//        log_i("UART1_Send_Thread");
        wairtresult = osMailGet(htsensorvalue_uart1_q_id, osWaitForever);
        if (wairtresult.status == osEventMail) {
//            osMutexWait(uart_mutex_id,osWaitForever);
            log_i("UART1_Send_Thread");
//            osMutexRelease(uart_mutex_id);
            temphumi = (htsensorvalue_t *) wairtresult.value.p;
//            printf("Temp:%d;Humi:%d\r\n",temphumi->temp,temphumi->humi);
            char buf[32];
            snprintf(buf, sizeof(buf),
                     "Temp:%d;Humi:%d\r\n",
                     temphumi->temp,
                     temphumi->humi);
            HAL_UART_Transmit(&huart1,
                              (uint8_t *) buf,
                              strlen(buf),
                              HAL_MAX_DELAY);
            osMailFree(htsensorvalue_uart1_q_id, temphumi);
        }
    }
}

void TempHumi_Collect_Thread(void const *arg) {
    uint32_t begintime;
    uint8_t temp, humi;
    htsensorvalue_t *temphumi;
    //Clion必须在main函数外，osKernelStart()后osTimerStart，因为无keil5自动优化
    if (collect_temphumi_timer_id != NULL) {
        osTimerStart(collect_temphumi_timer_id, 1000);
    }
    while (1) {
        osDelay(100);
//        log_i("TempHumi_Collect_Thread");
//        begintime = os_time;
//        if (DHT11_Read_Data(&temp, &humi) == 0) {
//            temphumi = (htsensorvalue_t *) osMailAlloc(htsensorvalue_uart1_q_id, osWaitForever);
//            temphumi->temp = temp;
//            temphumi->humi = humi;
//            osMailPut(htsensorvalue_uart1_q_id, temphumi);
//
//            temphumi = (htsensorvalue_t *) osMailAlloc(htsensorvalue_led_q_id, osWaitForever);
//            temphumi->temp = temp;
//            temphumi->humi = humi;
//            osMailPut(htsensorvalue_led_q_id, temphumi);
//        } else {
//            HAL_UART_Transmit(&huart1, (uint8_t *) "Error", 6, 100);
//        }
//        uint32_t cost = os_time - begintime;
////        printf("cost=%lu\r\n", cost);
//        if (cost < 1000) {
//            osDelay(1000 - cost);
//        } else {
//            printf("OVERTIME\r\n");
//            osDelay(1);
//        }
    }
}

volatile uint8_t key1_is_pressed = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == KEY1_Pin) {
//        key1_is_pressed = 1;
//        HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
//        osSignalSet(tid_led_show_thread, 0x00000001);
//        osSignalSet(tid_uart_send_thread, 0x00000001);
        osMessagePut(msgq_LEDShow_Thread_id, 0x01, 0);
        osMessagePut(msgq_UART1_Send_Thread_id, 0x01, 0);
    } else if (GPIO_Pin == KEY2_Pin) {
        osMessagePut(msgq_LEDShow_Thread_id, 0x02, 0);
        osMessagePut(msgq_UART1_Send_Thread_id, 0x02, 0);
    }
}

void collect_temphumi_timer_callback(void const *arg) {
    uint8_t temp, humi;
    htsensorvalue_t *temphumi;
    if (DHT11_Read_Data(&temp, &humi) == 0) {
        temphumi = (htsensorvalue_t *) osMailAlloc(htsensorvalue_uart1_q_id, osWaitForever);
        temphumi->temp = temp;
        temphumi->humi = humi;
        osMailPut(htsensorvalue_uart1_q_id, temphumi);

        temphumi = (htsensorvalue_t *) osMailAlloc(htsensorvalue_led_q_id, osWaitForever);
        temphumi->temp = temp;
        temphumi->humi = humi;
        osMailPut(htsensorvalue_led_q_id, temphumi);
    } else {
        log_e("DHT11出错了!");
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */
    osKernelInitialize();
    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_TIM4_Init();
    /* USER CODE BEGIN 2 */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;//Clion编译防止内核启动前切SysTick死机
//    setbuf(stdout, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
    elog_start();
    log_i("Wireless test start");

    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

    uart_mutex_id = osMutexCreate(osMutex(uart_mutex));

    msgq_LEDShow_Thread_id = osMessageCreate(osMessageQ(msgq_LEDShow_Thread), NULL);
    msgq_UART1_Send_Thread_id = osMessageCreate(osMessageQ(msgq_UART1_Send_Thread), NULL);

    htsensorvalue_uart1_q_id = osMailCreate(osMailQ(htsensorvalue_uart1_q), NULL);
    htsensorvalue_led_q_id = osMailCreate(osMailQ(htsensorvalue_led_q), NULL);

    tid_led_show_thread = osThreadCreate(osThread (LEDShow_Thread), NULL);
    tid_uart_send_thread = osThreadCreate(osThread (UART1_Send_Thread), NULL);
    tid_temphumi_collect_thread = osThreadCreate(osThread(TempHumi_Collect_Thread), NULL);

    collect_temphumi_timer_id = osTimerCreate(osTimer(collect_temphumi_timer), osTimerPeriodic, NULL);

    if (tid_led_show_thread == NULL) {
        log_e("LED_ERROR");
    } else {
        log_i("LED_SUCCESS");
    }
    if (tid_uart_send_thread == NULL) {
        log_e("UART_ERROR");
    } else {
        log_i("UART_SUCCESS");
    }
    if (tid_temphumi_collect_thread == NULL) {
        log_e("TH_ERROR");
    } else {
        log_i("TH_SUCCESS");
    }
//    osSignalSet(tid_led_showthread, 0x00000001);
//    __set_PSP(__get_MSP());
    osKernelStart();

//    HAL_StatusTypeDef result;
//    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_data, sizeof(rx_data));
//    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
//    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
//    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
//    osDelay(0xFFFFFFFF);
    while (1) {
        osDelay(100);
//        if (key1_is_pressed) {
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
//        } else {
//            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);
//        }
//        if (DHT11_Read_Data(&g_htsensor.temp, &g_htsensor.hu) == 0) {
//            log_i("温度:%d,湿度:%d", g_htsensor.temp, g_htsensor.hu);
//        } else {
//            log_e("DHT11读取失败");
//        }

//      result = HAL_UART_Receive_DMA(&huart1, rx_data, 10);
//        uart1_rev_finish++;
//      while(uart1_rev_finish==0)
//      {
//          HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
//          printf("hello world\r\n");
//          HAL_Delay(300);
//      }
//      HAL_UART_Transmit_DMA(&huart1, (uint8_t *)"Lin Jie\r\n", sizeof("Lin Jie"));
//      HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
//      printf("hello world %d",uart1_rev_finish);
//      log_d("调试信息.......");
//      log_i("提示信息.......");
//      log_e("警告信息.......");
//        HAL_Delay(1000);
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void) {

    /* USER CODE BEGIN TIM4_Init 0 */

    /* USER CODE END TIM4_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM4_Init 1 */

    /* USER CODE END TIM4_Init 1 */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 71;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 65535;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM4_Init 2 */

    /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void) {

    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) {

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    /* DMA1_Channel5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : LED_Pin */
    GPIO_InitStruct.Pin = LED_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : KEY1_Pin */
    GPIO_InitStruct.Pin = KEY1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : DHT11_IO_Pin */
    GPIO_InitStruct.Pin = DHT11_IO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_IO_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : KEY2_Pin */
    GPIO_InitStruct.Pin = KEY2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
