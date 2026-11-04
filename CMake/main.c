/* USER CODE BEGIN Header */
#include "main.h"
#include "cyber_gear.h"
#include <stdio.h> 

#define MOTOR_ID 34
#define MOTOR_ID2 50

CAN_HandleTypeDef hcan;
UART_HandleTypeDef huart1;

HAL_StatusTypeDef state;
float target_pos1 = 0.0f, target_vel1 = 0.0f, target_tor1 = 0.0f;
float target_pos2 = 0.0f, target_vel2 = 0.0f, target_tor2 = 0.0f;
float kp = 1.0f, kd = 0.0f;
_Bool enableFlag = 0, prevEnableFlag = 0, isEnableFlagUpdated = 0;
CG_Motor motorData1;
CG_Motor motorData2;
volatile uint8_t zeroFlag = 0;
HAL_StatusTypeDef zero_state;
float pos_before_zero;
volatile uint8_t clearFaultFlag1 = 0, clearFaultFlag2 = 0;

/* VARIABEL POIN 1 & 2 */
uint8_t rxByte;
char rxBuffer[128];
int rxIndex = 0;
volatile uint8_t dataReceived = 0;

/* LIVE EXPRESSIONS (Poin 2) */
int digital_angle_1 = 0;
int digital_angle_2 = 0;
int digital_angle_3 = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_USART1_UART_Init(void);

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rx_header;
        uint8_t rx_data[8];
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
            cybergear_rx_handler(&rx_header, rx_data);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rxByte == '\n') {
            rxBuffer[rxIndex] = '\0';
            rxIndex = 0;
            dataReceived = 1;
        } else {
            if (rxIndex < 127) rxBuffer[rxIndex++] = rxByte;
        }
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN_Init();
    MX_USART1_UART_Init();

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

    if (HAL_CAN_ConfigFilter(&hcan, &canFilter) != HAL_OK) Error_Handler();
    if (HAL_CAN_Start(&hcan) != HAL_OK) Error_Handler();
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) Error_Handler();

    cybergear_init(&hcan, 253);
    cybergear_add_motor(MOTOR_ID);
    cybergear_add_motor(MOTOR_ID2);

    HAL_UART_Receive_IT(&huart1, &rxByte, 1);

    while (1) {
        /* KOMUNIKASI 2 ARAH & PARSING LIVE EXPRESSION (Poin 1 & 2) */
        if (dataReceived) {
            dataReceived = 0;
            
            sscanf(rxBuffer, "%d %d %d", &digital_angle_1, &digital_angle_2, &digital_angle_3);
            
            char txBuf[128];
            int len = snprintf(txBuf, sizeof(txBuf), "ACK Diterima: %d %d %d\n", digital_angle_1, digital_angle_2, digital_angle_3);
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
        if (clearFaultFlag1) { clearFaultFlag1 = 0; cybergear_clear_fault(MOTOR_ID); }
        if (clearFaultFlag2) { clearFaultFlag2 = 0; cybergear_clear_fault(MOTOR_ID2); }

        if (enableFlag) {
            state = cybergear_send_motion(MOTOR_ID, target_pos1, target_vel1, target_tor1, kp, kd);
            state = cybergear_send_motion(MOTOR_ID2, target_pos2, target_vel2, target_tor2, kp, kd);
        }

        CG_Motor *motor1 = cybergear_get_motor(MOTOR_ID);
        if (motor1 != NULL && motor1->is_updated) { motorData1 = *motor1; motor1->is_updated = 0; }
        CG_Motor *motor2 = cybergear_get_motor(MOTOR_ID2);
        if (motor2 != NULL && motor2->is_updated) { motorData2 = *motor2; motor2->is_updated = 0; }

        HAL_Delay(10);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

static void MX_CAN_Init(void) {
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
    if (HAL_CAN_Init(&hcan) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(huart->Instance==USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
