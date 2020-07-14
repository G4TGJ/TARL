/** \file morse.h
 *
 *  \date 11/09/2019
 *  \author Richard Tomlinson G4TGJ
 */ 

#ifndef MORSE_H
#define MORSE_H

#include <inttypes.h>

/// Morse keyer mode
enum eMorseKeyerMode
{
    morseKeyerIambicA = 0,
    morseKeyerIambicB,
    morseKeyerUltimatic,
    MORSE_NUM_KEYER_MODES
};

// Functions exported by the morse module

/// Initialise the morse module.
void morseInit();

/// Get the morse speed in words per minute.
/// 
/// @return Morse speed in WPM
uint8_t morseGetWpm();

/// Set the morse speed in words per minute.
/// 
/// @param[in] wpm Morse speed in wpm
void morseSetWpm( uint8_t wpm );

/// Scan the dot and dash paddles.
/// Called from the main loop.
//////////////////////////////////////////////////////////////////////////
/// The return value can be used to decide whether or not to
/// go into a low power mode - useful for a battery powered keyer without
/// an off switch.
/// 
/// @returns true if active
/// @returns false if idle

bool morseScanPaddles( void );

/// Switch in or out of tune mode.
/// i.e. continuous transmission for tuning ATU etc.
/// 
/// @param[in] bTune True to set tune mode, False to leave tune mode
void morseSetTuneMode( bool bTune );

/// Returns whether or not in tune mode.
/// 
/// @return true if in tune mode
bool morseInTuneMode();

/// Set the keyer mode.
/// 
/// @param[in] eMorseKeyerMode The keyer mode
void morseSetKeyerMode( enum eMorseKeyerMode );

/// Get the keyer mode.
///
/// @return The keyer mode
enum eMorseKeyerMode morseGetKeyerMode();

#endif //MORSE_H
