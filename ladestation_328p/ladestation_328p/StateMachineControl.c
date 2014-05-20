/*
* StateMachineControl.c
*
* Created: 15-05-2014 09:24:30
*  Author: T
*/
#include "Macroes.h"
#define INTERVAL_SEC 1000
#define INTERVAL_READ_TIMEOUT 100
#define INTERVAL_READKEYS 9
#define ACK 0x86
#define CLIENT_ID "001"
enum state {stateIdle, stateCarConnected, stateCardSwiped, stateTypePassword, stateWrongPassword, stateCharging, stateChargingStopped, stateDisconnectCar, stateUploadToDB, stateDBoffline, stateUnknownCard, stateDisableCard,stateCardReadError,stateConnectCar,stateErrorState};
int state = stateIdle;
//uint16_t interruptCount = 0;
volatile bool runSec = false,readKeys = false, uartRecived = false, cardPresent = false, dataReady = false, keypadActive = false,gotUID = false, readTimeout = false,startReadTimeout = false; // Bool variables
unsigned int uartData;
char uID[12] = "";
char displayBuffer[64] = "";
char comBuffer[32] = "";

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
	volatile static uint16_t count1Sec = INTERVAL_SEC,count9ms = INTERVAL_READKEYS, count100ms = INTERVAL_READ_TIMEOUT;
	if ((--count1Sec) ==0)  // Do if 1 minus countSec = 0
	{
		count1Sec = INTERVAL_SEC; // 1000
		//runSec = true;      // Set runSec to 1. Activating this function to be run next time
	}
	if (((--count9ms) ==0) && (keypadActive))  // Do if 1 minus count9ms = 0 and keypadActive = true
	{
		count9ms = INTERVAL_READKEYS; //
		readKeys = true;      // Set readKeys to 1. Activating this function to be run next time
	}
	if (startReadTimeout)
	{
		if ((--count100ms) ==0)  // Do if 1 minus count9ms = 0 and keypadActive = true
		{
			count100ms = INTERVAL_READ_TIMEOUT; //
			readTimeout = true;      // Set readKeys to 1. Activating this function to be run next time
		}
	}
	else
	{
		count100ms = INTERVAL_READ_TIMEOUT;
	}
}

ISR(INT0_vect)
{
	if (!dataReady) cardPresent = true;
	//UART_Transmit_String("card present");
}

ISR(INT1_vect)
{
	dataReady = true;
	//UART_Transmit_String("data ready");
}

void Timer_init()
{
	TCCR1B = (1<<CS10)|(1<<WGM12);  // Set clock no prescaler(16Mhz) & mode 4 CTC
	OCR1A = 15999;                  // Get interrupt at 15999 = every 1mS
	TIMSK1 = (1<<OCIE1A);            // Enable interrupt compare match
}

void Disp_printState()
{
	switch(state)
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
			Disp_printString("type password");
			Disp_GotoXY(1,2);
		}
		break;
		case stateUnknownCard :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("card unknown");
			_delay_ms(2000);
		}
		break;
		case stateWrongPassword :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("wrong password");
			Disp_GotoXY(1,2);
			Disp_printString("try again:");
			//_delay_ms(2000);
		}
		break;
		case stateDisableCard :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("card blocked");
			Disp_GotoXY(1,2);
			Disp_printString("contact service");
			_delay_ms(2000);
		}
		break;
		case stateCardReadError :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("card read error");
			Disp_GotoXY(1,2);
			Disp_printString("swipe again");
			_delay_ms(2000);
		}
		break;
		case stateCharging:
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("charging data");
			Disp_GotoXY(1,2);
			Disp_printString("xxxxxxxxxxxx");
		}
		break;
		case stateConnectCar:
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Connect Car");
			Disp_GotoXY(1,2);
			Disp_printString("to continue");
		}
		break;
		case stateErrorState :
		{
			Disp_clear();
			Disp_GotoXY(1,1);
			Disp_printString("Error State");
			_delay_ms(2000);
		}
		break;
		default : state=stateErrorState; break;
	}
}

