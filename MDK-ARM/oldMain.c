/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "fonts.h"
#include "ssd1306.h"
#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "keypad.h"
#include "stdbool.h"
#include "mfrc522.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_ATTEMPTS 3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
		
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
char str[16];

typedef enum {
	STATE_INITIAL,
	STATE_CARD,
	STATE_PASSCODE,
	STATE_FINGERPRINT,
	STATE_OPEN,
	STATE_CLOSE,
	STATE_ALARM
} security_state_t;

unsigned int input;
char input_password[7];
uint8_t CardID[5];
KEYPAD_Name KeyPad;
bool passcode_check, rfid_check;
int attempt;
char flashed_password[7] = "050202";
uint8_t MyID[5] = {0x13, 0x0c, 0x0f, 0xec, 0xfc};
security_state_t currentState;
char KEYMAP[NUMROWS][NUMCOLS] = {
{'1','2','3'},
{'4','5','6'}, 
{'7','8','9'},
{'*','0','#'}
};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

bool check_password(char a[], char b[]) {
	if( strcmp(a, b) == 0){
    return true;
	}
	return false;
}

void rfid(){
	char szBuff[17];
	while(true) {
		if (TM_MFRC522_Check(CardID) == MI_OK){
			SSD1306_Clear();
			sprintf(szBuff, "ID: 0x%02X%02X%02X%02X%02X", CardID[0], CardID[1], CardID[2], CardID[3], CardID[4]);
			SSD1306_GotoXY(10,0);
			SSD1306_Puts(szBuff, &Font_7x10, 1);
			SSD1306_UpdateScreen();
			if (TM_MFRC522_Compare(CardID, MyID) == MI_OK) {
				SSD1306_GotoXY(15,25);
				SSD1306_Puts("ID PASSED", &Font_11x18, 1);
				SSD1306_UpdateScreen();
				HAL_Delay(500);
				SSD1306_Clear();
				rfid_check = true;
				break;
			}else {
				SSD1306_GotoXY(15,25);
				SSD1306_Puts("NOT PASSED", &Font_11x18, 1);
				SSD1306_UpdateScreen();
				HAL_Delay(500);
				rfid_check = false;
				SSD1306_Clear();
			}
		} else {
			SSD1306_GotoXY(10,50);
			SSD1306_Puts("Tap your ID card!", &Font_7x10, 1);
			SSD1306_UpdateScreen();
	 }
	}
}

void inputPasscode(char a[]) {
	unsigned int c;
	int x = 65;
	for (int i = 0; i < 6;) {
		c = KEYPAD3X4_Readkey(&KeyPad);
		if (c) {
			a[i] = (char)c;
			SSD1306_GotoXY(x, 30);
			SSD1306_Puts((char*)&c, &Font_7x10, 1);
			SSD1306_UpdateScreen();
			x = x + 7;
			i++;
		}
	}
}


void passcode(){
	char str[100];
	SSD1306_GotoXY(10,0);
	SSD1306_Puts("6-digit passcode", &Font_7x10, 1);
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("Input:", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	while(true) {
		inputPasscode(input_password);
		if(check_password(input_password, flashed_password) == true) {
			SSD1306_GotoXY(40,50);
			SSD1306_Puts("PASSED", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			passcode_check = true;
			HAL_Delay(500);
			SSD1306_Clear();
			attempt = 0;
			break;
		} else {
			attempt++;
			if(attempt > 3) {
				SSD1306_Clear();
				break;
			}
			SSD1306_GotoXY(20,50);
			SSD1306_Puts("NOT CORRECT", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			HAL_Delay(500);
			SSD1306_Clear();
			SSD1306_GotoXY(0,30);
			SSD1306_Puts("Re-input:", &Font_7x10, 1);
			SSD1306_GotoXY(0,40);
			sprintf(str,"Attempt: %d", attempt);
			SSD1306_Puts(str, &Font_7x10, 1);
			SSD1306_UpdateScreen();
			passcode_check = false;
			SSD1306_GotoXY(0, 30);
		SSD1306_Puts(input_password, &Font_7x10, 1);
		SSD1306_UpdateScreen();
		}
	}
}

void transitionToState(security_state_t nextState) {
    currentState = nextState;
}

void handleInitialState() {
	
    if (TM_MFRC522_Check(CardID) == MI_OK){
			transitionToState(STATE_CARD);
		}
}

void handleCardState() {
	rfid();
	if( rfid_check == true) {
		transitionToState(STATE_PASSCODE);
	}
}

void handlePasscodeState() {
	
}

void handleOpenState() {

}

void handleCloseState() {

}

void handleAlarmState() {

}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	passcode_check = false, rfid_check = false;
	attempt = 0;
	currentState = STATE_INITIAL;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	KEYPAD3X4_Init(&KeyPad, KEYMAP, GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_1, GPIOA, GPIO_PIN_0, 
																	GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_5,
																	GPIOA, GPIO_PIN_4, GPIOA, GPIO_PIN_3);
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
	TM_MFRC522_Init();
	SSD1306_Init();
	
	SSD1306_GotoXY(30,0);
  SSD1306_Puts("SMART LOCK", &Font_7x10, 1);
  SSD1306_GotoXY(30, 20);
  SSD1306_Puts("LOCKING", &Font_11x18, 1);
  SSD1306_UpdateScreen();
  HAL_Delay(1000);
  SSD1306_Clear();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		SSD1306_GotoXY(30, 20);
		SSD1306_Puts("LOCKING", &Font_11x18, 1);
		SSD1306_UpdateScreen();
		while(rfid_check == false) {
			rfid();
			if (rfid_check == true) {
				while(passcode_check == false) {
					passcode();
					break;
				}
			}
			if(attempt > 3) {
				rfid_check = false;
				attempt = 0;
				continue;
			}
		}
		HAL_Delay(300);
		SSD1306_GotoXY(15, 30);
		SSD1306_Puts(" OPENING", &Font_11x18, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(2000);
		SSD1306_Clear();
		rfid_check = false;
		passcode_check = false;
			
		HAL_Delay(2000);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA4 PA5 PA6
                           PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
