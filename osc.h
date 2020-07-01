/*
 * osc.h
 *
 * Created: 16/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#ifndef OSC_H
#define OSC_H

#include <inttypes.h>

bool oscInit( void );
void oscSetFrequency( uint8_t clock, uint32_t frequency, int8_t q );
void oscSetRXFrequency(uint32_t frequency, bool bCWReverse );
void oscSetTXFrequency(uint32_t frequency);
void oscRXClockEnable( bool bEnable );
void oscTXClockEnable( bool bEnable );
void oscSetXtalFrequency( uint32_t xtal_freq );

#endif //OSC_H
