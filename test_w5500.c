#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stm32f10x.h>
#include <string.h>
#include "mymisc.h"
extern void SWO_Enable();
unsigned char cmd[] = {0x01, 0x0f, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00};
unsigned char cmd1[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00};
void task()
{
	uint16_t crc = crc16(cmd, sizeof(cmd) - 2);
	uint16_t len = sizeof(cmd);
	int i = 0;
	cmd[len-2] = crc & 0x00ff;
	cmd[len-1] = (crc >> 8) & 0x00ff;
	while (1) {
		led(1);
		for (i = 0; i < len; i++)
			printf("%02x ", cmd[i]);
		printf("\r\n");
		rs485_send(cmd, len);
		delay_ms(1000);
		led(0);
		delay_ms(1000);
	}
}

int main(void)
{	
	led_init();
	delay_init(72);
	SWO_Enable();
	Debug_uart_Init();
	rs485_init();
	task();
}

