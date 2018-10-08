#ifndef _MY_MISC_H
#define _MY_MISC_H
void delay_ms(unsigned short nms);
void delay_us(unsigned long Nus);
void delay_init(unsigned char SYSCLK);
void led_init();
unsigned short Packet_CRC(unsigned char *Data,unsigned char Data_length);
void ctl_int(int line, int flag);
void led(int on);
void Debug_uart_Init();
uint16_t crc16(const unsigned char *buf, int len);
void rs485_init();
void rs485_send(unsigned char *cmd, unsigned short len);
#endif
