/*
 * StateMachineControl.h
 *
 * Created: 16-05-2014 21:15:17
 *  Author: T
 */ 


#ifndef STATEMACHINECONTROL_H_
#define STATEMACHINECONTROL_H_
#define INTERVAL_SEC 1000
#define INTERVAL_MIN 60000
#define INTERVAL_COM_TIMEOUT 10000
#define INTERVAL_CONNECT_CAR_TIMEOUT 10000
#define INTERVAL_READKEYS 9
#define KR_PR_MWS 0.5
#define ACK 0x86
#define CLIENT_ID "001"
#define START_CHAR "%%"
#define STOP_CHAR "*"
#define VALIDATE_CARD_COMMAND 'V'
#define LOGIN_COMMAND 'L'
#define UPLOAD_CHARGE_COMMAND 'C'
#include <stdbool.h>
extern volatile bool noConnection, tryConnect, takeSample, chargingActive, readKeys, uartRecived, cardPresent, dataReady, keypadActive,gotUID, comTimeout,connectCarTimeout,startComTimeout,startConnectCarTimeout, packetReceived,cancelPassword; // Bool variables

#endif /* STATEMACHINECONTROL_H_ */