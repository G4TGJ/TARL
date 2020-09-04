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
#include <avr/interrupt.h>
#include <util/atomic.h>

uint8_t eepromRead(uint16_t uiAddress)
{
#ifdef NVMCTRL
    // Check that the address is valid
    if( uiAddress < EEPROM_SIZE )
    {
        return *(uint8_t*)(EEPROM_START + uiAddress);
    }
    else
    {
        return 0xFF;
    }
#else
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
#endif
}


void eepromWrite(uint16_t uiAddress, uint8_t ucData)
{
#ifdef NVMCTRL
    // Check that the address is valid
    if( uiAddress < EEPROM_SIZE )
    {
        // First wait for the NVRAM controller to be free
        while(NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);

        // Load in the relevant EEPROM page by writing to the address
        *(uint8_t*)(EEPROM_START + uiAddress) = ucData;

        // Unlock self programming and then erase/write the page 
        // - should only erase the one byte
        CCP = CCP_SPM_gc;
        NVMCTRL.CTRLA = NVMCTRL_CMD_PAGEERASEWRITE_gc;
    }

#else
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
#endif
}
