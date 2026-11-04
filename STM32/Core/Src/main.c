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
#include "cyber_gear.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MOTOR_ID 34
#define MOTOR_ID2 50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

HAL_StatusTypeDef state;

float target_pos1 = 0.0f;
float target_vel1 = 0.0f;
float target_tor1 = 0.0f;

float target_pos2 = 0.0f;
float target_vel2 = 0.0f;
float target_tor2 = 0.0f;

float kp = 1.0f;
float kd = 0.0f;

_Bool enableFlag = 0;
_Bool prevEnableFlag = 0;
_Bool isEnableFlagUpdated = 0;

CG_Motor motorData1;
CG_Motor motorData2;

volatile uint8_t zeroFlag = 0;
HAL_StatusTypeDef zero_state;
float pos_before_zero;

volatile uint8_t clearFaultFlag1 = 0;
volatile uint8_t clearFaultFlag2 = 0;

/* VARIABEL POIN 1 & 2 */
uint8_t rxByte;
char rxBuffer[128];
int rxIndex = 0;
volatile uint8_t dataReceived = 0;

/* LIVE EXPRESSIONS (Poin 2) */
int digital_angle_1 = 0;
int digital_angle_2 = 0;
int digital_angle_3 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	if (hcan->Instance == CAN1) {
		CAN_RxHeaderTypeDef rx_header;
		uint8_t rx_data[8];

		if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data)
				== HAL_OK) {
			cybergear_rx_handler(&rx_header, rx_data);
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rxByte == '\\n') {
            rxBuffer[rxIndex] = '\\0';
            rxIndex = 0;
            dataReceived = 1;
        } else {
            if (rxIndex < 127) rxBuffer[rxIndex++] = rxByte;
        }
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
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

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CAN_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */

	CAN_FilterTypeDef canFilter;
	canFilter.FilterBank = 0;
	canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
	canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
	canFilter.FilterIdHigh = 0x0000;
	canFilter.FilterIdLow = 0x0000;
	canFilter.FilterMaskIdHigh = 0x0000;
	canFilter.FilterMaskIdLow = 0x0000;
	canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
	canFilter.FilterActivation = CAN_FILTER_ENABLE;
	canFilter.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &canFilter) != HAL_OK) {
		Error_Handler();
	}

	if (HAL_CAN_Start(&hcan) != HAL_OK) {
		Error_Handler();
	}

	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
		Error_Handler();
	}

	cybergear_init(&hcan, 253);
	cybergear_add_motor(MOTOR_ID);
	cybergear_add_motor(MOTOR_ID2);

	HAL_UART_Receive_IT(&huart1, &rxByte, 1);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		/* KOMUNIKASI 2 ARAH & PARSING LIVE EXPRESSION (Poin 1 & 2) */
		if (dataReceived) {
			dataReceived = 0;
            
			sscanf(rxBuffer, "%d %d %d", &digital_angle_1, &digital_angle_2, &digital_angle_3);
            
			char txBuf[128];
			int len = snprintf(txBuf, sizeof(txBuf), "ACK Diterima: %d %d %d\\n", digital_angle_1, digital_angle_2, digital_angle_3);
			HAL_UART_Transmit(&huart1, (uint8_t*)txBuf, len, HAL_MAX_DELAY);
		}

		if (enableFlag != prevEnableFlag) {
			if (enableFlag) {
				cybergear_enable(MOTOR_ID);
				cybergear_enable(MOTOR_ID2);
			} else {
				cybergear_disable(MOTOR_ID);
				cybergear_disable(MOTOR_ID2);
			}
			prevEnableFlag = enableFlag;
		}

		if (zeroFlag) {
		    zeroFlag = 0;
		    CG_Motor *m = cybergear_get_motor(MOTOR_ID);
		    if (m != NULL) pos_before_zero = m->position;
		    zero_state = cybergear_set_zero(MOTOR_ID);
		}

		if (clearFaultFlag1) {
			clearFaultFlag1 = 0;
			cybergear_clear_fault(MOTOR_ID);
		}

		if (clearFaultFlag2) {
			clearFaultFlag2 = 0;
			cybergear_clear_fault(MOTOR_ID2);
		}

		if (enableFlag) {
			state = cybergear_send_motion(MOTOR_ID, target_pos1, target_vel1, target_tor1, kp, kd);
			state = cybergear_send_motion(MOTOR_ID2, target_pos2, target_vel2, target_tor2, kp, kd);
		}

		CG_Motor *motor1 = cybergear_get_motor(MOTOR_ID);
		if (motor1 != NULL && motor1->is_updated) {
			motorData1.id = motor1->id;
			motorData1.torque = motor1->torque;
			motorData1.position = motor1->position;
			motorData1.velocity = motor1->velocity;
			motorData1.temperature = motor1->temperature;
			motorData1.is_error = motor1->is_error;
			motorData1.fault_bits = motor1->fault_bits;
			motor1->is_updated = false;
		}

		CG_Motor *motor2 = cybergear_get_motor(MOTOR_ID2);
		if (motor2 != NULL && motor2->is_updated) {
			motorData2.id = motor2->id;
			motorData2.torque = motor2->torque;
			motorData2.position = motor2->position;
			motorData2.velocity = motor2->velocity;
			motorData2.temperature = motor2->temperature;
			motorData2.is_error = motor2->is_error;
			motorData2.fault_bits = motor2->fault_bits;
			motor2->is_updated = false;
		}

		HAL_Delay(10);
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
 * @brief CAN Initialization Function
 * @param None
 * @retval None
 */
static void MX_CAN_Init(void) {

	/* USER CODE BEGIN CAN_Init 0 */

	/* USER CODE END CAN_Init 0 */

	/* USER CODE BEGIN CAN_Init 1 */

	/* USER CODE END CAN_Init 1 */
	hcan.Instance = CAN1;
	hcan.Init.Prescaler = 4;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_6TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = DISABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = ENABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = DISABLE;
	if (HAL_CAN_Init(&hcan) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN CAN_Init 2 */

	/* USER CODE END CAN_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

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
     ex: printf("Wrong parameters value: file %s on line %d\\r\\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
""")
