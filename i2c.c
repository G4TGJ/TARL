/*
 * i2c.c
 *
 * I2C driver supporting most AVR hardware.
 *
 * Created: 04/09/2020 20:51:19
 *  Author: Richard Tomlinson
 */ 

 #include <avr/io.h>

 // Select the correct I2C driver

 #if defined TWI0
 #include "i2c_tiny.c"
 #elif defined TWCR
 #include "i2c_mega.c"
 #elif defined USIDR
 #include "USI_TWI_Master.c"
 #else
 #error "No support for I2C"
 #endif