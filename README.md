# TARL
**G4TGJ AVR Radio Library**
 
This library is a set of source files for AVR processors that provide functionality of use in amateur radio applications, although many of the files could also be used in 
non-radio systems.

These files have been tested on the ATTiny85 - testing and full support for the ATMega328P will be added soon.

When adding these files to your project in Atmel Studio you should add them as links rather than copying them over. This way you can easily update the library when I issue
new versions. You should add as links the C files that you need so that they are compiled. You can also link to the header files but this does not bring them into the 
compilation. For the compiler to find them you need to add their path in Project Properties.

## config.h

All source files include config.h. It is used to customise the library for specific hardware, e.g. IO ports, I2C address, frequency ranges.

config.h is not part of the library but should be provided as part of your project.

## LCD

These files support common displays based on the Hitachi chip, connected either directly to I/O lines or via I2C (also requires the I2C driver).

|LCD| |
|----|------|
|lcd.h|LCD Header|
|lcd.c|LCD driver using either:|
|lcd_i2c.c|I2C interface|
|lcd_port.c|Direct port connection|

## Display

Sits on top of the LCD interface to provide some helper display functions.

|Display| |
|----|------|
|display.h|Display Header|
|display.c|Display driver|

## I2C

I2C driver for chips with native 2-wire serial support (such as the ATMega328) and those that use the Universal Serial Interface (USI) to provide I2C (such as the ATTiny85).

|I2C| |
|----|------|
|i2c.h|I2C Header|
|i2c.c|Native I2C driver|
|USI_TWI_Master.c|USI I2C driver|
|USI_TWI_Master.h|Header for USI I2C driver|

## EEPROM

EEPROM driver for the internal device.

|EEPROM| |
|----|------|
|eeprom.h|EEPROM Header|
|eeprom.c|EEPROM Driver|

## Millis

Provides a millisecond timer and delay function.

|Millis| |
|----|------|
|millis.h|Millis Header|
|millis.c|Millis Driver|

## Morse

Provides a morse keyer with support for Iambic A and B and Ultimatic.

|Morse| |
|----|------|
|morse.h|Morse Header|
|morse.c|Morse Driver|

## Rotary

Supports a rotary control with integral push button. Provides key debouncing.

|Rotary| |
|----|------|
|rotary.h|Rotary Header|
|rotary.c|Rotary Driver|

## Oscillator

Supports oscillator chips allowing the setting of individual output frequencies and enabling quadrature mode.

Currently only supports the Si5351A chip with 3 outputs. CLK0 and CLK1 use PLL A and CLK2 uses PLL B.

|Oscillator| |
|----|------|
|osc.h|Oscillator Header|
|si5351a.c|Si5351A Driver|

