/*
 * Created: 13-02-2014 12:42:01
 *  Author: T
 */
#include "Macroes.h"
#define INTERVAL_SEC 1000
#define INTERVAL_READKEYS 9
#define ACK 0x86
//uint16_t interruptCount = 0;
volatile bool runSec = false,readKeys = false, uartRecived = false, cardPresent = false, dataReady = false; // Bool variables
unsigned int uartData;

void RFID_init() 
{	
	EICRA |= (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00); // Get interrupt on rising edge
	EIMSK |= (1<<INT1)|(1<<INT0); // Enable external interrupts
	DDRD &= ~(1<<DDD3)&~(1<<DDD2); //Set external interrupts INT0 & INT1 as inputs
	DDRB |= (1<<DDB2); //Set SS as output. RFID
	PORTB |= (1<<PORTB2); //Set SS high (Active low)
}

ISR(USART_RX_vect)
{
	uartData = UDR0;
	uartRecived = true;
	//UART_Transmit(uartData);
}

ISR(TIMER1_COMPA_vect) // Comes every 1ms
{
	volatile static uint16_t count1Sec = INTERVAL_SEC,count2ms = INTERVAL_READKEYS;
	if ((--count1Sec) ==0)  // Do if 1 minus countSec = 0
	{
		count1Sec = INTERVAL_SEC; // 1000
		runSec = true;      // Set runTemp to 1. Activating this function to be run next time
	}
	if ((--count2ms) ==0)  // Do if 1 minus count2ms = 0
	{
		count2ms = INTERVAL_READKEYS; // 2
		readKeys = true;      // Set runTemp to 1. Activating this function to be run next time
	}
}

ISR(INT0_vect)
{
	if (!dataReady) cardPresent = true;
}

ISR(INT1_vect)
{
	dataReady = true;
}

void Timer_init() 
{
	TCCR1B = (1<<CS10)|(1<<WGM12);  // Set clock no prescaler(16Mhz) & mode 4 CTC
	OCR1A = 15999;                  // Get interrupt at 15999 = every 1mS
	TIMSK1 = (1<<OCIE1A);            // Enable interrupt compare match
}

int main(void)
{
	double energy=0, power=0;
	uint16_t data=0, lastData=0;
	SPI_MasterInit();
	UART_init();
	RFID_init();
	Timer_init();
	Disp_init();
	ADC_init();
	sei();
	Disp_printString("P:     mW A:");
	Disp_GotoXY(1,2);
	Disp_printString("E:     mWs");
	//char arrayTest[]="123456789abcdefghijklmnopqrstuvw";
	//Disp_printString(arrayTest);
	char displayBuffer[64] = "";
	UART_Transmit_String(displayBuffer);
	char comBuffer[32] = "";
	//unsigned char testSprintf[20];
	//sprintf(testSprintf,"test text here1"); // make string
	//Disp_printString(testSprintf);
	UART_Transmit_String("Card test \n");
    while(1)
    {
		if (cardPresent) // Interrupt 0. From RFID
		{
			Disp_GotoXY(1,1);
			Disp_printString("Card Present");
			UART_Transmit_String("Card Present \n");
			PORTB &= ~(1<<PORTB2); // SS low to start transfer
			SPI_MasterTransmit('U'); //0x55 Command get UID
			PORTB |= (1<<PORTB2); // SS high to end transfer
			//UART_Transmit_String("Get UID \n");
			_delay_us(100);
			cardPresent = false;
		}
		
		if (dataReady) // Interrupt 1. From RFID
		{			
			Disp_GotoXY(1,2);
			Disp_printString("Data Ready");
			UART_Transmit_String("Data Ready \n");
			PORTB &= ~(1<<PORTB2); // SS low to start transfer
			SPI_MasterTransmit(0xF5); // Send dummy data
			PORTB |= (1<<PORTB2); // SS high to end transfer
			_delay_us(100);
			if (SPDR == ACK)
			{
				sprintf(displayBuffer, "UID = ");
				for (int i = 1; i <= 7; i++)
				{
					PORTB &= ~(1<<PORTB2); // SS low to start transfer
					SPI_MasterTransmit(0xF5); // Send dummy data
					PORTB |= (1<<PORTB2); // SS high to end transfer
					_delay_us(100);
					sprintf(comBuffer, "%X", SPDR);
					strcat(displayBuffer,comBuffer);
					//Disp_char('0'+i);	
				}
				strcat(displayBuffer,"\n");
				UART_Transmit_String(displayBuffer);
			} 
			else
			{
				UART_Transmit_String("Command failed \n");
			}
			dataReady = false;
			//UART_Transmit_String("Dummy Data Sent \n");
		}
			
		if (readKeys)
		{
			unsigned char keyP = KBDchar(1);
			if (keyP != 0) Disp_char(keyP);
			readKeys = false;
		}
		
		if (runSec)
		{
			//Disp_GotoXY(1,2);
			//sprintf(adc, "%u", ADC_Sample());
			//Disp_printString(adc);

			data=ADC_Sample();							
			if((lastData!=data) && (data>0)) //Only update lcd if needed and data is not 0
			{							
				Disp_GotoXY(13,1);
				sprintf(displayBuffer, "%4u", data);
				Disp_printString(displayBuffer);
				power = (double)data/409,2; //((2/5)*1023);   //mW 0.4*1023 = 409.2 // uW 0.4*1.023
				Disp_GotoXY(3,1);
				sprintf(displayBuffer, "%.2f", power);
				Disp_printString(displayBuffer);
				lastData=data;
			}
			else if (data==0)
			{
				Disp_GotoXY(13,1);
				sprintf(displayBuffer, "%4u", data);
				Disp_printString(displayBuffer);
				power = 0;
			}		
			energy += power;
			Disp_GotoXY(3,2);
			if (energy < 1000) sprintf(displayBuffer, "%.1f", energy);
			else sprintf(displayBuffer, "%5.f", energy);	
			Disp_printString(displayBuffer);	
			runSec = false;
		}
    }
}