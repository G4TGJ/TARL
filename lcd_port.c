/*
 * lcd_port.c
 *
 * Low level functions for writing to LCD display over an IO port
 *
 * Created: 27/06/2020 07:35:54
 * Author : Richard Tomlinson G4TGJ
*/ 

#include "config.h"
#include "lcd.h"

// Initialise the LCD interface
void lcdIFInit()
{
    // Set up LCD pins as outputs
    LCD_RS_DDR     |= (1<<LCD_RS_PIN);
    LCD_ENABLE_DDR |= (1<<LCD_ENABLE_PIN);
    LCD_DATA_DDR_0 |= (1<<LCD_DATA_PIN_0);
    LCD_DATA_DDR_1 |= (1<<LCD_DATA_PIN_1);
    LCD_DATA_DDR_2 |= (1<<LCD_DATA_PIN_2);
    LCD_DATA_DDR_3 |= (1<<LCD_DATA_PIN_3);

    // RW port is often not used
#ifdef LCD_RW_PORT
    LCD_RW_DDR     |= (1<<LCD_RW_PIN);
#endif
}

// Write to the LCD's data bits
void lcdWriteData( uint8_t value )
{
    LCD_DATA_PORT_0 = (LCD_DATA_PORT_0 & ~(1<<LCD_DATA_PIN_0)) | (((value >> 0) & 0x01)<<LCD_DATA_PIN_0);
    LCD_DATA_PORT_1 = (LCD_DATA_PORT_1 & ~(1<<LCD_DATA_PIN_1)) | (((value >> 1) & 0x01)<<LCD_DATA_PIN_1);
    LCD_DATA_PORT_2 = (LCD_DATA_PORT_2 & ~(1<<LCD_DATA_PIN_2)) | (((value >> 2) & 0x01)<<LCD_DATA_PIN_2);
    LCD_DATA_PORT_3 = (LCD_DATA_PORT_3 & ~(1<<LCD_DATA_PIN_3)) | (((value >> 3) & 0x01)<<LCD_DATA_PIN_3);
}

// Set or clear the RS bit
void lcdRS( bool bOn )
{
    LCD_RS_PORT = (LCD_RS_PORT & ~(1<<LCD_RS_PIN)) | (bOn<<LCD_RS_PIN);
}

// Set or clear the EN bit
void lcdEN( bool bOn )
{
    LCD_ENABLE_PORT = (LCD_ENABLE_PORT & ~(1<<LCD_ENABLE_PIN)) | (bOn<<LCD_ENABLE_PIN);
}

// Set or clear the RW bit
// This bit is not always used
void lcdRW( bool bOn )
{
#ifdef LCD_RW_PORT
    LCD_RW_PORT = (LCD_RW_PORT & ~(1<<LCD_RW_PIN)) | (bOn<<LCD_RW_PIN);
#endif
}
