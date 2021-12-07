#include <avr/io.h>
#include <util/delay.h>

#include "key.h"

#define KEY_IN	PINE
#define KEY_DDR	DDRE

volatile char key_flag;

// sw는 모두  pull-up (평소 1)

unsigned char getkey(unsigned char keyin)  // 0xFF
{
	unsigned char key;

	key_flag = 0;

	key = ~KEY_IN & 0xF8;  // ~(1111 1111) & 1111 1000 = 0000 0000
	if( key ) {
		_delay_ms(5);
		key = ~KEY_IN & 0xF8;
		if( key ) {
			if( key == 0x08 )		key = KEY_1;
			else if( key == 0x10 )	key = KEY_2;
			else if( key == 0x20 )	key = KEY_3;
			else if( key == 0x40 )	key = KEY_4;
			else if( key == 0x80 )	key = KEY_5;
		}
	}

	if( key && (key != keyin) )	key_flag = 1;

	return key;
}

void key_init(void)
{
	KEY_DDR &= 0x07;  // 0000 1100 PE를 인풋으로 쓰겠다
}