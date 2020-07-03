/*
 * millis.h
 *
 * Created: 08/02/2019 13:22:24
 *  Author: Richard
 */ 


#ifndef MILLIS_H_
#define MILLIS_H_

uint32_t millis();
void setup_millis(void);
void delay( uint16_t ms );

// A busy wait when a millisecond is too long
#define delayMicroseconds( us ) __builtin_avr_delay_cycles( F_CPU / 1000000 * us )

#endif /* MILLIS_H_ */