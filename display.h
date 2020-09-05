/** \file display.h
 *
 * \date 11/09/2019
 * \author Richard Tomlinson G4TGJ
 */ 
 
#ifndef DISPLAY_H
#define DISPLAY_H

/// Possible cursor states
enum eCursorState
{
    cursorOff,
    cursorUnderline,
    cursorBlink
};

/// Initialise the display module.
void displayInit();

/// Display text on the specified line replacing the existing text
/// or scrolling from the right.
///
/// @param[in] line Line number
/// @param[in] text Pointer to the text to display
/// @param[in] bReplace If true then replace the existing line,
///                     otherwise scroll the text in
void displayText( uint8_t line, char *text, bool bReplace );

/// Set the cursor position and state (off, underline or blink).
///
/// @param[in] col Column number
/// @param[in] line Line number
/// @param[in] state cursorOff, cursorUnderline or cursorBlink
void displayCursor( uint8_t col, uint8_t line, enum eCursorState state );

/// Split a line at the given column. This means text will scroll up to that point.
///
/// @param[in] col Column number
/// @param[in] line Line number
void displaySplitLine( uint8_t col, uint8_t line );

#endif //DISPLAY_H
