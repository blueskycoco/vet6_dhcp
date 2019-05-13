#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stm32f10x.h>
#include <string.h>
#include "mymisc.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include "socket.h"
extern void SWO_Enable();
extern uint8_t ip_assigned;
wiz_NetInfo gWIZNETINFO = {
	.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
	.ip = {192, 168, 1, 2},
	.sn = {255, 255, 255, 0},
	.gw = {192, 168, 1, 1},
	.dns = {0, 0, 0, 0},
	.dhcp = NETINFO_DHCP 
};
#define SOCK_DHCP       	6
#define SOCK_TASK			1
#define DATA_BUF_SIZE   2048
uint8_t gDATABUF[DATA_BUF_SIZE];
uint8_t cmd[] = {0xad,0xac,0x00,0x26,0x01,0x00,0x01,0x00,0x00,0xa1,0x18,0x08,0x10,0x00,0x09,0x13,0x51,0x56,0x42,0x48,0x42,0x4c,0x08,0x66,0x85,0x60,0x31,0x97,0x56,0x81,0x28,0x37,0x41,0x33,0x19,0x01,0xed,0x60};

void EXTI2_IRQHandler(void)
{
	intr_kind source; 
	uint8_t sn_intr;
	if(EXTI_GetITStatus(EXTI_Line2))
	{	
		ctlwizchip(CW_GET_INTERRUPT,&source);
		ctlwizchip(CW_CLR_INTERRUPT,&source);
		printf("w5500 intr %x\r\n", source);
		ctlsocket(SOCK_DHCP, CS_GET_INTERRUPT, &sn_intr);
		//ctlsocket(SOCK_DHCP, CS_CLR_INTERRUPT, &sn_intr);
		printf("dhcp ir %x\r\n", sn_intr);
		EXTI_ClearITPendingBit(EXTI_Line2);
	}
}

void spi_init(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOD
			|RCC_APB2Periph_AFIO	, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable , ENABLE);

	/* w5500 reset, power */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	GPIO_ResetBits(GPIOD, GPIO_Pin_6);
	
	/* spi cs,mosi,miso,clk */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI3, &SPI_InitStructure);
	SPI_Cmd(SPI3, ENABLE);

	/* w5500 int */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
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
	SPI_I2S_SendData(SPI3, 0xff);
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(SPI3);
}

void  wizchip_write(uint8_t wb)
{
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI3, wb);
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData(SPI3);
}
void wizchip_reset(void)
{
	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	delay_ms(160);
	GPIO_ResetBits(GPIOD, GPIO_Pin_4);
	delay_ms(5);  
	GPIO_SetBits(GPIOD, GPIO_Pin_4);
	delay_ms(160);
}

static uint8_t PHYStatus_Check(void)
{
	uint8_t tmp;

	ctlwizchip(CW_GET_PHYLINK, (void*) &tmp);
	//printf("tmp %x\r\n", tmp);
	return (tmp == PHY_LINK_OFF) ? 0 : 1;
}

