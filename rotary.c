/*
 * rotary.c
 *
 * Read and debounce a rotary control and its pushbutton.
 *
 * Created: 27/06/2020 11:14:21
 * Author : Richard Tomlinson G4TGJ
 */ 

#include "config.h"
#include "rotary.h"
#include "pushbutton.h"
#include "io.h"
#include "millis.h"

// Read the rotary control and decide which direction it is moving in
// Also handle the pushbutton. Debounce it and decide if it's a short or long press
//
// bool *pbCW           Pointer to a boolean that is set to true if control turned clockwise
// bool *pbCCW          Pointer to a boolean that is set to true if control turned counter clockwise
// bool *pbShortPress   Pointer to a boolean that is set to true if the pushbutton is pressed for a short time
// bool *pbLongPress    Pointer to a boolean that is set to true if the pushbutton is pressed for a long time
void readRotary( bool *pbCW, bool *pbCCW, bool *pbShortPress, bool *pbLongPress )
{
    // State of the rotary control
    bool bRotaryA;
    bool bRotaryB;
    bool bRotarySw;

    // Keep track of the the rotary control states
    static bool prevbRotaryA;
    static bool prevbRotaryB;

    // Switch debounce state
    static struct sDebounceState debounceState;

    // Check that the pointers are sane
    if( pbCW && pbCCW && pbShortPress && pbLongPress)
    {
        // Start by assuming nothing pressed
        *pbCW = false;
        *pbCCW = false;
        *pbShortPress = false;
        *pbLongPress = false;

        // Read the rotary control
        ioReadRotary( &bRotaryA, &bRotaryB, &bRotarySw );

        // Debounce the pushbutton
        debouncePushbutton( bRotarySw, pbShortPress, pbLongPress, ROTARY_BUTTON_DEBOUNCE_TIME, ROTARY_LONG_PRESS_TIME, &debounceState);

        // Only do something with the rotary control if there is a change
        if( (bRotaryA != prevbRotaryA) ||
        (bRotaryB != prevbRotaryB) )
        {
            // We need to debounce the control
            // The two outputs run in a set sequence so we ensure this is
            // followed.
            
            // We code each transition into 4 bits. The bottom two bits contain
            // the current state of A and B. The top two bits contain the previous
            // state. We can then look up whether the transition is valid and
            // which direction it is going in.
            // Each transition is invalid or valid for one direction
            enum eTransition
            {
                INVALID,
                CW,
                CCW
            };
            const enum eTransition direction[] =
            {
                INVALID,
                CW,
                CCW,
                INVALID,
                CCW,
                INVALID,
                INVALID,
                CW,
                CW,
                INVALID,
                INVALID,
                CCW,
                INVALID,
                CCW,
                CW,
                INVALID
            };

            // Keep track of the transitions
            static uint8_t transition;

            // The previous direction - need enough transitions in the same direction
            static int8_t prevDirection;

            // How many transitions in the same direction
            // We start with a count of 1 because the first transition is always
            // in a new direction
            static uint8_t countDirection = 1;

            // Work out the transition - shift the previous state up and incorporate the new state
            transition = ((transition&3)<<2) | (bRotaryB<<1) | bRotaryA;

            // For a valid rotation need 4 valid rotations in that direction
            countDirection++;
            if( direction[transition] == prevDirection )
            {
                if( countDirection == 4 )
                {
                    // Have got our 4 transition in the same direction
                    if( prevDirection == CW )
                    {
                        *pbCW = true;
                    }
                    else
                    {
                        *pbCCW = true;
                    }

                    // Start counting again
                    countDirection = 0;
                }
            }
            else
            {
                // Invalid so start counting again
                // Since this invalid transition is probably a bounce we start with a
                // count of 1 since the next transition is probably going to be different
                countDirection = 1;
                prevDirection = direction[transition];
            }

            prevbRotaryA = bRotaryA;
            prevbRotaryB = bRotaryB;
        }
    }
}
