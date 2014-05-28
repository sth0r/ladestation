/*
 * DataControl.h
 *
 * Created: 26-05-2014 13:49:49
 *  Author: T
 */ 


#ifndef DATACONTROL_H_
#define DATACONTROL_H_
#include <stdbool.h>
extern void GetUID();
extern bool CardKnown();
extern bool ValidatePassword();
extern bool UploadFinishedCharge();
extern void TryConnection();
extern bool CarConnected();
extern void BeginCharging();
extern char receiveBuffer[64];
extern volatile unsigned int timeChargedInSeconds;
#endif /* DATACONTROL_H_ */