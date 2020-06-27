#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <avr/io.h>

 #include "config.h"
 #include "lcd.h"
 #include "millis.h"

static uint8_t _displayfunction;
static uint8_t _numlines;
static uint8_t _displaycontrol;
static uint8_t _displaymode;
static uint8_t _row_offsets[4];

static void write4bits(uint8_t value);
static void write8bits(uint8_t value);
static void send(uint8_t value, uint8_t mode);
static inline void command(uint8_t value);

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//

void lcd_init()
{
#ifdef EIGHT_BIT_MODE
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
#else
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
#endif
    
    lcd_begin(16, 1, LCD_5x8DOTS);
}

void lcd_begin(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
    if (lines > 1)
    {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = lines;

    lcd_setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

    // for some 1 line displays you can select a 10 pixel high font
    if ((dotsize != LCD_5x8DOTS) && (lines == 1))
    {
        _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
    delayMicroseconds(50000);

    // Now we pull both RS and R/W low to begin commands
    lcdRS( false );
    lcdEN( false );
    lcdRW( false );
    delayMicroseconds(1000000);

    //put the LCD into 4 bit or 8 bit mode
    if (! (_displayfunction & LCD_8BITMODE))
    {
        // this is according to the hitachi HD44780 datasheet
        // figure 24, pg 46

        // we start in 8bit mode, try to set 4 bit mode
        write4bits(0x03);
        delayMicroseconds(4500); // wait min 4.1ms

        // second try
        write4bits(0x03);
        delayMicroseconds(4500); // wait min 4.1ms
        
        // third go!
        write4bits(0x03);
        delayMicroseconds(150);

        // finally, set to 4-bit interface
        write4bits(0x02);
    }
    else
    {
        // this is according to the hitachi HD44780 datasheet
        // page 45 figure 23

        // Send function set command sequence
        command(LCD_FUNCTIONSET | _displayfunction);
        delayMicroseconds(4500);  // wait more than 4.1ms

        // second try
        command(LCD_FUNCTIONSET | _displayfunction);
        delayMicroseconds(150);

        // third go
        command(LCD_FUNCTIONSET | _displayfunction);
    }

    // finally, set # lines, font size, etc.
    command(LCD_FUNCTIONSET | _displayfunction);

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_display();

    // clear it off
    lcd_clear();

    // Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_setRowOffsets(int row0, int row1, int row2, int row3)
{
    _row_offsets[0] = row0;
    _row_offsets[1] = row1;
    _row_offsets[2] = row2;
    _row_offsets[3] = row3;
}

/********** high level commands, for the user! */
void lcd_clear()
{
    command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
    delayMicroseconds(2000);  // this command takes a long time!
}

void lcd_home()
{
    command(LCD_RETURNHOME);  // set cursor position to zero
    delayMicroseconds(2000);  // this command takes a long time!
}

void lcd_setCursor(uint8_t col, uint8_t row)
{
    const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
    if ( row >= max_lines )
    {
        row = max_lines - 1;    // we count rows starting w/0
    }
    if ( row >= _numlines )
    {
        row = _numlines - 1;    // we count rows starting w/0
    }
    
    command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void lcd_noDisplay()
{
    _displaycontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_display()
{
    _displaycontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void lcd_noCursor()
{
    _displaycontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_cursor()
{
    _displaycontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void lcd_noBlink()
{
    _displaycontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_blink()
{
    _displaycontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void lcd_scrollDisplayLeft(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd_scrollDisplayRight(void)
{
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void lcd_leftToRight(void)
{
    _displaymode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void lcd_rightToLeft(void)
{
    _displaymode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void lcd_autoscroll(void)
{
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void lcd_noAutoscroll(void)
{
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
}

#if 0
// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcd_createChar(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));
    for (int i=0; i<8; i++)
    {
        write(charmap[i]);
    }
}
#endif

/*********** mid level commands, for sending data/cmds */

static inline void command(uint8_t value)
{
    send(value, 0);
}

static inline size_t lcd_write(uint8_t value)
{
    send(value, 1);
    return 1; // assume success
}

/************ low level data pushing commands **********/

// Pulse the enable bit
static void lcd_pulseEnable()
{
    lcdEN( true );
    delayMicroseconds(1);
    lcdEN( false );
    delayMicroseconds(100);   // commands need > 37us to settle
}

static void write4bits( uint8_t value )
{
    lcdWriteData( value );
    lcd_pulseEnable();
}

static void write8bits(uint8_t value) 
{
#if 0
    for (int i = 0; i < 8; i++)
    {
        digitalWrite(_data_pins[i], (value >> i) & 0x01);
    }
    
    pulseEnable();
#endif
}

// write either command or data, with automatic 4/8-bit selection
static void send(uint8_t value, uint8_t mode)
{
    lcdRS( mode );

    if (_displayfunction & LCD_8BITMODE)
    {
        write8bits(value);
    }
    else
    {
        write4bits(value>>4);
        write4bits(value);
    }
}

void lcd_print( const char *string )
{
    for( int i = 0 ; i < strlen(string) ; i++ )
    {
        lcd_write(string[i]);
    }
}
