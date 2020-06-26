/*
 * morse.h
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 

#ifndef MORSE_H
#define MORSE_H

#include <inttypes.h>

// Morse keyer mode
enum eMorseKeyerMode
{
    morseKeyerIambicA = 0,
    morseKeyerIambicB,
    morseKeyerUltimatic,
    MORSE_NUM_KEYER_MODES
};

// Functions exported by the morse module

// Set up the morse module
void morseConfigure();

// Get and set the morse speed in wpm
uint8_t morseGetWpm();
void morseSetWpm( uint8_t wpm );

// Scan the dot and dash paddles
bool morseScanPaddles( void );

// Switch in or out of tune mode i.e. continuous transmission for tuning ATU etc
void morseSetTuneMode( bool bTune );

// Returns true if in tune mode
bool morseInTuneMode();

// Set and get the keyer mode
void morseSetKeyerMode( enum eMorseKeyerMode );
enum eMorseKeyerMode morseGetKeyerMode();

#endif //MORSE_H
