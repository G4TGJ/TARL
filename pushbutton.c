 /*
 * pushbutton.c
 *
 * Debounce pushbuttons.
 *
 * Created: 07/09/2020
 * Author : Richard Tomlinson G4TGJ
 */ 

#include "config.h"
#include "pushbutton.h"
#include "millis.h"

// Debounce a pushbutton
void debouncePushbutton( bool bDown, bool *pbShortPress, bool *pbLongPress, uint16_t debounceTime, uint16_t longPressTime, struct sDebounceState *pDebounceState)
{
    // Get the time now
    uint32_t currentTime = millis();

    // Dummy in case the long press pointer is null
    bool bLongPress;
    
    // If the long press pointer is null then just use a dummy
    if( pbLongPress == 0 )
    {
        pbLongPress = &bLongPress;
    }

    // Nothing pressed yet
    *pbShortPress = false;
    *pbLongPress = false;
        
    // Debounce the pushbutton with a state machine
    switch( pDebounceState->state )
    {
        // Currently idle i.e. nothing pressed
        case PUSHBUTTON_IDLE:
            // If the pushbutton is pressed then debounce it
            if( bDown )
            {
                // Start the debounce timer
                pDebounceState->debounceTimer = currentTime + debounceTime;
                pDebounceState->state = PUSHBUTTON_DOWN;
            }
            break;

        // The pushbutton is down but not yet debounced
        case PUSHBUTTON_DOWN:
            // If the pushbutton is released then back to idle
            // to start debounce timer again
            if( !bDown )
            {
                pDebounceState->state = PUSHBUTTON_IDLE;
            }
            // If pushbutton down long enough then it can be
            // considered pressed.
            // Have to see how long it is pressed for
            else if( currentTime > pDebounceState->debounceTimer )
            {
                // Start the long press time if appropriate
                if( longPressTime > 0 )
                {
                    // Start the long press timer
                    pDebounceState->longPressTimer = currentTime + longPressTime;
                }
                else
                {
                    // Can consider button pressed
                    *pbShortPress = true;
                }
                pDebounceState->state = PUSHBUTTON_PRESSED;
            }
            break;

        // The pushbutton has been down long enough to have
        // been debounced
        case PUSHBUTTON_PRESSED:
            // Switch is no longer pressed so need to start
            // the debounce process
            if( !bDown )
            {
                // Start the debounce timer
                pDebounceState->debounceTimer = currentTime + debounceTime;

                // Also restart the long press timer to
                // prevent a spurious result
                pDebounceState->longPressTimer = currentTime + longPressTime;

                pDebounceState->state = PUSHBUTTON_RELEASED;
            }
            // Are we interested in long presses?
            else if( longPressTime > 0 )
            {
                // The pushbutton has been down long enough that it is
                // a long press
                if( currentTime > pDebounceState->longPressTimer )
                {
                    *pbLongPress = true;
                    pDebounceState->state = PUSHBUTTON_LONG_PRESS;
                }
            }
            else
            {
                // Switch is still down
                *pbShortPress = true;
            }
            break;

        // The pushbutton was down but has now been released
        case PUSHBUTTON_RELEASED:
            // If the pushbutton is down again then keep
            // debouncing
            if( bDown )
            {
                pDebounceState->state = PUSHBUTTON_PRESSED;
            }
            // The pushbutton has been up long enough so it is
            // a short press and we are idle again
            else if( currentTime > pDebounceState->debounceTimer )
            {
                // If the long press time is zero we will will have already
                // reported the button when it was pressed
                if( longPressTime > 0 )
                {
                    *pbShortPress = true;
                }
                pDebounceState->state = PUSHBUTTON_IDLE;
            }
            break;

        // The pushbutton has been pressed a long time so just waiting
        // for it to be released.
        case PUSHBUTTON_LONG_PRESS:
            if( !bDown )
            {
                pDebounceState->state = PUSHBUTTON_IDLE;
            }
            break;

        default:
            // Should never get here
            break;
    }
}

