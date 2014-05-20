#include "Macroes.h"
static uint8_t displayAddress = 1;

void Disp_char(char data)
{
	//static uint8_t displayAddress = 1;
	if (displayAddress == 17) Disp_GotoXY(1,2);
	else if (displayAddress == 33)
	{
		 Disp_GotoXY(1,1);
		 displayAddress = 1;
	}
	DDRC |= (1<<DDC4); // RS output
	PORTC |= (1<<PORTC4); // RS high = data
	SPCR |= (1<<DORD); // Send LSB first
	DDRB |= (1<<DDB1); // Shift register Output Enable
	PORTB |= (1<<PORTB1); // Shift register Output Enable high (active low)
	DDRB |= (1<<DDB0); // Enable display output
	PORTB &= ~(1<<PORTB0); // Enable display low

	SPI_MasterTransmit(data);
	SPCR &= ~(1<<DORD); // Reset to MSB
	PORTB |= (1<<PORTB0); // Enable display high
	PORTB &= ~(1<<PORTB1);
	//PORTB |= (1<<PORTB0); // Enable display high
	_delay_us(50);
	PORTB &= ~(1<<PORTB0); // Enable display low
	//PORTB |= (1<<PORTB1);
	//PORTB &= ~(1<<PORTB1);
	displayAddress++;
}

void Disp_command(char command)
{
	SPCR |= (1<<DORD); // Send LSB first
	DDRB |= (1<<DDB1); // Shift register Output Enable
	PORTB |= (1<<PORTB1); // Shift register Output Enable high (active low)
	DDRB |= (1<<DDB0); // Enable display output
	PORTB &= ~(1<<PORTB0); // Enable display low
	DDRC |= (1<<DDC4); // RS output
	PORTC &= ~(1<<PORTC4); // RS low = instruction
	SPI_MasterTransmit(command);
	SPCR &= ~(1<<DORD); // Reset to MSB first
	PORTB |= (1<<PORTB0); // Enable display high
	PORTB &= ~(1<<PORTB1); // Shift register Output Enable low (active low)
	_delay_us(50);
	PORTB &= ~(1<<PORTB0); // Enable display low
	_delay_us(800);
	//PORTB |= (1<<PORTB1);
	//PORTB &= ~(1<<PORTB1);
}

void Disp_printString (char *str)
{
	while (*str > 0)              // Continue until the pointer reaches -
	{                             // the zero termination of the string
		Disp_char(*str);           // Send the value of the pointer address
		str++;                     // Increment pointer
	}
}

void Disp_init()
{
	_delay_ms(50);
	Disp_command(0b00111000); // 0b00111000
	_delay_us(50);
	Disp_command(0b00001111); // 0b00001111
	_delay_us(500);
	Disp_command(0b00000001); // 0b00000001
	_delay_us(100);
	Disp_command(0b00000110); // 0b00000111
	_delay_us(1000);
}

void Disp_GotoXY (int x,int y)   // Go to position x (max 16) in line y (max 2).
{
	if (y == 2) displayAddress = x + 16;
	else displayAddress = x;
	char lineaddr = ((y-1) * 0x40 + (x-1)) | 0x80;// Put the x position
	Disp_command(lineaddr);           // Send a command with 'lineaddr'
}

void Disp_clear (void)
{
	Disp_command(0x01);      // Send command '0x01' (Display Clear)
}

/*
void Disp_printState(state printState)
{
	switch(printState)
	{
		case stateIdle :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Welcome");
			Disp_GotoXY(1,2);
			Disp_printString("Swipe card");
		}
		break;
		case stateTypePassword :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Type password");
			Disp_GotoXY(1,2);
		}
		break;
		case stateUnknownCard :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Card Unknown");
			_delay_ms(2000);
		}
		break;
		case stateWrongPassword :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Wrong Password");
			Disp_GotoXY(1,2);
			Disp_printString("Try again:");
			//_delay_ms(2000);
		}
		case stateDisableCard :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Card Blocked");
			Disp_GotoXY(1,2);
			Disp_printString("Contact Service");
			_delay_ms(2000);
		}
		case stateCardReadError :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Card read error");
			Disp_GotoXY(1,2);
			Disp_printString("Swipe again");
			_delay_ms(2000);
		}
		default : state=stateIdle; break;
	}
} */