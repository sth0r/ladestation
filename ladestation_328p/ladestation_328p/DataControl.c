/*
 * DatabaseCom.c
 *
 * Created: 26-05-2014 13:35:22
 *  Author: T
 */ 
#include "Macroes.h"
char uID[12] = "";
char displayBuffer[64] = "";
char comBuffer[64] = "";
char password[5] = "";
char passResult[5] = "";
char amountKr[5] = "";
char amountOere[3] = "";
char timeStamp[6] = "";
char taID[9] = "";
volatile unsigned int usedOere=0, usedKr=0;
char receiveBuffer[64] = "";
volatile unsigned int timeChargedInSeconds = 0;

void GetUID()
{
	PORTB &= ~(1<<PORTB2); // SS low to start transfer
	SPI_MasterTransmit('U'); //0x55 Command get UID
	PORTB |= (1<<PORTB2); // SS high to end transfer
	startComTimeout = true;
	cardPresent = false;
	while(1)//while((!dataReady) || (!comTimeout)); Virker ikke på denne måde?
	{
		if((dataReady) || (comTimeout)) break;
	}
	startComTimeout = false;
	comTimeout = false;
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
		case 'V': // Packet: % 001 V UID----- *
		{
			strcat(comBuffer, uID);
		}
		break;
		case 'L': // Packet: % 001 L UID----- PSWD *
		{
			strcat(comBuffer, uID);
			strcat(comBuffer, password);
		}
		break;
		case 'C': // Packet: % 001 C UID----- KR-- Oe Time- *
		{
			strcat(comBuffer, taID);
			strcat(comBuffer, uID);
			strcat(comBuffer, amountKr);
			strcat(comBuffer, amountOere);
			strcat(comBuffer, timeStamp);
		}
		break;
		case 'E': // Packet: % 001 L UID----- PSWD *
		{
			UART_Transmit_String("Error Command State");
		}
		break;
		default : command='E'; break;
	}
	strcat(comBuffer, STOP_CHAR);
	UART_Transmit_String("Packet send\n");
	UART_Transmit_String(comBuffer);
}

bool CardKnown()
{
	SendCommand(VALIDATE_CARD_COMMAND);
	packetReceived = false;
	startComTimeout = true;
	while(1)
	{
		if((packetReceived) || (comTimeout)) break;
	}
	startComTimeout = false;
	comTimeout = false;
	if (packetReceived)
	{
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
			return false;
		}
	}
	else
	{
		UART_Transmit_String("Cardknown communication timeout\n");
		return false;
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
			cancelPassword = true;
			return false;
		}
	}
	UART_Transmit_String("Password typed\n");
	UART_Transmit_String(password);
	memset(receiveBuffer, '\0', sizeof(receiveBuffer));
	SendCommand(LOGIN_COMMAND);
	packetReceived = false;
	startComTimeout = true;
	while(1)
	{
		if((packetReceived) || (comTimeout)) break;
	}
	startComTimeout = false;
	comTimeout = false;
	if (packetReceived)
	{
		//UART_Transmit_String("receiveBuffer content\n");
		//UART_Transmit_String(receiveBuffer); //Packet expected: %001A00000001---UID--truexxxx*
		memcpy(passResult,receiveBuffer+21, 4);
		passResult[4] = '\0';   /* null character manually added */
		memcpy(taID,receiveBuffer+5, 8);
		taID[9] = '\0';   /* null character manually added */
		UART_Transmit_String("TAID content\n");
		UART_Transmit_String(taID);
		UART_Transmit_String("passwordResult content\n");
		UART_Transmit_String(passResult);
		memset(receiveBuffer, '\0', sizeof(receiveBuffer));
		if (strncmp (passResult,"true",4) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		UART_Transmit_String("Validate password communication timeout\n");
		return false;
	}
}

bool UploadFinishedCharge()
{
	SendCommand(UPLOAD_CHARGE_COMMAND);
	packetReceived = false;
	startComTimeout = true;
	//while(!packetReceived || !comTimeout); // Implement when com is working
	while(1)
	{
		if((packetReceived) || (comTimeout)) break;
	}
	startComTimeout = false;
	comTimeout = false;
	if (packetReceived)
	{
		UART_Transmit_String("receiveBuffer content. Expected: %001Atrue*\n");
		UART_Transmit_String(receiveBuffer); //Packet expected: %001Atrue*
		memcpy(passResult,receiveBuffer+5, 4);
		passResult[4] = '\0';   /* null character manually added */
		UART_Transmit_String("Acknowledge passResult content\n");
		UART_Transmit_String(passResult);
		memset(receiveBuffer, '\0', sizeof(receiveBuffer));
		if (strncmp (passResult,"true",4) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		UART_Transmit_String("Upload Charge Communication timeout\n");
		return false;
	}
	
}

void TryConnection()
{
	UART_Transmit_String("Try connection function\n");
	noConnection = true;
	while(noConnection)
	{
		if (tryConnect)
		{
			UART_Transmit_String("try connect interval\n");
			if (UploadFinishedCharge()) noConnection = false;
		}
		tryConnect = false;
	}
}

bool CarConnected()
{
	startConnectCarTimeout = true;
	UART_Transmit_String("wait for timeout\n");
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
	//UpdatePrice();
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
				power = (double)data/409.2; //((2/5)*1023);   //mW 0.4*1023 = 409.2 // uW 0.4*1.023
				Disp_GotoXY(3,1);
				sprintf(displayBuffer, "%.2f", power);
				Disp_printString(displayBuffer);
				lastData=data;
			}
			else if (data == 0)
			{
				usedKr = floor(price);
				usedOere = (price-usedKr)*100;
				chargingActive = false;
			}
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
			if (keyP == 'C')
			{
				usedKr = floor(price);
				usedOere = (price-usedKr)*100;
				chargingActive = false;
			}
			readKeys = false;
		}
	}
	keypadActive = false;
	UART_Transmit_String("\nKr ");
	sprintf(amountKr, "%04u", usedKr);
	UART_Transmit_String(amountKr);
	UART_Transmit_String("\nOere ");
	sprintf(amountOere, "%02u", usedOere);
	UART_Transmit_String(amountOere);
	UART_Transmit_String("\nTime in seconds");
	sprintf(timeStamp, "%05u", timeChargedInSeconds);
	UART_Transmit_String(timeStamp);
}