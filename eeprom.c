/*
 * eeprom.c
 * 
 * Read and write to the AVR's internal EEPROM.
 * Based on sample code in the data sheet.
 * 
 * Generally we shouldn't call this code directly
 * but via the nvram code.
 *
 * Created: 07/08/2019
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

uint8_t eepromRead(uint16_t uiAddress)
{
    uint8_t eedr;
    
    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON) 
    {
        /* Wait for completion of previous write */
        while(EECR & (1<<EEPE));
    
        /* Set up address register */
        EEAR = uiAddress;
    
        /* Start eeprom read by writing EERE */
        EECR |= (1<<EERE);
    
        /* Return data from Data Register */
        eedr = EEDR;
    }

    return eedr;
}


void eepromWrite(uint16_t uiAddress, uint8_t ucData)
{
    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        /* Wait for completion of previous write */
        while(EECR & (1<<EEPE));
    
        /* Set up address and Data Registers */
        EEAR = uiAddress;
        EEDR = ucData;
    
        /* Write logical one to EEMPE */
        EECR |= (1<<EEMPE);
    
        /* Start eeprom write by setting EEPE */
        EECR |= (1<<EEPE);
    }
}
