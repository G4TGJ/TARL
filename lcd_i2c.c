/*
 * lcd_i2c.c
 *
 * Low level functions for writing to LCD display over I2C
 *
 * Created: 27/06/2020 07:32:25
 * Author : Richard Tomlinson G4TGJ
 */ 

#include "config.h"
#include "lcd.h"
#include "i2c.h"
#include "millis.h"

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define LCD_ENABLE_BIT 0x04 // Enable bit
#define LCD_RW_BIT 0x02     // Read/Write bit
#define LCD_RS_BIT 0x01     // Register select bit

// Mask and position of the 4 data bits within the expander byte
#define DATA_POS 4
#define DATA_BITS (0xF<<DATA_POS)

// Keep track of the current register value
// Default to having the backlight on
static uint8_t regVal = LCD_BACKLIGHT;

// Write to the I2C expander
static void lcd_i2c_write( uint8_t value )
{
    // We use the pre-existing I2C send register function which actually sends the
    // data twice (this simple device has no register addresses)
    i2cSendRegister( LCD_I2C_ADDRESS, value, value );

    regVal = value;
}

// Write to the LCD's data bits
void lcdWriteData( uint8_t value )
{
    lcd_i2c_write( (regVal & ~DATA_BITS) | (value << DATA_POS) );
}

// Set or clear the RS bit
void lcdRS( bool bOn )
{
    lcd_i2c_write( (regVal & ~LCD_RS_BIT) | (bOn ? LCD_RS_BIT : 0) );
}

// Set or clear the EN bit
void lcdEN( bool bOn )
{
    lcd_i2c_write( (regVal & ~LCD_ENABLE_BIT) | (bOn ? LCD_ENABLE_BIT : 0) );
}

// We don't use the RW pin
void lcdRW( bool bOn )
{
}
