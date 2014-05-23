/*
* StateMachineControl.c
*
* Created: 15-05-2014 09:24:30
*  Author: T
*/
#include "Macroes.h"
#define INTERVAL_SEC 1000
#define INTERVAL_READ_TIMEOUT 100
#define INTERVAL_CONNECT_CAR_TIMEOUT 10000
#define INTERVAL_READKEYS 9
#define KR_PR_MWS 0.5
#define ACK 0x86
#define CLIENT_ID "001"
#define START_CHAR "%%"
#define STOP_CHAR "*"
#define VALIDATE_CARD_COMMAND 'V'
#define LOGIN_COMMAND 'L'
enum state {stateIdle, stateCarConnected, stateCardSwiped, stateTypePassword, stateWrongPassword, stateCharging, stateChargingStopped, stateDisconnectCar, stateUploadToDB, stateDBoffline, stateUnknownCard, stateDisableCard,stateCardReadError,stateConnectCar,stateErrorState};
int state = stateIdle;
int dataIndex = 0;
//uint16_t interruptCount = 0;
volatile bool takeSample = false,chargingActive = false,readKeys = false, uartRecived = false, cardPresent = false, dataReady = false, keypadActive = false,gotUID = false, readTimeout = false,connectCarTimeout = false,startReadTimeout = false,startConnectCarTimeout = false, packetReceived = false,cancel = false; // Bool variables
unsigned int uartData;
char uID[12] = "";
char displayBuffer[64] = "";
char comBuffer[64] = "";
char receiveBuffer[64] = "";
char password[4];
char passResult[4];
//char comPacket[64] = "";

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
	volatile static uint16_t count1Sec = INTERVAL_SEC,count9ms = INTERVAL_READKEYS, count100ms = INTERVAL_READ_TIMEOUT,count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT;
	if (chargingActive)
	{
		if ((--count1Sec) ==0)  // Do if 1 minus countSec = 0
		{
			count1Sec = INTERVAL_SEC; // 1000
			takeSample = true;      // Set runSec to 1. Activating this function to be run next time
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
	
	if (startReadTimeout)
	{
		if ((--count100ms) ==0)  // Do if 1 minus count9ms = 0 and keypadActive = true
		{
			count100ms = INTERVAL_READ_TIMEOUT; //
			readTimeout = true;      // Set readKeys to 1. Activating this function to be run next time
		}
	}
	else count100ms = INTERVAL_READ_TIMEOUT;

	if (startConnectCarTimeout)
	{
		if ((--count10Sec) ==0)  // Do if 1 minus count9ms = 0 and keypadActive = true
		{
			count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT; //
			connectCarTimeout = true;      // Set readKeys to 1. Activating this function to be run next time
		}
	}
	else count10Sec = INTERVAL_CONNECT_CAR_TIMEOUT;
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

void GetUID()
{
	PORTB &= ~(1<<PORTB2); // SS low to start transfer
	SPI_MasterTransmit('U'); //0x55 Command get UID
	PORTB |= (1<<PORTB2); // SS high to end transfer
	//UART_Transmit_String("Get UID \n");
	//_delay_us(100000);
	startReadTimeout = true;
	cardPresent = false;
	while((!dataReady) || (!readTimeout));
	startReadTimeout = false;
	readTimeout = false;
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
			strncpy(uID, displayBuffer+6, 8);
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

void SendCommand(char command)
{
	char commandString[1];
	sprintf(commandString, "%c", command);
	sprintf(comBuffer, START_CHAR);
	strcat(comBuffer, CLIENT_ID);
	strcat(comBuffer, commandString);
	switch(command)
	{
		case 'V':
		{
			strcat(comBuffer, uID);
		}
		break;
		case 'L': // Packet: %001LxxxUIDxxPASS
		{
			strcat(comBuffer, uID);
			strcat(comBuffer, password);
		}
		break;
	}
	strcat(comBuffer, STOP_CHAR);
	UART_Transmit_String("Packet send\n");
	UART_Transmit_String(comBuffer);
}

bool CardKnown()
{
	//char validateCompare[6] = "%001AV*";
	SendCommand(VALIDATE_CARD_COMMAND);
	packetReceived = false;
	//startReadTimeout = true;
	//while(!packetReceived || !readTimeout); // Implement when com is working
	//startReadTimeout = false;
	
	while(!packetReceived);
	UART_Transmit_String("receiveBuffer content\n");
	UART_Transmit_String(receiveBuffer); //Packet expected: %001Atrue*
	memcpy(passResult,receiveBuffer+5, 4);
	passResult[4] = '\0';   /* null character manually added */
	UART_Transmit_String("validate card passResult content\n");
	UART_Transmit_String(passResult);
	memset(receiveBuffer, '\0', sizeof(receiveBuffer));
	if (strncmp (passResult,"true",4) == 0)
	{
		return true;
	} 
	else
	{
		return true;
	}
}

bool ValidatePassword()
{
	for (int i = 0; i < 4;)
	{
		char keyP = KBDchar(1);
		if (keyP != 0 && keyP != 'C')
		{
			password[i] = keyP;
			Disp_char(keyP);
			i++;
		} 
		else if (keyP == 'C')
		{
			cancel = true;
			return false;
		}
	}
	UART_Transmit_String("Password typed\n");
	UART_Transmit_String(password);
	SendCommand(LOGIN_COMMAND);
	packetReceived = false;
	//startReadTimeout = true;
	//while(!packetReceived || !readTimeout); // Implement when com is working
	//startReadTimeout = false;
	while(!packetReceived);
	UART_Transmit_String("receiveBuffer content\n");
	UART_Transmit_String(receiveBuffer); //Packet expected: %001A00000001001truexxxx*
	
	memcpy(passResult,receiveBuffer+16, 4);
	passResult[4] = '\0';   /* null character manually added */
	UART_Transmit_String("passwordResult content\n");
	UART_Transmit_String(passResult);
	memset(receiveBuffer, '\0', sizeof(receiveBuffer));
	if (strncmp (passResult,"true",4) == 0) // if (strncmp (receiveBuffer,"%001A00000001001true",20) == 0)
	{
		return true;
	}
	else
	{
		return true;
	}
}

bool CarConnected()
{
	startConnectCarTimeout = true;
	UART_Transmit_String("wait for timeout\n");
	//while(!carConnected || !connectCarTimeout); // Timeout virker ikke på denne måde?
	while(1)
	{
		if((ADC_Sample() > 10) || (connectCarTimeout)) break;
	}
	startConnectCarTimeout = false;
	if (!connectCarTimeout)
	{
		return true;
	} 
	else 
	{
		UART_Transmit_String("connect car timeout\n");
		connectCarTimeout = false;
		return false;
	}
}

void BeginCharging()
{
	double energy=0, power=0, price = 0;
	uint16_t data=0, lastData=0;
	keypadActive = true;
	chargingActive = true;
	while (chargingActive)
	{
		if (takeSample)
		{
			data=ADC_Sample();
			if((lastData!=data) && data > 0) //Only update display if needed (any change)
			{
				Disp_GotoXY(13,1);
				sprintf(displayBuffer, "%4u", data);
				Disp_printString(displayBuffer);
				//if (data == 0) power = 0; // Avoid dividing by zero
				//else power = (double)data/409,2; //((2/5)*1023);   //mW 0.4*1023 = 409.2 // uW 0.4*1.023
				power = (double)data/409.2; //((2/5)*1023);   //mW 0.4*1023 = 409.2 // uW 0.4*1.023
				Disp_GotoXY(3,1);
				sprintf(displayBuffer, "%.2f", power);
				Disp_printString(displayBuffer);
				lastData=data;
			}
			else if (data == 0) chargingActive = false;
			//else chargingDone = true;
			energy += power;
			Disp_GotoXY(3,2);
			if (energy < 9999) sprintf(displayBuffer, "%.1f", energy);
			else sprintf(displayBuffer, "%5.f", energy);
			Disp_printString(displayBuffer);
			price = energy*KR_PR_MWS;
			Disp_GotoXY(11,2);
			if (price < 99) sprintf(displayBuffer, "%.1f", price);
			else sprintf(displayBuffer, "%4.f", price);
			//sprintf(displayBuffer, "%.1f", price);
			Disp_printString(displayBuffer);
			takeSample = false;
		}
		if (readKeys)
		{
			char keyP = KBDchar(1);
			if (keyP == 'C') chargingActive = false;
			readKeys = false;
		}
	}
	keypadActive = false;
}
bool UploadFinishedCharge()
{
	return false;
}

void TryConnection()
{
	while(1);
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
						//state=stateCardSwiped;
						state = stateConnectCar;
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
						if (cancel)
						{
							cancel = false;
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
					if (ADC_Sample() > 10)
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
					while (ADC_Sample() > 10);
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
