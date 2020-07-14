/** \file eeprom.h
 *
 * \date 07/08/2019
 * \author Richard Tomlinson G4TGJ
 */ 
 

#ifndef EEPROM_H
#define EEPROM_H

#include <inttypes.h>

/// Initialise the EEPROM driver.
void eepromInit();

/// Read a byte from the EEPROM.
///
/// @param[in] uiAddress EEPROM address
/// @return The byte read from the address
uint8_t eepromRead(uint16_t uiAddress);

/// Write a byte to the EEPROM.
///
/// @param[in] uiAddress EEPROM address
/// @param[in] ucData Data to write
void eepromWrite(uint16_t uiAddress, uint8_t ucData);

#endif //EEPROM_H
