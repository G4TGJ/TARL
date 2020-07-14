/** \file cat.h
 *
 * \author Richard Tomlinson G4TGJ
 */ 


#ifndef I2C_H
#define I2C_H

#include <inttypes.h>

/// Initialise the I2C driver.
///
/// Must be called before any other I2C functions.
void i2cInit();

/// Write to an 8 bit register over I2C.
///
/// @param[in] addr I2C address
/// @param[in] reg Register address
/// @param[in] data Data to write to register
uint8_t i2cWriteRegister(uint8_t addr, uint8_t reg, uint8_t data);

/// Read from an 8 bit register over I2C.
///
/// @param[in] addr I2C address
/// @param[in] reg Register address
/// @param[out] data Pointer to data location to write contents of register to
uint8_t i2cReadRegister(uint8_t addr, uint8_t reg, uint8_t *data);

#endif //I2C_H
