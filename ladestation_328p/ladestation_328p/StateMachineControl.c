/*
* StateMachineControl.c
*
* Created: 15-05-2014 09:24:30
*  Author: T
*/
#include "Macroes.h"
enum state {stateIdle, stateCarConnected, stateCardSwiped, stateTypePassword, stateWrongPassword, stateCharging, stateChargingStopped, stateDisconnectCar, stateUploadToDB, stateDBoffline, stateUnknownCard, stateDisableCard,stateCardReadError,stateConnectCar,stateErrorState};
volatile bool noConnection = false, tryConnect = false,takeSample = false, chargingActive = false,readKeys = false, uartRecived = false, cardPresent = false, dataReady = false, keypadActive = false,gotUID = false, comTimeout = false,connectCarTimeout = false,startComTimeout = false,startConnectCarTimeout = false, packetReceived = false,cancelPassword = false; // Bool variables
int state = stateIdle;
unsigned int dataIndex = 0;

void RFID_init()
{
	EICRA |= (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00); // Get interrupt on rising edge
	EIMSK |= (1<<INT1)|(1<<INT0); // Enable external interrupts
	DDRD &= ~(1<<DDD3)&~(1<<DDD2); //Set external interrupts INT0 & INT1 as inputs
	DDRB |= (1<<DDB2); //Set SS as output. RFID
	PORTB |= (1<<PORTB2); //Set SS high (Active low)
}

void Timer_init()
{
	TCCR1B = (1<<CS10)|(1<<WGM12);  // Set clock no prescaler(16Mhz) & mode 4 CTC
	OCR1A = 15999;                  // Get interrupt at 15999 = every 1mS
	TIMSK1 = (1<<OCIE1A);            // Enable interrupt compare match
}

ISR(USART_RX_vect)
{
	receiveBuffer[dataIndex] = UDR0;
	if (receiveBuffer[dataIndex] == '*')
	{
		dataIndex = 0;
		packetReceived = true;
	}
	else dataIndex++;
}

ISR(TIMER1_COMPA_vect) // Comes every 1ms
{
	volatile static uint16_t count1Min = INTERVAL_MIN, count1Sec = INTERVAL_SEC,count9ms = INTERVAL_READKEYS, count10S = INTERVAL_COM_TIMEOUT,count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT;
	if (chargingActive)
	{
		if ((--count1Sec) ==0)  // Do if 1 minus count1Sec = 0
		{
			timeChargedInSeconds++;
			count1Sec = INTERVAL_SEC; // 1000
			takeSample = true;      // Activating this function to be run next time
		}
	}
	else count1Sec = INTERVAL_SEC;
	
	if (keypadActive)
	{
		if ((--count9ms) ==0)  // Do if 1 minus count9ms = 0 and keypadActive = true
		{
			count9ms = INTERVAL_READKEYS; //
			readKeys = true;      // Set readKeys to 1. Activating this function to be run next time
		}
	}
	else count9ms = INTERVAL_READKEYS;
	
	if (startComTimeout)
	{
		if ((--count10S) ==0)  // Do if 1 minus count10S = 0 and keypadActive = true
		{
			count10S = INTERVAL_COM_TIMEOUT; //
			comTimeout = true;      // Activating this function to be run next time
		}
	}
	else count10S = INTERVAL_COM_TIMEOUT;

	if (startConnectCarTimeout)
	{
		if ((--count10Sec) ==0)  // Do if 1 minus count10Sec = 0 and keypadActive = true
		{
			count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT; //
			connectCarTimeout = true;      // Activating this function to be run next time
		}
	}
	else count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT;
	
	if (noConnection)
	{
		if ((--count1Min) ==0)  // Do if 1 minus count9ms = 0 and keypadActive = true
		{
			count1Min = INTERVAL_MIN; //
			tryConnect = true;      // Activating this function to be run next time
		}
	}
	else count1Min = INTERVAL_MIN;
}

ISR(INT0_vect)
{
	if (!dataReady) cardPresent = true;
}

ISR(INT1_vect)
{
	dataReady = true;
}

int main(void)
{
	int preState = 99;
	SPI_MasterInit();
	UART_Init();
	RFID_init();
	Timer_init();
	Disp_init();
	ADC_init();
	sei();
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
						preState = state;
						state = stateCardReadError;
					}
				}
				break;
				
				case stateCardReadError:
				{
					UART_Transmit_String("Did not get UID \n");
					Disp_printState(state);
					preState = state;
					state = stateIdle;
				}
				break;
				
				case stateTypePassword:
				{
					UART_Transmit_String("stateTypePassword \n");
					Disp_printState(state);
					if (ValidatePassword())// Get typed password and validate in database
					{
						if (ADC_Sample() < 5)
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
						if (cancelPassword)
						{
							cancelPassword = false;
							preState = state;
							state = stateIdle;
						} 
						else
						{
							preState = state;
							state = stateWrongPassword;
						}
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
					BeginCharging();
					preState = state;
					state = stateChargingStopped;
				}
				break;
				
				case stateChargingStopped:
				{
					UART_Transmit_String("stateChargingStopped \n");
					Disp_printState(state);
					if (ADC_Sample() > 5)
					{
						preState = state;
						state = stateDisconnectCar;
					} 
					else
					{
						preState = state;
						state = stateUploadToDB;
					}
				}
				break;
				
				case stateDisconnectCar:
				{
					UART_Transmit_String("stateDisconnectCar \n");
					Disp_printState(state);
					while (ADC_Sample() > 5);
					preState = state;
					state = stateUploadToDB;
				}
				break;
				
				case stateUploadToDB:
				{
					UART_Transmit_String("stateUploadToDB \n");
					Disp_printState(state);
					if (UploadFinishedCharge())
					{
						preState = state;
						state = stateIdle;
					} 
					else
					{
						preState = state;
						state = stateDBoffline;
					}
				}
				break;
				
				case stateDBoffline:
				{
					UART_Transmit_String("stateDBoffline \n");
					Disp_printState(state);
					TryConnection();
					preState = state;
					state = stateIdle;
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
	}
}
