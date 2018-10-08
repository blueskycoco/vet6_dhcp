#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stm32f10x.h>
#include "mymisc.h"
static unsigned char  fac_us=0;
static unsigned short fac_ms=0;
void delay_init(unsigned char SYSCLK)
{
	SysTick->VAL=0X00000000;
	SysTick->CTRL&=0xfffffffb;
	fac_us=SYSCLK/8;      
	fac_ms=(unsigned short)fac_us*1000;
	SysTick->CTRL&=0XFFFFFFFE;
	SysTick->VAL=0X00000000;

}            
void delay_ms(unsigned short nms)
{    
	SysTick->LOAD=(unsigned long)nms*fac_ms;
	SysTick->CTRL|=0x01;
	while(!(SysTick->CTRL&(1<<16)));
	SysTick->CTRL&=0XFFFFFFFE;
	SysTick->VAL=0X00000000; 
}   
void delay_us(unsigned long Nus)
{ 
	SysTick->LOAD=Nus*fac_us;
	SysTick->CTRL|=0x01;
	while(!(SysTick->CTRL&(1<<16)));
	SysTick->CTRL=0X00000000;
	SysTick->VAL=0X00000000;
} 
void DBG_PutChar(char ptr)
{   
	USART_SendData(UART4, ptr);
	while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET); 
}

void Debug_uart_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(UART4, &USART_InitStructure);

	USART_Cmd(UART4, ENABLE);
}
