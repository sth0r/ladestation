#include "Macroes.h"

void UART_Init()
{
	//SREG = (1<<I); //Enable global interrupts // Same as: sei();
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	/* Set frame format: 8data, 1stop bit */
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);
	UCSR0A = (1<<U2X0);//Double the USART Transmission Speed
	/* Set baud rate */
	//UBRR0H = (unsigned char)(baud>>8);
	UBRR0L = 0x67; // 19200
}

char UART_Receive()
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );
	/* Get and return received data from buffer */
	return UDR0;
}

void UART_Transmit(char data)
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

void UART_Transmit_String(char *str)
{
	while (*str > 0)              // Continue until the pointer reaches -
	{                             // the zero termination of the string
		UART_Transmit(*str);      // Send the value of the pointer address
		str++;                    // Increment pointer
	}
}