#include <avr/io.h>
#include <util/delay.h>

void serial_transmit(unsigned char data)  // 0x0C
{
	while( !( UCSR1A & (1 << UDRE1)) );

	UDR1 = data;
}

void serial_string(char *str)
{
	while( *str )	serial_transmit(*str++);
}

unsigned char serial_receive(void)
{
	while ( !(UCSR1A & (1 << RXC)) );

	return UDR1;
}

 // baudrate란 초당 몇 개의 데이터를 보내느냐를 나타낸다
 // BPS는 초당 몇 개의 비트 보냈느냐를 의미
 // 9600bps/8bit라면 baudrate는 1200Baud가 된다
void serial_init(unsigned int baudrate) // B9600(=103)으로 초기화
{
	UCSR1A = 0x00;
	UCSR1B = 0x08;
	UCSR1C = 0x06;
	UBRR1H = baudrate >> 8;
	UBRR1L = baudrate & 0x0FF;
}