/*
 * eeprom.h
 *
 * Created: 07/08/2019
 * Author : Richard Tomlinson G4TGJ
 */ 
 

#ifndef EEPROM_H
#define EEPROM_H

#include <inttypes.h>

void eepromInit();
uint8_t eepromRead(uint16_t uiAddress);
void eepromWrite(uint16_t uiAddress, uint8_t ucData);

#endif //EEPROM_H
