#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "fnd.h"

#define FND_OUT	PORTB
#define FND_DDR	DDRB

volatile unsigned char fnd_buf[2], digit;
volatile char fnd_flag;

ISR(TIMER2_OVF_vect)
{
	TCNT2 = 256 - 125;

	FND_OUT = 0xF0;

	if( fnd_flag ) {
		FND_OUT = fnd_buf[digit] << 4;
		FND_OUT |= 0x04 << digit;
	}

	digit ^= 0x01;
}

void timer2_init(void)
{
	TCCR2 = 0x04;
	TCNT2 = 256 - 125;
	TIMSK |= 0x40;
}

void fnd_off(void)
{
	fnd_flag = 0;
	FND_OUT = 0x00;
}

void fnd_init(void)
{
	FND_DDR = 0xFF;
	FND_OUT = 0xF0;

	fnd_buf[0] = 0xF0;
	fnd_buf[1] = 0xF0;
	digit = 0;

	fnd_flag = 0;

	timer2_init();
}