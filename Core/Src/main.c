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
#include "flash.h"
#include "as608.h"
#include "CommunicationESP.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_ATTEMPTS 3
#define FLASH_ADDR_PAGE_62			0x0800F800	
#define PASSCODE_ADDR_PAGE			0x0800F81A
#define FLASH_ADDR_PAGE_63			0x0800FC00

#define FLASH_USER_START_ADDR			FLASH_ADDR_PAGE_62
#define FLASH_USER_END_ADDR 			FLASH_ADDR_PAGE_63 + FLASH_PAGE_SIZE
extern uint8_t CheckedCode;
uint32_t startpage = FLASH_USER_START_ADDR;
uint32_t dataread, fingerValue;
uint8_t UpdateCode;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
		
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
typedef struct {
	char name[29];
	uint8_t ID;
	char mnv[8];
} user_t;

int fingerprintValue, k, attempt;
unsigned int input;
char input_password[7], newPasscode[7];
char original_passcode[] = "111111";
uint8_t fingerID;
uint8_t CardID[5];
uint8_t MyID[5] = {0x13, 0x0c, 0x0f, 0xec, 0xfc};
KEYPAD_Name KeyPad;
bool passcode_check, rfid_check, menu_check;
uint16_t *tempF;
char str[16];
char inputText[29];
char data[50] = {0};
int test_wo = 0;
int test_noupdate = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	  static uint8_t check = 0;
	  static uint8_t state = 0;
		if(huart->Instance == huart2.Instance){
			  CheckedCode = 1;
			  if(state == 1){
				if(check == 1){
				 if(UpdateCode == 'P'){
	         NVIC_SystemReset();
				 }
				 else {
					 check = 0;
					 HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
				 }
				}
				else if(check == 0){
					if(UpdateCode == 'U'){
					check = 1;
					}
					if(UpdateCode == 'N'){
					check = 2;
					}
					HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
				}
				else if(check == 2){
				  if(UpdateCode == 'O'){
					test_noupdate ++;
					check =0; state = 0; 
					HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
					}
					else {
					check = 0;
					HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
					}
				}
			}
			else if(state == 0){
			  if(check == 0){
				if(UpdateCode == 'W')check = 1;
					HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
				}
				else {
				if(UpdateCode == 'O'){
					check = 0;
					state = 1;
					test_wo ++;
					uint8_t IdentifyCode = 'A';
					HAL_UART_Transmit(&huart2,&IdentifyCode,1,100);
				}
				else check =0;
				HAL_UART_Receive_IT(&huart2,&UpdateCode,1);
				}
			}
		}
} 
void  FLASH_PageErase(uint32_t PageAddress);
typedef enum {
	STATE_INITIAL,
	STATE_FINGERPRINT,
	STATE_OPEN,
	MENU_PASSCODE,
	MENU_CARD,
	MENU_FINGERPRINT,
	MENU
} security_state_t;
security_state_t currentState;
const char KEYMAP[NUMROWS][NUMCOLS] = {
		{'1','2','3'},
		{'4','5','6'}, 
		{'7','8','9'},
		{'*','0','#'}
};

void transitionToState(security_state_t nextState) {
    currentState = nextState;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void FLASH_Write_Page(uint32_t passcodePage, uint32_t endPage, uint32_t data32, uint32_t fingerPage, uint32_t data) {
	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef EraseInit;
	EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInit.PageAddress = passcodePage;
	EraseInit.NbPages = (endPage - passcodePage)/FLASH_PAGE_SIZE;
	uint32_t PageError = 0;
	HAL_FLASHEx_Erase(&EraseInit, &PageError);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, fingerPage, data); //4 byte dau tien
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, passcodePage, data32); //4 byte dau tien
  HAL_FLASH_Lock();
}

uint32_t readPassword(uint32_t addr) {
    return *(uint32_t*) (addr);
}

uint32_t FLASH_ReadData32(uint32_t addr) {
	uint32_t data = *(__IO uint32_t *) (addr);
	return data;
}