int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport)
{
   int32_t ret;
   uint16_t size = 0, sentsize=0;

   static uint16_t any_port = 	50000;

   switch(getSn_SR(sn))
   {
      case SOCK_ESTABLISHED :
         if(getSn_IR(sn) & Sn_IR_CON)
         {
#ifdef _LOOPBACK_DEBUG_
			printf("%d:Connected to - %d.%d.%d.%d : %d\r\n",sn, destip[0], destip[1], destip[2], destip[3], destport);
#endif
			setSn_IR(sn, Sn_IR_CON);  // this interrupt should be write the bit cleared to '1'
         }

		 if((size = getSn_RX_RSR(sn)) > 0) // Sn_RX_RSR: Socket n Received Size Register, Receiving data length
         {
			if(size > DATA_BUF_SIZE) size = DATA_BUF_SIZE; // DATA_BUF_SIZE means user defined buffer size (array)
			ret = recv(sn, buf, size); // Data Receive process (H/W Rx socket buffer -> User's buffer)
			if(ret <= 0) return ret; // If the received data length <= 0, receive failed and process end
			size = (uint16_t) ret;
			sentsize = 0;
			for (int i=0; i<ret; i++)
				printf("%02x ",buf[i]);
			printf("\r\n");
#if 0
			// Data sentsize control
			while(size != sentsize)
			{
				ret = send(sn, buf+sentsize, size-sentsize); // Data send process (User's buffer -> Destination through H/W Tx socket buffer)
				if(ret < 0) // Send Error occurred (sent data length < 0)
				{
					close(sn); // socket close
					return ret;
				}
				sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
			}
#endif
         } else {
         	 ret = send(sn, cmd, sizeof(cmd));
         	 if (ret < 0) {
         	 	 close(sn);
         	 	 return ret;
			 }

		 }
		 //////////////////////////////////////////////////////////////////////////////////////////////
         break;

      case SOCK_CLOSE_WAIT :
#ifdef _LOOPBACK_DEBUG_
         printf("%d:CloseWait\r\n",sn);
#endif
         if((ret=disconnect(sn)) != SOCK_OK) return ret;
#ifdef _LOOPBACK_DEBUG_
         printf("%d:Socket Closed\r\n", sn);
#endif
         break;

      case SOCK_INIT :
#ifdef _LOOPBACK_DEBUG_
    	 printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
#endif
    	 if( (ret = connect(sn, destip, destport)) != SOCK_OK) return ret;	//	Try to TCP connect to the TCP server (destination)
         break;

      case SOCK_CLOSED:
      	 printf("%d:Sock Closed\r\n", sn);
    	  close(sn);
    	  if((ret=socket(sn, Sn_MR_TCP, any_port++, 0x00)) != sn){
          	if(any_port == 0xffff) 
          		any_port = 50000;
          return ret; // TCP socket open with 'any_port' port number
        } 
#ifdef _LOOPBACK_DEBUG_
    	 //printf("%d:TCP client loopback start\r\n",sn);
         //printf("%d:Socket opened\r\n",sn);
#endif
         break;
      default:
      	 printf("default \r\n");
         break;
   }
   return 1;
}

void w5500_init()
{
	uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } };
	uint8_t tmpstr[6] = {0};
	intr_kind intr_source = 0;
	uint8_t sn_intr = 0;
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
#if 1
	ctlwizchip(CW_GET_INTRMASK,&intr_source);
	printf("intr %x\r\n", intr_source);
	intr_source |= IK_SOCK_ALL;
	ctlwizchip(CW_SET_INTRMASK,&intr_source);
	ctlwizchip(CW_GET_INTRMASK,&intr_source);
	printf("new intr %x\r\n", intr_source);
	ctlsocket(SOCK_DHCP, CS_SET_INTMASK, &sn_intr);
	ctlsocket(SOCK_DHCP, CS_GET_INTMASK, &sn_intr);
	printf("socket sn ir %x\r\n", sn_intr);
	sn_intr |= 0x1f;
	ctlsocket(SOCK_DHCP, CS_SET_INTMASK, &sn_intr);
	ctlsocket(SOCK_DHCP, CS_GET_INTMASK, &sn_intr);
	printf("new socket sn ir %x\r\n", sn_intr);
#endif
}
void task()
{
	uint8_t  task_ip[] = {106,14,116,201};
	uint16_t task_port = 1704;
	uint8_t dest_ip[] = {47,93,48,167};
	uint16_t dest_port = 1721;
	uint8_t *path = "/A1/21/stm32.bin";
	uint8_t ftpstart = 0;
	w5500_init();
	while (1) {
		if (!PHYStatus_Check()) {
			printf("Link is lost .. \r\n");
			delay_ms(1000);
			continue;
		}
		dhcp_handler();
		if (ip_assigned) {
			loopback_tcpc(SOCK_TASK, gDATABUF, task_ip, task_port);
			delay_ms(5000);
			if (ftpstart == 0) {
				printf("to start ftp\r\n");
				ftpstart = 1;
				printf("ftp result %d\r\n", ftpc_run(gDATABUF, dest_ip, dest_port, path));
			}
		}
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

