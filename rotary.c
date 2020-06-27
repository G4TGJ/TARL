/*
 * rotary.c
 *
 * Read and debounce a rotary control and its switch.
 *
 * Created: 27/06/2020 11:14:21
 * Author : Richard Tomlinson G4TGJ
 */ 

#include "config.h"
#include "rotary.h"
#include "io.h"
#include "millis.h"

// Debounce a switch
static void debounceSwitch( bool bSwitchDown, bool *pbShortPress, bool *pbLongPress, uint16_t debounceTime, uint16_t longPressTime)
{
    // Current state of the switch debouncing machine
    static enum
    {
        SWITCH_IDLE,        // Switch is up
        SWITCH_DOWN,        // Switch is down but not yet debounced
        SWITCH_PRESSED,     // Switch is now regarded as pressed
        SWITCH_RELEASED,    // Switch is now regarded as released
        SWITCH_LONG_PRESS   // Switch held down long enough for a long press
    } switchState = SWITCH_IDLE;

    // Switch debounce timer
    static uint32_t debounceTimer;

    // Switch long press timer
    static uint32_t longPressTimer;

    // Get the time now
    uint32_t currentTime = millis();

    // Dummy in case the long press pointer is null e.g. for rotary control
    bool bLongPress;
    
    // If the long press pointer is null then just use a dummy
    if( pbLongPress == 0 )
    {
        pbLongPress = &bLongPress;
    }
    
    // Debounce the switch with a state machine
    switch( switchState )
    {
        // Currently idle i.e. nothing pressed
        case SWITCH_IDLE:
        // If the switch is pressed then debounce it
        if( bSwitchDown )
        {
            // Start the debounce timer
            debounceTimer = currentTime + debounceTime;
            switchState = SWITCH_DOWN;
        }
        break;

        // The switch is down but not yet debounced
        case SWITCH_DOWN:
        // If the switch is released then back to idle
        // to start debounce timer again
        if( !bSwitchDown )
        {
            switchState = SWITCH_IDLE;
        }
        // If switch down long enough then it can be
        // considered pressed.
        // Have to see how long it is pressed for
        else if( currentTime > debounceTimer )
        {
            // Start the long press time if appropriate
            if( longPressTime > 0 )
            {
                // Start the long press timer
                longPressTimer = currentTime + longPressTime;
            }
            else
            {
                // Can consider button pressed
                *pbShortPress = true;
            }
            switchState = SWITCH_PRESSED;
        }
        break;

        // The switch has been down long enough to have
        // been debounced
        case SWITCH_PRESSED:
        // Switch is no longer pressed so need to start
        // the debounce process
        if( !bSwitchDown )
        {
            // Start the debounce timer
            debounceTimer = currentTime + debounceTime;

            // Also restart the long press timer to
            // prevent a spurious result
            longPressTimer = currentTime + longPressTime;

            switchState = SWITCH_RELEASED;
        }
        // Are we interested in long presses?
        else if( longPressTime > 0 )
        {
            // The switch has been down long enough that it is
            // a long press
            if( currentTime > longPressTimer )
            {
                *pbLongPress = true;
                switchState = SWITCH_LONG_PRESS;
            }
        }
        else
        {
            // Switch is still down
            *pbShortPress = true;
        }
        break;

        // The switch was down but has now been released
        case SWITCH_RELEASED:
        // If the switch is down again then keep
        // debouncing
        if( bSwitchDown )
        {
            switchState = SWITCH_PRESSED;
        }
        // The switch has been up long enough so it is
        // a short press and we are idle again
        else if( currentTime > debounceTimer )
        {
            // If the long press time is zero we will will have already
            // reported the button when it was pressed
            if( longPressTime > 0 )
            {
                *pbShortPress = true;
            }
            switchState = SWITCH_IDLE;
        }
        break;

        // The switch has been pressed a long time so just waiting
        // for it to be released.
        case SWITCH_LONG_PRESS:
        if( !bSwitchDown )
        {
            switchState = SWITCH_IDLE;
        }
        break;

        default:
        // Should never get here
        break;
    }
}

// Read the rotary control and decide which direction it is moving in
// Also handle the switch. Debounce it and decide if it's a short or long press
//
// bool *pbCW           Pointer to a boolean that is set to true if control turned clockwise
// bool *pbCCW          Pointer to a boolean that is set to true if control turned counter clockwise
// bool *pbShortPress   Pointer to a boolean that is set to true if the switch is pressed for a short time
// bool *pbLongPress    Pointer to a boolean that is set to true if the switch is pressed for a long time
void readRotary( bool *pbCW, bool *pbCCW, bool *pbShortPress, bool *pbLongPress )
{
    // State of the rotary control
    bool bRotaryA;
    bool bRotaryB;
    bool bRotarySw;

    // Keep track of the the rotary control states
    static bool prevbRotaryA;
    static bool prevbRotaryB;

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

        // Debounce the switch
        debounceSwitch( bRotarySw, pbShortPress, pbLongPress, DEBOUNCE_TIME, LONG_PRESS_TIME);

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
            INVALID};

            // Keep track of the transition
            static uint8_t transition;

            // The previous direction - need enough transitions in the same direction
            static int8_t prevDirection;

            // How many transitions in the same direction
            // We start with a count of 1 because the first transition is always
            // in a new direction
            static uint8_t countDirection = 1;

            // Work out the transition - shift the previous state up and incorporate the new state
            transition = ((transition&3)<<2) | (bRotaryB<<1) | bRotaryA;

            #if 0
            char buf[TEXT_BUF_LEN];
            sprintf( buf, "%c%c%c%c:%d ", (transition&8?'1':'0'), (transition&4?'1':'0'), (transition&2?'1':'0'), (transition&1?'1':'0'), direction[transition] );
            serialTXString( buf );
            #endif
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