void GetUID()
{
	PORTB &= ~(1<<PORTB2); // SS low to start transfer
	SPI_MasterTransmit('U'); //0x55 Command get UID
	PORTB |= (1<<PORTB2); // SS high to end transfer
	//UART_Transmit_String("Get UID \n");
	//_delay_us(100000);
	startReadTimeout = true;
	cardPresent = false;
	while(!dataReady || !readTimeout);
	if (dataReady)
	{
		UART_Transmit_String("Data Ready \n");
		PORTB &= ~(1<<PORTB2); // SS low to start transfer
		SPI_MasterTransmit(0xF5); // Send dummy data
		PORTB |= (1<<PORTB2); // SS high to end transfer
		_delay_us(100);
		if (SPDR == ACK)
		{
			sprintf(displayBuffer, "UID = ");
			for (int i = 1; i <= 4; i++)
			{
				PORTB &= ~(1<<PORTB2); // SS low to start transfer
				SPI_MasterTransmit(0xF5); // Send dummy data
				PORTB |= (1<<PORTB2); // SS high to end transfer
				_delay_us(100);
				sprintf(comBuffer, "%X", SPDR);
				strcat(displayBuffer,comBuffer);
				strcat(uID, comBuffer);
				//Disp_char('0'+i);
			}
			strcat(displayBuffer,"\n");
			UART_Transmit_String(displayBuffer);
			UART_Transmit_String(uID);
			gotUID = true;
		}
		else
		{
			UART_Transmit_String("Command failed \n");
			gotUID = false;
		}
		dataReady = false;
	} 
	else
	{
		UART_Transmit_String("RFID reader timeout \n");
		dataReady = false;
		gotUID = false;
	}
}

bool CardKnown()
{
	/*sprintf(comBuffer, "%");
	strcat(comBuffer, CLIENT_ID);
	strcat(comBuffer, uID);
	strcat(comBuffer, "*");
	UART_Transmit_String(comBuffer); // validate packet*/
	return true;
}

bool ValidatePassword()
{
	keypadActive = true;
	return true;
}

bool CarConnected()
{
	return true;
}

int main(void)
{
	double energy=0, power=0;
	uint16_t data=0, lastData=0;
	int preState = 99;
	SPI_MasterInit();
	UART_init();
	RFID_init();
	Timer_init();
	Disp_init();
	ADC_init();
	sei();
	Disp_printString("x");
	//Disp_printString("P:     mW A:");
	//Disp_GotoXY(1,2);
	//Disp_printString("E:     mWs");
	//char arrayTest[]="123456789abcdefghijklmnopqrstuvw";
	//Disp_printString(arrayTest);
	//UART_Transmit_String(displayBuffer);
	//unsigned char testSprintf[20];
	//sprintf(testSprintf,"test text here1"); // make string
	//Disp_printString(testSprintf);
	UART_Transmit_String("Card test \n");
	while(1)
	{
		if ((state != preState) || (cardPresent && (state == stateIdle)))
		{
			switch(state)
			{
				case stateIdle :
				{
					UART_Transmit_String("stateIdle \n");
					Disp_printState(state);
					if(cardPresent)
					{
						UART_Transmit_String("Card Present \n");
						GetUID();
						preState = state;
						state=stateCardSwiped;
					}
					else preState = state;
				}
				break;
				
				case stateCardSwiped :
				{
					UART_Transmit_String("stateCardSwiped \n");
					if (gotUID)
					{
						UART_Transmit_String("got UID \n");
						if (CardKnown()) // Find card ID in database
						{
							preState = state;
							state = stateTypePassword;
						}
						else
						{
							preState = state;
							state = stateUnknownCard;
						}
						gotUID = false;
					}
					else
					{
						UART_Transmit_String("Did not get UID \n");
						Disp_printState(stateCardReadError);
						preState = state;
						state = stateIdle;
					}
				}
				break;
				
				case stateTypePassword:
				{
					UART_Transmit_String("stateTypePassword \n");
					Disp_printState(state);
					if (ValidatePassword())// Get typed password and validate in database
					{
						if (ADC_Sample() < 10)
						{
							preState = state;
							state = stateConnectCar;
						}
						else
						{
							preState = state;
							state = stateCharging;
						}
					}
					else
					{
						preState = state;
						state = stateWrongPassword;
					}
				}
				break;
				
				case stateUnknownCard:
				{
					UART_Transmit_String("stateUnknownCard \n");
					Disp_printState(state);
					preState = state;
					state = stateIdle;
				}
				break;
				
				case stateWrongPassword:
				{
					UART_Transmit_String("stateWrongPassword \n");
					Disp_printState(state);
					preState = state;
					state = stateTypePassword;
				}
				break;
				
				case stateConnectCar:
				{
					UART_Transmit_String("stateConnectCar \n");
					Disp_printState(state);
					if (CarConnected())
					{
						preState = state;
						state = stateCharging;
					}
					else
					{
						preState = state;
						state = stateIdle;
					}
				}
				break;
				
				case stateCharging:
				{
					UART_Transmit_String("stateCharging \n");
					Disp_printState(state);
					preState = state;
					//state = stateIdle;
				}
				break;
				
				case stateErrorState:
				{
					UART_Transmit_String("stateErrorState \n");
					Disp_printState(state);
					preState = state;
					//state = stateIdle;
				}
				break;
				
				default : state=stateErrorState; break;
			}
		}
		
		/*
		if (dataReady) // Interrupt 1. From RFID
		{
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
				gotUID = true;
			}
			else
			{
				UART_Transmit_String("Command failed \n");
				gotUID = false;
			}
			dataReady = false;
		}
		*/
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
