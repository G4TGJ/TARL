/*
 * display.c
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>

#include <string.h>
#include <stdio.h>

#include "config.h"
#include "display.h"
#include "lcd.h"

// Keep track of each line
static char textBuf[LCD_HEIGHT][ LCD_WIDTH + 1];

// Keep track of the cursor position
static uint8_t cursorCol, cursorLine;

#ifdef ENABLE_DISPLAY_SPLIT_LINE
// Split point for the line
static uint8_t splitPoint[LCD_HEIGHT];
#endif

// Update the line buffer
// Text scrolls in from the right unless bReplace is true
// in which case the new text replaces the existing line
void displayText( uint8_t line, char *text, bool bReplace )
{
	// Number of chars in new string
	uint8_t newLen = strlen( text );
	
	// Buffer pointer
	char *pBuf = textBuf[line];

    // Ensure the text length is not longer than the line
    if(newLen > LCD_WIDTH)
    {
        newLen = LCD_WIDTH;
    }

    // Only do anything if the line is valid
    if( line < LCD_HEIGHT )
    {
        if( bReplace )
        {
            // Replacing line so copy text over and then pad with spaces
            for( int i = 0 ; i < LCD_WIDTH ; i++ )
            {
                if( i < newLen )
                {
                    pBuf[i] = text[i];
                }
#ifdef ENABLE_DISPLAY_SPLIT_LINE
                // Only pad with spaces up to the split point, if there is one
                else if( (i < splitPoint[line]) || (splitPoint[line] == 0) )
#else
                else
#endif
                {
                    pBuf[i] = ' ';
                }
            }
        }
#ifdef ENABLE_DISPLAY_SPLIT_LINE
        else
        {
            // Copy characters over to the text buffer
            // and then pad with spaces.
            // Start at the split point in the output buffer
            for( int i = splitPoint[line] ; i < LCD_WIDTH ; i++ )
            {
                // Copy over old buffer with space at end for new characters
                if( i < (LCD_WIDTH - newLen) )
                {
                    pBuf[i] = pBuf[i + newLen];
                }
                else
                {
                    // Copy new characters
                    pBuf[i] = text[i - (LCD_WIDTH - newLen)];
                }
            }
        }
#endif
        // Move to the start of the line and print the buffer
        lcd_setCursor( 0, line );
        lcd_print( pBuf );
        
        // Put the cursor back to where it was
        lcd_setCursor( cursorCol, cursorLine );
    }
}

void displayConfigure()
{
    lcd_init();

	// Set up the LCD's number of columns and rows:
	lcd_begin(LCD_WIDTH, LCD_HEIGHT, LCD_5x8DOTS);
	
    // No scrolling and no cursor
	lcd_noAutoscroll();
	displayCursor(0, 0, cursorOff);

	// Initialise all line buffers with spaces
    for( int line = 0 ; line < LCD_HEIGHT ; line++ )
    {
        memset( textBuf[line], ' ', LCD_WIDTH );
        textBuf[line][LCD_WIDTH] = '\0';
    }
}
	
// Set the cursor position and state (off, underline or blink)
void displayCursor( uint8_t col, uint8_t line, enum eCursorState state )
{
    // Keep track of cursor position so we can move it back after updating the display
    cursorCol = col;
    cursorLine = line;
    
    // Move the cursor
    lcd_setCursor( col, line );
    
    // Set it off, underlined or blinking
    switch( state )
    {
        case cursorOff:
            lcd_noCursor();
            lcd_noBlink();
            break;
            
        case cursorUnderline:
            lcd_cursor();
            lcd_noBlink();
            break;
            
        case cursorBlink:
            lcd_cursor();
            lcd_blink();
            break;
    }
}

#ifdef ENABLE_DISPLAY_SPLIT_LINE
// Split a line at the given column. This means text will scroll up to that
// point.
void displaySplitLine( uint8_t col, uint8_t line )
{
    if( (line < LCD_HEIGHT) && (col < LCD_WIDTH) )
    {
        splitPoint[line] = col;
    }
}
#endif
