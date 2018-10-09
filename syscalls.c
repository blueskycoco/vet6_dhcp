#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
extern void DBG_PutChar(char ptr);
#define ITM_ENA   (*(volatile unsigned int*)0xE0000E00) // ITM Enable
#define ITM_TPR   (*(volatile unsigned int*)0xE0000E40) // Trace Privilege Register
#define ITM_TCR   (*(volatile unsigned int*)0xE0000E80) // ITM Trace Control Reg.
#define ITM_LSR   (*(volatile unsigned int*)0xE0000FB0) // ITM Lock Status Register
#define DHCSR     (*(volatile unsigned int*)0xE000EDF0) // Debug register
#define DEMCR     (*(volatile unsigned int*)0xE000EDFC) // Debug register
#define TPIU_ACPR (*(volatile unsigned int*)0xE0040010) // Async Clock presacler register
#define TPIU_SPPR (*(volatile unsigned int*)0xE00400F0) // Selected Pin Protocol Register
#define DWT_CTRL  (*(volatile unsigned int*)0xE0001000) // DWT Control Register
#define FFCR      (*(volatile unsigned int*)0xE0040304) // Formatter and flush Control Register

#define ITM_STIM_U32  (*(volatile unsigned int*)0xE0000000)
#define ITM_STIM_U8   (*(volatile char*)0xE0000000)

unsigned int ITM_PORT_BIT0 = 0;

unsigned int TargetDiv = 32;

void SWO_Enable( void )
{
	unsigned int StimulusRegs;
	DEMCR |= ( 1 << 24 );
	ITM_LSR = 0xC5ACCE55;
	StimulusRegs = ITM_ENA;
	StimulusRegs &= ~( 1 << ITM_PORT_BIT0 );
	ITM_ENA = StimulusRegs; 
	ITM_TCR = 0;            

	TPIU_SPPR = 0x00000002;     
	TPIU_ACPR = TargetDiv - 1;
	ITM_TPR = 0x00000000;
	DWT_CTRL = 0x400003FE;
	FFCR = 0x00000100;
	ITM_TCR = 0x1000D;
	ITM_ENA = StimulusRegs | ( 1 << ITM_PORT_BIT0 );
}

void SWO_PrintChar( char c )
{
	if ( ( DHCSR & 1 ) != 1 )
		return;

	if ( ( DEMCR & ( 1 << 24 ) ) == 0 )
		return;

	if ( ( ITM_TCR & ( 1 << 22 ) ) == 1 )
		return;

	if ( ( ITM_ENA & 1 ) == 0 )
		return;

	while ( ( ITM_STIM_U8 & 1 ) == 0 )
	{

	}

	ITM_STIM_U8 = c;
}
int _close(int file) {
	return 0;
}

int _fstat(int file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file) {
	return 1;
}

int _lseek(int file, int ptr, int dir) {
	return 0;
}

int _open(const char *name, int flags, int mode) {
	return -1;
}

int _read(int file, char *ptr, int len) {
	int todo;//,ch;
	if(len == 0)
		return 0;
	// uart_wait_rcv();
	//*ptr++ = uart_read();
	for(todo = 1; todo < len; todo++) {
		//	ch=uart_read();
		// if(ch==-1) { break; }
		//*ptr++ = ch;
	}
	return todo;
}

char *heap_end = 0;
caddr_t _sbrk(int incr) {
	extern char _end; /* Defined by the linker */
	extern char __cs3_heap_end; /* Defined by the linker */
	char *prev_heap_end;
	if (heap_end == 0) {
		heap_end = &_end;
	}  
	prev_heap_end = heap_end;
	if (heap_end + incr > &__cs3_heap_end) {
		/* Heap and stack collision */
		return (caddr_t)0;
	}
	heap_end += incr;
	return (caddr_t) prev_heap_end;
}

int _write(int file, char *ptr, int len) {
	int todo;
	for (todo = 0; todo < len; todo++) {
		DBG_PutChar(*ptr++);
		//SWO_PrintChar(*ptr++);
	}
	return len;
}
