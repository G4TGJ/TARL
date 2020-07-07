/*
 * serial.h
 *
 * Created: 30/01/2020 19:25:25
 * Author : Richard Tomlinson G4TGJ
 */ 


#ifndef SERIAL_H_
#define SERIAL_H_

void serialInit( uint32_t baud);
void serialTransmit( uint8_t data );
void serialTXString( char *string );
uint8_t serialReceive( void );

#endif /* SERIAL_H_ */