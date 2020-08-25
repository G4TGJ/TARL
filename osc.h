/** \file osc.h
 *
 *  \date 16/09/2019
 *  \author Richard Tomlinson G4TGJ
 */ 
 
#ifndef OSC_H
#define OSC_H

#include <inttypes.h>

/// Initialise the oscillator.
///
/// @returns true if successful
/// @returns false if unable to talk to it or it doesn't initialise properly
bool oscInit( void );

/// Set the clock to the given frequency with optional quadrature.
//
/// quadrature is only used for clock 1 - it is ignored for the others.
///
/// +ve is CLK1 leads CLK0 by 90 degrees
/// -ve is CLK1 lags  CLK0 by 90 degrees
///
/// 0 is no quadrature i.e. set the frequency as normal.
///
/// When quadrature is set for clock 1 then it is set to the same frequency as clock 0.
/// 
/// @param[in] clock Clock output to set
/// @param[in] frequency Frequency in hertz
/// @param[in] q Quadrature mode
void oscSetFrequency( uint8_t clock, uint32_t frequency, int8_t q );

/// Enable/disable a clock output.
///
/// @param[in] clock Clock output to control
/// @param[in] bEnable true to enable, false to disable
void oscClockEnable( uint8_t clock, bool bEnable );

/// Set the crystal frequency.
///
/// @param[in] xtal_freq Crystal frequency (in hertz)
void oscSetXtalFrequency( uint32_t xtal_freq );

#endif //OSC_H
