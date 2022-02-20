/** \file lcd.h
 *
 * \author Richard Tomlinson G4TGJ
 */ 

#ifndef lcd_h
#define lcd_h

#include <inttypes.h>

/// Initialise the LCD driver.
void lcdInit();
    
/// Set the number of rows and columns.
///
/// @param[in] cols Number of columns
/// @param[in] rows Number of rows
void lcdBegin(uint8_t cols, uint8_t rows);

/// Clear the LCD screen.
void lcdClear();

/// Move the cursor back to the first location.
void lcdHome();

/// Turn off the display.
void lcdDisplayOff();

/// Turn on the display.
void lcdDisplayOn();

/// Stop the cursor blinking.
void lcdBlinkOff();

/// Start the cursor blinking.
void lcdBlinkOn();

/// Turn off the cursor.
void lcdCursorOff();

/// Turn on the cursor.
void lcdCcursorOn();

/// Scroll the display left.
void lcdScrollDisplayLeft();

/// Scroll the display right.
void lcdScrollDisplayRight();

/// Set the scroll direction to left to right.
void lcdScrollLeftToRight();

/// Set the scroll direction to right to left.
void lcdScrollRightToLeft();

/// Set the display to automatically scroll.
void lcdAutoscrollOn();

/// Turn off automatic scrolling.
void lcdAutoscrollOff();

/// Set the cursor position.
///
/// @param[in] col Column number
/// @param[in] row Row number
void lcdSetCursor(uint8_t col, uint8_t row);

/// Print text on the LCD screen.
/// 
/// @param[in] string Pointer to the null terminated string
void lcdPrint( const char *string );

/// Turn the backlight on or off
///
/// This function is only available with the I2C interface
/// unless separately implemented
/// @param[in] bOn True to turn on backlight
void lcdBacklight( bool bOn );

// Internal functions
// Do not call these
void lcdIFInit();
void lcdWriteData( uint8_t value );
void lcdRS( bool bOn );
void lcdEN( bool bOn );
void lcdRW( bool bOn );

#endif
