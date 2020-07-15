/** \file rotary.h
 *
 *  \date 27/06/2020 11:22:26
 *  \author Richard Tomlinson G4TGJ
 */ 

#ifndef ROTARY_H_
#define ROTARY_H_

/// Read the rotary control and its push button
/// 
/// @param[out] pbCW Pointer to boolean set to true if the control has been moved clockwise
/// @param[out] pbCCW Pointer to boolean set to true if the control has been moved counter clockwise
/// @param[out] pbShortPress Pointer to boolean set to true if the pushbutton has been pressed for a short time
/// @param[out] pbLongPress Pointer to boolean set to true if the pushbutton has been pressed for a long time
void readRotary( bool *pbCW, bool *pbCCW, bool *pbShortPress, bool *pbLongPress );

#endif /* ROTARY_H_ */