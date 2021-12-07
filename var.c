#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "var.h"

volatile int money;

ISR(ADC_vect)
{
	money = (int)(ADC / 20.46) * 100;
	ADCSRA |= 0x40;
}

int var_read(void)
{
	ADCSRA |= 0x40;
	while( !(ADCSRA & 0x10) );

	return ADC;
}

void var_start(void)
{
	ADCSRA |= 0x40;
}

void var_init(void)
{
	ADCSRA = 0x8F;
	ADMUX = 0x00;
}