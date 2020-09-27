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
                INVALID,    // 0000 0 0
                CW,         // 0001 1 1
                CCW,        // 0010 2 2
                INVALID,    // 0011 3 3
                CCW,        // 0100 4 4
                INVALID,    // 0101 5 5
                INVALID,    // 0110 6 6
                CW,         // 0111 7 7
                CW,         // 1000 8 8
                INVALID,    // 1001 9 9
                INVALID,    // 1010 A 10
                CCW,        // 1011 B 11
                INVALID,    // 1100 C 12
                CCW,        // 1101 D 13
                CW,         // 1110 E 14
                INVALID     // 1111 F 15
            };

            // Keep track of the transitions
            static uint8_t transition;

            // Work out the transition - shift the previous state up and incorporate the new state
            transition = ((transition&3)<<2) | (bRotaryB<<1) | bRotaryA;

            // We ignore invalid transitions
            if( direction[transition] != INVALID )
            {
                // Keep track of the current state, made up of the current and previous
                // transition. Ideally we'd look for all 4 valid transitions but if
                // the control is noisy we would struggle to detect many clicks so
                // just look for two.
                static uint8_t state;
                state = (state << 4) | transition;

                // If the transitions are 1 followed by 7 in the above table, then it is
                // clockwise
                if( (state == 0x17) )
                {
                    *pbCW = true;
                }
                // If the transitions are 2 followed by b in the above table, then it is
                // counter clockwise
                else if( (state == 0x2b) )
                {
                    *pbCCW = true;
                }
            }
            prevbRotaryA = bRotaryA;
            prevbRotaryB = bRotaryB;
        }
    }
}
