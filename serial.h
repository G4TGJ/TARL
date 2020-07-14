/** \file serial.h
 *
 *  \date 30/01/2020 19:25:25
 *  \author Richard Tomlinson G4TGJ
 */ 


#ifndef SERIAL_H_
#define SERIAL_H_

/// Initialise the serial driver.
/// 
/// @param[in] baud Baud rate
void serialInit( uint32_t baud);

/// Transmit a byte over the serial line.
///
/// @param[in] data The byte to send
void serialTransmit( uint8_t data );

/// Transmit a null terminated string over the serial line.
///
/// @param[in] string Pointer to the string to send
void serialTXString( char *string );

/// Receive a byte from the serial line.
///
/// If the receive buffer is empty then NUL is returned.
///
/// @return The byte received, or NUL.
uint8_t serialReceive( void );

#endif /* SERIAL_H_ */