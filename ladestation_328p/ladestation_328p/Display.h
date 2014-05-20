/*
 * Display.h
 *
 * Created: 17-03-2014 16:17:12
 *  Author: T
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_
#include "StateMachineControl.h"
extern void Disp_char(char data);
extern void Disp_command(char command);
extern void Disp_printString (char *str);
extern void Disp_init();
extern void Disp_GotoXY (int x,int y);
extern void Disp_clear (void);
//extern void Disp_printState(state printState);

#endif /* DISPLAY_H_ */