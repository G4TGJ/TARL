/*
 * lcd_if.c
 *
 * Include either the LCD port interface or the I2C interface
 *
 * Created: 18/10/2020 13:47:03
 * Author : Richard Tomlinson G4TGJ
 */ 

 #include "config.h"

 #if defined LCD_I2C

 #include "lcd_i2c.c"

 #elif defined LCD_PORT

 #include "lcd_port.c"

 #else

 #error "Define either LCD_I2C or LCD_PORT"

 #endif
