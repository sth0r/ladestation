/*
 * CFile1.c
 *
 * Created: 09-04-2014 13:23:10
 *  Author: T
 */ 
#include "Macroes.h"
void ADC_init()
{
	DDRC &= ~(1<<DDC5);
	ADMUX |= (1<<MUX0)|(1<<MUX2); // External ref and adc5 selected (1<<REFS0)||(1<<MUX1)|(1<<MUX3)
	DIDR0 |= (1<<ADC5D); // Disable digital input buffer
	ADCSRA |= (1<<ADEN)|(1<<ADIF)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);   //enable adc
}

uint16_t ADC_Sample()
{
	ADCSRA|=(1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	return ADC;
}