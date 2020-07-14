/** \file millis.h
 *
 *  \date 08/02/2019 13:22:24
 *  \author: Richard Tomlinson G4TGJ
 */ 

#ifndef MILLIS_H_
#define MILLIS_H_

/// Return the number of milliseconds since the box started.
/// Wraps after approx 49 days.
/// 
/// @return Number of milliseconds
uint32_t millis();

/// Initialise the millisecond timer.
void millisInit(void);

/// Delay a number of milliseconds.
/// 
/// @param[in] ms Number of milliseconds to wait
void delay( uint16_t ms );

/// Delay a number of microseconds.
/// 
/// @param[in] us Number of microseconds to wait
#define delayMicroseconds( us ) __builtin_avr_delay_cycles( F_CPU / 1000000 * us )

#endif /* MILLIS_H_ */