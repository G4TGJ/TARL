/*
 * pushbutton.h
 *
 * Created: 07/09/2020 17:47:56
 *  Author: Richard
 */ 


#ifndef PUSHBUTTON_H_
#define PUSHBUTTON_H_

// Current state of the pushbutton debouncing state machine
enum ePushbuttonState
{
    PUSHBUTTON_IDLE,        // Pushbutton is up
    PUSHBUTTON_DOWN,        // Pushbutton is down but not yet debounced
    PUSHBUTTON_PRESSED,     // Pushbutton is now regarded as pressed
    PUSHBUTTON_RELEASED,    // Pushbutton is now regarded as released
    PUSHBUTTON_LONG_PRESS   // Pushbutton held down long enough for a long press
};

// Structure to keep track of the debounce state machine
struct sDebounceState
{
    enum ePushbuttonState   state;          // State machine state
    uint32_t                debounceTimer;  // Debounce timer
    uint32_t                longPressTimer; // Long press timer
};

/// Debounce a pushbutton
///
/// @param[in] bDown True if the pushbutton is down as read from the hardware
/// @param[out] pbShortPress Pointer to boolean set to true if the pushbutton has been pressed for a short time
/// @param[out] pbLongPress Pointer to boolean set to true if the pushbutton has been pressed for a long time or null if not used
/// @param[in] debounceTime Minimum time pushbutton must be down for it to be a short press
/// @param[in] longPressTime Minimum time pushbutton must be down for it to be a long press
/// @param[in] pDebounceState Pointer to structure holding state machine state
void debouncePushbutton( bool bDown, bool *pbShortPress, bool *pbLongPress, uint16_t debounceTime, uint16_t longPressTime, struct sDebounceState *pDebounceState);

#endif /* PUSHBUTTON_H_ */