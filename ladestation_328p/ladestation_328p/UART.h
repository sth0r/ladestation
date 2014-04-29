/*
 * IncFile1.h
 *
 * Created: 15-03-2014 11:39:20
 *  Author: T
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

extern void UART_init();
extern char UART_receive();
extern void UART_Transmit(char data);
extern void UART_Transmit_String(char *str);

#endif /* INCFILE1_H_ */