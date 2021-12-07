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

 // baudrate�� �ʴ� �� ���� �����͸� �������ĸ� ��Ÿ����
 // BPS�� �ʴ� �� ���� ��Ʈ ���´��ĸ� �ǹ�
 // 9600bps/8bit��� baudrate�� 1200Baud�� �ȴ�
void serial_init(unsigned int baudrate) // B9600(=103)���� �ʱ�ȭ
{
	UCSR1A = 0x00;
	UCSR1B = 0x08;
	UCSR1C = 0x06;
	UBRR1H = baudrate >> 8;
	UBRR1L = baudrate & 0x0FF;
}