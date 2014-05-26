/*
 * Macroes.h
 *
 * Created: 20-02-2014 11:26:23
 *  Author: T
 */ 


#ifndef MACROES_H_
#define MACROES_H_

#include <stdbool.h>
#include "avr/interrupt.h"
#include "SPI.h"
#include "UART.h"
#include "Display.h"
#include "Keypad.h"
#include "ADC.h"
#include "StateMachineControl.h"
#include "DataControl.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#define F_CPU 16000000UL
#include <util/delay.h>

#define SETBIT(ADDR,BIT)(ADDR |= (1<<BIT)) //Ex  called by:  SETBIT(PORTA,PA2);
#define CLRBIT(ADDR,BIT)(ADDR &= ~(1<<BIT)) //Ex. called by: CLRBIT(PORTA,3);
#define DDRINP(ADDR,BIT)(ADDR &= ~(1<<BIT)) //Ex: called by: DDRINP(DDRB,DDB2);
#define DDROUT(ADDR,BIT)(ADDR |= (1<<BIT))  //Ex: called by: DDROUT(DDRB,DDB2);
#define CHKBIT(ADDR,BIT)(ADDR & (1<< BIT)) //ex: if ( CHKBIT(PINA,2) )  x = 47;


#endif /* MACROES_H_ */