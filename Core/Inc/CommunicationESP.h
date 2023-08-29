#include "main.h"
#include <string.h>
#include <stdlib.h>
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
extern UART_HandleTypeDef huart2;
#define ADDRESS_DATABASE 0x08007000
#define ADDRESS_START_BLINK_LED_APPLICATION 0x08004000
#define THREASHOLD_ERROR 20
#define FLASH_START_ADDRESS ((uint32_t)0x08004000)
#define ADDRESS_START_BLINK_LED_APPLICATION 0x08004000
#define START_CODE "ST"
#define ADD_CODE "AD"
#define REMOVE_CODE "RE"
#define EMPTY_CODE "EM"
#define END_CODE "\n"
uint8_t UpdateCode;
uint64_t ConvertToDecimal(char*array,int idex1,int idex2);
int EmptyFlash(uint32_t flash_start_address,uint32_t NumberOfPage);
int write_to_flash(char*array,int size,uint32_t flash_start_address);
int CheckError(char* CheckedArray,int size);
void JumpToUpdatedFirmware(void);
void ConvertDecimalToHex(uint32_t data,char*result);
int ReadFirmware(void);
void WakeUpESP(void);
int AddRecord(char* name,char* tempt_mnv, int ID);
int RemoveRecord(int ID);