void uint32ToCharArray(uint32_t value, char charArray[]) {
	sprintf(charArray, "%06X", value);
}

uint32_t charArrayToUint32(char charArray[]) {
	uint32_t result = 0;
	for (int i = 0; i < 8; i++) {
		if (charArray[i] >= '0' && charArray[i] <= '9'){
				result = (result << 4) + (charArray[i] - '0');
		}
	}
	return result;
}

unsigned int passcodeCase(unsigned input, char output1, char output2, char output3){
	uint32_t currentTime = HAL_GetTick();
	unsigned int c;
	int pressCount = 1;
	while(HAL_GetTick() - currentTime < 300) {
		c = KEYPAD3X4_Readkey(&KeyPad);
		if(c && c == input) {
			HAL_Delay(10);
			pressCount++;
			currentTime = HAL_GetTick();
		}
	}
	if (pressCount == 1) {
			return input;
	}else if (pressCount == 2) {
			return output1;
	} else if (pressCount == 3) {
			return output2;
	} else if (pressCount == 4){
			return output3;
	}
}

void inputPasscodeWithText(char s[]) {
	SSD1306_Clear();
	unsigned int c, k;
	int x = 40;
	char name[29] = {'\0'};
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("Name:", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	for (int i = 0; i < 29;) {
		c = KEYPAD3X4_Readkey(&KeyPad);
		if (c) {
			switch (c) {
				case '1':
					k = passcodeCase(c, 'A', 'B', 'C');
					break;
				case '2':
					k = passcodeCase(c, 'D', 'E', 'F');
					break;
				case '3':
					k = passcodeCase(c, 'G', 'H', 'I');
					break;
				case '4':
					k = passcodeCase(c, 'J', 'K', 'L');
					break;
				case '5':
					k = passcodeCase(c, 'M', 'N', 'O');
					break;
				case '6':
					k = passcodeCase(c, 'P', 'Q', 'R');
					break;
				case '7':
					k = passcodeCase(c, 'S', 'T', 'U');
					break;
				case '8':
					k = passcodeCase(c, 'V', 'W', 'X');
					break;
				case '9':
					k = passcodeCase(c, 'Y', 'Z', '\0');
					break;
				case '0':
					k = ' ';
					break;
				case '#':
					i = 29;
					break;
//				case '*':
//					i = 0;
//					name[0] = '\0';
//					x = 10;
//					k = '\0';
//					SSD1306_Clear();
//					break;
			}
			name[i] = (char)k;
			SSD1306_GotoXY(x, 30);
			SSD1306_Puts((char*)&k, &Font_7x10, 1);
			SSD1306_UpdateScreen();
			x = x + 7;
			i++;
		}
	}
	strcpy(s,name);
	SSD1306_Clear();
	SSD1306_GotoXY(10, 50);
	SSD1306_Puts(&name, &Font_7x10, 1);
	SSD1306_UpdateScreen();
}

void inputMNV(char a[]) {
	SSD1306_Clear();
	unsigned int c;
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("MNV: ", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	int x = 40;
	for (int i = 0; i < 8;) {
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


void setupDefaultPassword() {
	if (readPassword(PASSCODE_ADDR_PAGE) == 0xFFFFFFFF) {
		emptyfinger_database();
		WakeUpESP();
		EmptyDatabase();
		uint32_t value = charArrayToUint32(original_passcode);
		FLASH_Write_Page(PASSCODE_ADDR_PAGE, FLASH_USER_END_ADDR, value, startpage, 0x00);
	}
}

bool check_password(char a[], char b[]) {
	if( strcmp(a, b) == 0){
    return true;
	}
	return false;
}

void rfid(){
	char szBuff[17];
	SSD1306_Puts("ADMIN", &Font_7x10, 1);
			SSD1306_GotoXY(10,50);
			SSD1306_Puts("Tap your ID card!", &Font_7x10, 1);
	while(true) {
		if (TM_MFRC522_Check(CardID) == MI_OK){
			SSD1306_Clear();
			sprintf(szBuff, "ID: 0x%02X%02X%02X%02X%02X", CardID[0], CardID[1], CardID[2], CardID[3], CardID[4]);
			SSD1306_GotoXY(10,50);
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
			SSD1306_GotoXY(20, 0);
			//SSD1306_Puts("ADMIN", &Font_7x10, 1);
			SSD1306_GotoXY(10,50);
			//SSD1306_Puts("Tap your ID card!", &Font_7x10, 1);
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
	dataread = FLASH_ReadData32(PASSCODE_ADDR_PAGE);
	char value[7];
	uint32ToCharArray(dataread,&value);
	SSD1306_GotoXY(20, 0);
	SSD1306_Puts("ADMIN", &Font_7x10, 1);
	SSD1306_GotoXY(10,18);
	SSD1306_Puts("6-digit passcode", &Font_7x10, 1);
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("Input:", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	while(true) {
		inputPasscode(input_password);
		if(check_password(input_password, value) == true) {
			SSD1306_GotoXY(40,50);
			SSD1306_Puts("PASSED", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			passcode_check = true;
			menu_check = false;
			HAL_Delay(500);
			SSD1306_Clear();
			attempt = 0;
			break;
		} else {
			attempt++;
			if(attempt > 3) {
				passcode_check = false;
				rfid_check = false;
				attempt = 0;
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
		}
	}
}
void openLock(){
	int x;
	for(x =250;x<1250;x++) {
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, x);
	}
}

void closeLock(){
	int x;
	for(x =1250;x>250;x--) {
		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, x);
	}
}

void gotoSleep(){
	SSD1306_Clear();
	SSD1306_GotoXY(20, 30);
	SSD1306_Puts("Zzzz....", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	//ControlLed(0);
	//Ham vao che do ngu, dua code che do ngu cua module vao day
	if(SwitchSensorToSleepMode()!=0){
		write_command("Fingerprint not sleep",strlen("Fingerprint not sleep"));
		HAL_Delay(1000);
	}
	HAL_SuspendTick();
	//Enter Sleep Mode , wake up is done once User push-button is pressed
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
	
}
void wakeup() {
	SSD1306_Clear();
	SSD1306_GotoXY(30, 20);
	SSD1306_Puts("LOCKING", &Font_11x18, 1);
	//Dua lenh thoat che do ngu vao day
  //ControlLed(1);
	SSD1306_UpdateScreen();
}
void handleInitialState() {
	SSD1306_GotoXY(30, 20);
	SSD1306_Puts("LOCKING", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	uint8_t temp;
	uint32_t currentTime = HAL_GetTick();
	while (1) {
	    if(HAL_GetTick() - currentTime > 5000){
			SSD1306_Clear();
			SSD1306_GotoXY(10, 20);
			SSD1306_Puts("Going into SLEEP MODE", &Font_7x10, 1);
			SSD1306_GotoXY(10, 30);
			SSD1306_Puts("in 1 seconds", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			HAL_Delay(1000);
			gotoSleep();
			currentTime = HAL_GetTick();
		  wakeup();
			//TM_MFRC522_Init();
		} else if(fingerprintValue == 0) {
			SSD1306_Clear();
			SSD1306_GotoXY(35,0);
			SSD1306_Puts("No user!!", &Font_7x10, 1);
			SSD1306_GotoXY(15,20);
			SSD1306_Puts("Please add new", &Font_7x10, 1);
			SSD1306_GotoXY(20,30);
			SSD1306_Puts("fingerprint!!", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			HAL_Delay(1000);
			transitionToState(MENU_CARD);
			transitionToState(MENU_PASSCODE);
			break;
		} else if (checkFinger(temp)  == FINGERPRINT_OK) {
			transitionToState(STATE_FINGERPRINT);
			break;
		} else if(TM_MFRC522_Check(CardID) == MI_OK){
			//write_command("test",strlen("test"));
			transitionToState(MENU_CARD);
			break;
		}		
		HAL_Delay(50);
 	}
}
void handleFingerprintState(){
	SSD1306_GotoXY(0, 20);
	//SSD1306_Puts("Put Finger on sensor", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	//HAL_Delay(100);
	uint8_t temp;
	int f = 0;
	//while(checkFinger(f)!=FINGERPRINT_OK)HAL_Delay(100);
	temp = checkFingerPrintID(&tempF);
	if (temp == 0) {
		transitionToState(STATE_OPEN);
		WakeUpESP();
		AddTimeRecord(tempF);
	} else {
		transitionToState(STATE_INITIAL);
	}
}

void handleOpenState() {
	SSD1306_GotoXY(30, 20);
	SSD1306_Puts("OPENING", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	openLock();
	if (HAL_GPIO_ReadPin(SensorPin_GPIO_Port, SensorPin_Pin) == 0) {
		uint32_t tick = HAL_GetTick();
		while(HAL_GPIO_ReadPin(SensorPin_GPIO_Port, SensorPin_Pin) == 0){
			if(HAL_GetTick() - tick > 2000){
				SSD1306_Clear();
				closeLock();
				break;
			}
		}
		transitionToState(STATE_INITIAL);
	}
}

void handleChangePasscode() {
	SSD1306_Clear();
	char str[100];
	SSD1306_GotoXY(10,18);
	SSD1306_Puts("6-digit passcode", &Font_7x10, 1);
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("Input:", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	inputPasscode(newPasscode);
	uint32_t passcode = charArrayToUint32(newPasscode);
	fingerValue = FLASH_ReadData32(startpage);
	FLASH_Write_Page(PASSCODE_ADDR_PAGE, FLASH_USER_END_ADDR, passcode, startpage, (int)fingerValue);
	SSD1306_Clear();
}

void defineUser(user_t *a) {
	a->ID = (int)fingerValue;
	inputPasscodeWithText(&a->name);
	inputMNV(&a->mnv);
}

int getID(user_t *user){
	return user->ID;
}
char* getName(user_t *user){
	return user->name;
}
char* getMnv(user_t *user){
	return user->mnv;
}
void printUSER(user_t *a) {
	char str[20];
	SSD1306_Clear();
	SSD1306_GotoXY(10,0);
	sprintf(str,"ID: %d",getID(a));
	SSD1306_Puts(str, &Font_7x10, 1);
	
	SSD1306_GotoXY(10,30);
	sprintf(str,"Name: %s",getName(a));
	SSD1306_Puts(str, &Font_7x10, 1);
	
	SSD1306_GotoXY(10,50);
	sprintf(str,"MNV: %s",getMnv(a));
	SSD1306_Puts(str, &Font_7x10, 1);
	
	SSD1306_UpdateScreen();
	HAL_Delay(3000);
}

void handleAddUser(){
	fingerValue = FLASH_ReadData32(startpage);
	fingerprintValue = (int)fingerValue;
	dataread = FLASH_ReadData32(PASSCODE_ADDR_PAGE);
	while(1){
	if(getFingerprintEnroll(find_avaiable_idex_table()) == 0){
	fingerprintValue = (int)fingerValue;
	user_t user;
	defineUser(&user);
	WakeUpESP();
	if(AddRecord(user.name,user.mnv,user.ID)!=0){
		  write_command("Fail ADD USER",strlen("Fail ADD USER"));
	}
	else write_command("Success Update",strlen("Success Update"));
	HAL_Delay(2000);
	fingerprintValue++;
	FLASH_Write_Page(PASSCODE_ADDR_PAGE, FLASH_USER_END_ADDR, dataread, startpage, fingerprintValue);
	printUSER(&user);
	break;
	}
	else {
		if(fingerprintValue != 0){
		write_command("Failed Enroll Finger",strlen("Failed Enroll Finger"));
		HAL_Delay(1500);
		transitionToState(MENU);
		break;
		}			
	  else {
		write_command("Failed Enroll Finger",strlen("Failed Enroll Finger"));
		HAL_Delay(1000);
		write_command("Preparing enroll again",strlen("Preparing enroll again"));
		HAL_Delay(1000);
		}
	}
}
}
void handleRemoveUser(){
	SSD1306_Clear();
	char str[100];
	int a[2];
	SSD1306_GotoXY(0,30);
	SSD1306_Puts("Remove ID: ", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	unsigned int c;
	int x = 75;
	for (int i = 0; i < 2;) {
		c = KEYPAD3X4_Readkey(&KeyPad);
		if (c) {
			a[i] = (int)c;
			SSD1306_GotoXY(x, 30);
			SSD1306_Puts((char*)&c, &Font_7x10, 1);
			SSD1306_UpdateScreen();
			x = x + 7;
			i++;
		}
	}
	int k = (a[0]-48)*10 + (a[1]-48);
	fingerValue = FLASH_ReadData32(startpage);
	dataread = FLASH_ReadData32(PASSCODE_ADDR_PAGE);
	fingerprintValue = (int)fingerValue;
	HAL_Delay(1000);
	if ( k > fingerprintValue) {
		SSD1306_Clear();
		SSD1306_GotoXY(20,30);
		SSD1306_Puts("ID not found!", &Font_7x10, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(1000);
	} else {
			fingerID = (uint8_t)k;
			if (deleteFingerprint(fingerID) == FINGERPRINT_OK){
				fingerprintValue--;
				FLASH_Write_Page(PASSCODE_ADDR_PAGE, FLASH_USER_END_ADDR, dataread, startpage, fingerprintValue);
				WakeUpESP();
				if(RemoveRecord(fingerID)==0)write_command("Remove on database successfully",strlen("Remove on database successfully"));
			  else write_command("Fail update on database",strlen("Fail update on database"));
				HAL_Delay(2000);
		}
	}
}
void handleCard(){
	SSD1306_Clear();
	rfid();
	if (rfid_check == true) {
		transitionToState(MENU_PASSCODE);
	}
}

void handlePasscode() {
	SSD1306_Clear();
	passcode();
	if (passcode_check == true) {
		transitionToState(MENU_FINGERPRINT);
	}
}
int handleEmpty(){
	fingerprintValue = 0;
	if(EmptyFlash(FLASH_ADDR_PAGE_62,1)!=HAL_OK){
		 write_command("Empting flash failed",strlen("Empting flash failed"));
		 return -1;
	}
	HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,0x0800F800,fingerprintValue);
  HAL_FLASH_Lock();
	if(emptyfinger_database()!=0){
	   write_command("Empty sensor failed",strlen("Empty sensor failed"));
		 return -1;
	}
	write_command("Handling ...",strlen("Handling ..."));
  WakeUpESP();
	if(EmptyDatabase()==0){
		write_command("Empty database success",strlen("Empty database success"));
		HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,0x0800F818,0x1111FFFF);
	  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,0x0800F81C,0xFFFF0011);
    HAL_FLASH_Lock();
	  transitionToState(STATE_INITIAL);
		return 0;
	}
	else {
		write_command("Failed empty database",strlen("Failed empty database"));
		//transitionToState(MENU);
		return -1;
	}
	HAL_Delay(2000);
  
}
void handleMenu() {
	char c;
	SSD1306_Clear();
	SSD1306_GotoXY(30, 30);
	SSD1306_Puts("MENU", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(200);
	c = KEYPAD3X4_Readkey(&KeyPad);
	int HaveEmptied = 0;
	SSD1306_GotoXY(20, 0);
		SSD1306_Puts("ADMIN", &Font_7x10, 1);
		SSD1306_GotoXY(10, 10);
		SSD1306_Puts("0.Empty Database", &Font_7x10, 1);
		SSD1306_GotoXY(10, 20);
		SSD1306_Puts("1.Change passcode", &Font_7x10, 1);
		SSD1306_GotoXY(10, 30);
		SSD1306_Puts("2.Add user", &Font_7x10, 1);
		SSD1306_GotoXY(10, 40);
		SSD1306_Puts("3.Remove user", &Font_7x10, 1);
		SSD1306_GotoXY(10, 50);
		SSD1306_Puts("#.Exit", &Font_7x10, 1);
	while(c != '#') {
		if(HaveEmptied == 1)break;
		SSD1306_UpdateScreen();
		c = KEYPAD3X4_Readkey(&KeyPad);
		switch (c) {
			case '0':
				if(handleEmpty()==0)HaveEmptied = 1;
			  else write_command("Failed empty",strlen("Failed empty"));
			  SSD1306_Clear();
				break;
			case '1':
				handleChangePasscode();
				SSD1306_Clear();
				break;
			case '2':
				handleAddUser();
				SSD1306_Clear();
				break;
			case '3':
				handleRemoveUser();
				SSD1306_Clear();
				break;
		}
	}
	SSD1306_Clear();
	transitionToState(STATE_INITIAL);
}
void handleFingerprint(){
	SSD1306_GotoXY(20, 0);
	SSD1306_Puts("ADMIN", &Font_7x10, 1);
	SSD1306_GotoXY(0, 20);
	SSD1306_Puts("FINGERPRINT", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(500);
	uint8_t temp;
	int count  = 0;
	fingerValue = FLASH_ReadData32(startpage);
	dataread = FLASH_ReadData32(PASSCODE_ADDR_PAGE);
	fingerprintValue = (int)fingerValue;
	if(fingerprintValue == 0) {
		handleAddUser();
		transitionToState(MENU);
	} 
	else {
		int f =0;
	  while(checkFinger(f)!=FINGERPRINT_OK)HAL_Delay(100);
		temp = checkFingerPrintID(&tempF);
		if (temp == 0) {
			//WakeUpESP();
			//HAL_UART_Transmit(&huart2,"This is a test\n",strlen("This is a test\n"),5000);
			transitionToState(MENU);
		} else if (temp == FINGERPRINT_NOTFOUND){
			while (1) {
				while(checkFinger(f)!=FINGERPRINT_OK)HAL_Delay(100);
				temp = checkFingerPrintID(&tempF);
				if (temp == 0) {
					transitionToState(MENU);
					break;
				} else if (count == 2) {
					rfid_check = false;
					passcode_check = false;
					transitionToState(STATE_INITIAL);
					break;
				}
				else if(temp == FINGERPRINT_NOTFOUND)count++;
			}
		}
		else {
		while(checkFinger(f)!=FINGERPRINT_OK)HAL_Delay(100);
		temp = checkFingerPrintID(&tempF);
		}
	}
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
	KEYPAD3X4_Init(&KeyPad, KEYMAP, GPIOA, GPIO_PIN_4, GPIOA, GPIO_PIN_1, GPIOC, GPIO_PIN_15, 
																	GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_0,
																	GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_5);
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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	//SSD1306_Puts("Put your finger", &Font_7x10, 1);
	//WakeUpESP();
  /* USER CODE BEGIN 2 */
	TM_MFRC522_Init();
	SSD1306_Init();
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
	checkPassword();
	setupDefaultPassword();
	fingerValue = FLASH_ReadData32(startpage);
	dataread = FLASH_ReadData32(PASSCODE_ADDR_PAGE);
	fingerprintValue = (int)fingerValue;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint16_t testt;
	HAL_UART_Receive_IT(&huart2,&UpdateCode,1); // UART2 interrupt receive for FOTA 
  while (1)
  {
		 switch (currentState) {
			case STATE_INITIAL:
					handleInitialState();
					break;
			case STATE_FINGERPRINT:
					handleFingerprintState();
					break;
			case STATE_OPEN:
					handleOpenState();
					break;
			case MENU_CARD:
					handleCard();
					break;
			case MENU_PASSCODE:
					handlePasscode();
					break;
			case MENU_FINGERPRINT:
					handleFingerprint();
					break;
			case MENU:
					handleMenu();
					break;    
		}  
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
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{
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
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK){
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK){
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK){
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK){
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK){
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 57600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_2;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 SensorPin_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|SensorPin_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET); // GPIO Pin setting for waking up ESP8266  
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

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
