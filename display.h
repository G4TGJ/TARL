/*
 * display.h
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#ifndef DISPLAY_H
#define DISPLAY_H

// Possible cursor states
enum eCursorState
{
    cursorOff,
    cursorUnderline,
    cursorBlink
};

// Configure the display module
void displayConfigure();

// Display text on the specified line replacing or scrolling
void displayText( uint8_t line, char *text, bool bReplace );

// Set the cursor position and state (off, underline or blink)
void displayCursor( uint8_t col, uint8_t line, enum eCursorState state );

// Update the display - called from the loop
void displayUpdate();

// Split a line at the given column. This means text will scroll up to that
// point.
void displaySplitLine( uint8_t col, uint8_t line );

#endif //DISPLAY_H
