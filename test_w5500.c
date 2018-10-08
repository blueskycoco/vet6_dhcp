#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stm32f10x.h>
#include <string.h>
#include "mymisc.h"
#include "wizchip_conf.h"
#include "dhcp.h"
extern void SWO_Enable();
wiz_NetInfo gWIZNETINFO = {
	.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
	.ip = {192, 168, 1, 2},
	.sn = {255, 255, 255, 0},
	.gw = {192, 168, 1, 1},
	.dns = {0, 0, 0, 0},
	.dhcp = NETINFO_DHCP 
};
#define SOCK_DHCP       	6
#define MY_MAX_DHCP_RETRY	2
#define DATA_BUF_SIZE   2048
uint8_t gDATABUF[DATA_BUF_SIZE];

void EXTI2_IRQHandler(void)
{
}

void spi_init(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOA
			|RCC_APB2Periph_AFIO	, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 ; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_4);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI3, &SPI_InitStructure);
	SPI_Cmd(SPI3, ENABLE);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

void  wizchip_select(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_3); 
}

void  wizchip_deselect(void)
{
	GPIO_SetBits(GPIOD, GPIO_Pin_3); 
}

uint8_t wizchip_read()
{
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI3, 0);
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(SPI3);
}

void  wizchip_write(uint8_t wb)
{
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI3, wb);
}
void wizchip_reset(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_4);
	delay_ms(5);  
	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	delay_ms(160);
}

static uint8_t PHYStatus_Check(void)
{
	uint8_t tmp;

	ctlwizchip(CW_GET_PHYLINK, (void*) &tmp);
	return (tmp == PHY_LINK_OFF) ? 0 : 1;
}
void my_ip_assign(void)
{
	getIPfromDHCP(gWIZNETINFO.ip);
	getGWfromDHCP(gWIZNETINFO.gw);
	getSNfromDHCP(gWIZNETINFO.sn);
	getDNSfromDHCP(gWIZNETINFO.dns);
	gWIZNETINFO.dhcp = NETINFO_DHCP;
	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
	printf("ip assigned\r\n");
}
void my_ip_conflict(void)
{
	printf("CONFLICT IP from DHCP\r\n");
}
void w5500_init()
{
	uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } };
	uint8_t tmpstr[6] = {0};
	wizchip_reset();
	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1) {
		printf("WIZCHIP Initialized fail.\r\n");
		while (1);
	}

	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	printf("WIZnet %s EVB - DHCP client \r\n", tmpstr);

	DHCP_init(SOCK_DHCP, gDATABUF);
	reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
}
void task()
{
	uint8_t my_dhcp_retry = 0;
	w5500_init();
	while (1) {
		if (!PHYStatus_Check()) {
			printf("Link is lost .. \r\n");
			delay_ms(1000);
			continue;
		}

		printf("hi\r\n");
		switch(DHCP_run())
		{
			case DHCP_IP_ASSIGN:
			case DHCP_IP_CHANGED:
				break;
			case DHCP_IP_LEASED:
				printf("IP got\r\n");
				break;
			case DHCP_FAILED:
				my_dhcp_retry++;
				if(my_dhcp_retry > MY_MAX_DHCP_RETRY)
				{
					gWIZNETINFO.dhcp = NETINFO_STATIC;
					DHCP_stop();
					printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
					ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
					my_dhcp_retry = 0;
				}
				break;
			default:
				break;
		}

		delay_ms(1000);
		DHCP_time_handler();
	}
}

int main(void)
{	
	delay_init(72);
	SWO_Enable();
	Debug_uart_Init();
	spi_init();
	task();
}

