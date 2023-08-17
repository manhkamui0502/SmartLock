#include "spi.h"


void TM_MFRC522_InitPins(void)
{
	GPIO_InitTypeDef gpioInit;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	gpioInit.GPIO_Mode=GPIO_Mode_Out_PP;
	gpioInit.GPIO_Speed=GPIO_Speed_50MHz;
	gpioInit.GPIO_Pin=GPIO_Pin_12;
	GPIO_Init(GPIOB, &gpioInit);
	MFRC522_CS_HIGH;
}
