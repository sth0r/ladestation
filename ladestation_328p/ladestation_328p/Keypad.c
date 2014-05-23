/*
 * Keyboard_driver.c
 *
 * Created: 14-03-2014 10:25:17
 *  Author: Alex
 */ 


#include <avr/io.h>
#include "Macroes.h"

char KeyTranslate (uint8_t keyData)
{
	char kbdChar = 0;
	switch  (keyData)
	{
		case 0b01110111: kbdChar='1'; break;
		case 0b10110111: kbdChar='2'; break;
		case 0b11010111: kbdChar='3'; break;
		case 0b01111011: kbdChar='4'; break;
		case 0b10111011: kbdChar='5'; break;
		case 0b11011011: kbdChar='6'; break;
		case 0b01111101: kbdChar='7'; break;
		case 0b10111101: kbdChar='8'; break;
		case 0b11011101: kbdChar='9'; break;
		case 0b10111110: kbdChar='0'; break;
		case 0b01111110: kbdChar='A'; break;
		case 0b11011110: kbdChar='B'; break;
		case 0b11101110: kbdChar='C'; break;
		case 0b11101101: kbdChar='D'; break;
		case 0b11101011: kbdChar='E'; break;
		case 0b11100111: kbdChar='F'; break;
		case 0b00000000: kbdChar='L'; break;
		case 0b11111111: kbdChar='H'; break;
		case 0b00001111: kbdChar='Q'; break;
		case 0b11110000: kbdChar='P'; break;
		case 0b11001110: Disp_clear(); break;
		default:		 kbdChar='X'; break;
	}
	return kbdChar;
}

bool DebounceKBD(uint8_t keyData)
{
uint8_t static lastKey = 0;
uint8_t static dbCount = 0;
uint8_t static dbLimit = 5;	//Debounce "depth"
bool debounced = false;

	if (keyData == lastKey)
	{
		if (dbCount==dbLimit)
		{
			dbCount=0;
			return debounced = true;
		}
		else
		{
			dbCount++;
			return debounced = false;
		}
	}
	else
	{
		lastKey = keyData;
		dbCount=0;
		return debounced = false;
	}
	return debounced;
}

char KBDchar (uint8_t charReturn)
{
	uint8_t keyData;
	static char keyPrevious = 0, keyPressed = 0;

	DDRC  |= (1<<PORTC0)|(1<<PORTC1)|(1<<PORTC2)|(1<<PORTC3);//C udgang
	PORTC &= (0<<PORTC0)&(0<<PORTC1)&(0<<PORTC2)&(0<<PORTC3);//C lav
	DDRD  &= (0<<PORTD4)&(0<<PORTD5)&(0<<PORTD6)&(0<<PORTD7);//D indgang
	PORTD |= (1<<PORTD4)|(1<<PORTD5)|(1<<PORTD6)|(1<<PORTD7);//D pullup
	_delay_us(50);
	keyData = PIND & 0b11110000; // Coloum
	
	DDRC  &= (0<<PORTC0)&(0<<PORTC1)&(0<<PORTC2)&(0<<PORTC3);//C indgang
	PORTC |= (1<<PORTC0)|(1<<PORTC1)|(1<<PORTC2)|(1<<PORTC3);//C pullup
	DDRD  |= (1<<PORTD4)|(1<<PORTD5)|(1<<PORTD6)|(1<<PORTD7);//D udgang
	PORTD &= (0<<PORTD4)&(0<<PORTD5)&(0<<PORTD6)&(0<<PORTD7);//D lav
	_delay_us(50);
	keyData |= (PINC & 0b00001111); // Row
	if (keyData != 0xFF)
	{
		keyPressed = KeyTranslate(keyData);
		if ((keyPressed != keyPrevious) && DebounceKBD(keyData))
		{
			keyPrevious = keyPressed;
			if (charReturn != 0)
			{
				return keyPressed;
			}
			else return keyData;
		}
		else
		{
			return 0;
		}		
	}
	keyPrevious = 0;
	return 0;
}