/*****************************************************************************
 *
 * Atmel Corporation
 *
 * File              : USI_TWI_Master.c
 * Date              : $Date: 2016-7-15 $
 * Updated by        : $Author: Atmel $
 *
 * Support mail      : avr@atmel.com
 *
 * Supported devices : All device with USI module can be used.
 *                     The example is written for the ATmega169, ATtiny26 and ATtiny2313
 *
 * AppNote           : AVR310 - Using the USI module as a TWI Master
 *
 * Description       : This is an implementation of an TWI master using
 *                     the USI module as basis. The implementation assumes the AVR to
 *                     be the only TWI master in the system and can therefore not be
 *                     used in a multi-master system.
 * Usage             : Initialize the USI module by calling the USI_TWI_Master_Initialise()
 *                     function. Hence messages/data are transceived on the bus using
 *                     the USI_TWI_Transceive() function. The transceive function
 *                     returns a status byte, which can be used to evaluate the
 *                     success of the transmission.
 *
 ****************************************************************************/
#if __GNUC__
#include <avr/io.h>
#else
#include <inavr.h>
#include <ioavr.h>
#endif
#include "config.h"
#include "USI_TWI_Master.h"
#include "i2c.h"

 // set USI to shift 8 bits i.e. count 16 clock edges.
#define USISR_8bit ((1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC) | (0x0 << USICNT0))

 // set USI to shift 1 bit i.e. count 2 clock edges.
#define USISR_1bit ((1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC) | (0xE << USICNT0))


/*---------------------------------------------------------------
 USI TWI single master initialization function
---------------------------------------------------------------*/
static void USI_TWI_Master_Initialise(void)
{
	PORT_USI |= (1 << PIN_USI_SDA); // Enable pullup on SDA, to set high as released state.
	PORT_USI |= (1 << PIN_USI_SCL); // Enable pullup on SCL, to set high as released state.

	DDR_USI |= (1 << PIN_USI_SCL); // Enable SCL as output.
	DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

	USIDR = 0xFF;                                           // Preload dataregister with "released level" data.
	USICR = (0 << USISIE) | (0 << USIOIE) |                 // Disable Interrupts.
	        (1 << USIWM1) | (0 << USIWM0) |                 // Set USI in Two-wire mode.
	        (1 << USICS1) | (0 << USICS0) | (1 << USICLK) | // Software stobe as counter clock source
	        (0 << USITC);
	USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC) | // Clear flags,
	        (0x0 << USICNT0);                                             // and reset counter.
}

/*---------------------------------------------------------------
 Core function for shifting data in and out from the USI.
 Data to be sent has to be placed into the USIDR prior to calling
 this function. Data read, will be return'ed from the function.
---------------------------------------------------------------*/
static uint8_t USI_TWI_Master_Transfer(uint8_t data)
{
	USISR = data;                                          // Set USISR according to temp.
	                                                       // Prepare clocking.
	data = (0 << USISIE) | (0 << USIOIE) |                 // Interrupts disabled
	       (1 << USIWM1) | (0 << USIWM0) |                 // Set USI in Two-wire mode.
	       (1 << USICS1) | (0 << USICS0) | (1 << USICLK) | // Software clock strobe as source.
	       (1 << USITC);                                   // Toggle Clock Port.
	do
    {
		DELAY_T2TWI;

        // Generate positve SCL edge.
		USICR = data;

		// Wait for SCL to go high.
		while (!(PIN_USI & (1 << PIN_USI_SCL)));

		DELAY_T4TWI;

        // Generate negative SCL edge.
		USICR = data;
	} while (!(USISR & (1 << USIOIF))); // Check for transfer complete.

	DELAY_T2TWI;

    // Read out data.
	data = USIDR;

    // Release SDA.
	USIDR = 0xFF;

     // Enable SDA as output.
	DDR_USI |= (1 << PIN_USI_SDA);

     // Return the data from the USIDR
	return data;
}

static void USI_TWI_Start()
{
	// Release SCL to ensure that (repeated) Start can be performed
	PORT_USI |= (1 << PIN_USI_SCL);

    // Verify that SCL becomes high.
	while (!(PIN_USI & (1 << PIN_USI_SCL)));

	DELAY_T2TWI;

	// Generate Start Condition Force SDA LOW.
	PORT_USI &= ~(1 << PIN_USI_SDA);

	DELAY_T4TWI;

    // Pull SCL LOW.
	PORT_USI &= ~(1 << PIN_USI_SCL);

    // Release SDA.
	PORT_USI |= (1 << PIN_USI_SDA);
}

static void USI_TWI_Read( uint8_t *pData)
{
    // Enable SDA as input.
    DDR_USI &= ~(1 << PIN_USI_SDA);
    *pData = USI_TWI_Master_Transfer(USISR_8bit);

    // Load NACK to confirm End Of Transmission.
    USIDR = 0xFF;

    // Generate ACK/NACK.
    USI_TWI_Master_Transfer(USISR_1bit);
}

static void USI_TWI_Write( uint8_t data)
{
    // Pull SCL LOW.
	PORT_USI &= ~(1 << PIN_USI_SCL);

    // Setup data.
	USIDR = data;

    // Send 8 bits on bus.
	USI_TWI_Master_Transfer(USISR_8bit);

	/* Clock and verify (N)ACK from slave */
	DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
	USI_TWI_Master_Transfer(USISR_1bit);
}


/*---------------------------------------------------------------
 Function for generating a TWI Stop Condition. Used to release
 the TWI bus.
---------------------------------------------------------------*/
static uint8_t USI_TWI_Master_Stop(void)
{
    // Pull SDA low.
	PORT_USI &= ~(1 << PIN_USI_SDA);

    // Release SCL.
	PORT_USI |= (1 << PIN_USI_SCL);

	// Wait for SCL to go high.
	while (!(PIN_USI & (1 << PIN_USI_SCL)));

	DELAY_T4TWI;

     // Release SDA.
	PORT_USI |= (1 << PIN_USI_SDA);

	DELAY_T2TWI;

	return (true);
}

void i2cInit()
{
    USI_TWI_Master_Initialise();
}

uint8_t i2cSendRegister(uint8_t addr, uint8_t reg, uint8_t data)
{
    USI_TWI_Start();
    USI_TWI_Write( (addr << TWI_ADR_BITS) | (0 << TWI_READ_BIT) );
    USI_TWI_Write( reg );
    USI_TWI_Write( data );
    USI_TWI_Master_Stop(); // Send a STOP condition on the TWI bus.

    return 0;
}

uint8_t i2cReadRegister(uint8_t addr, uint8_t reg, uint8_t *data)
{
    USI_TWI_Start();
    USI_TWI_Write( (addr << TWI_ADR_BITS) | (0 << TWI_READ_BIT) );
    USI_TWI_Write( reg );
    USI_TWI_Start();
    USI_TWI_Write( (addr << TWI_ADR_BITS) | (1 << TWI_READ_BIT) );
    USI_TWI_Read( data );
    USI_TWI_Master_Stop(); // Send a STOP condition on the TWI bus.

    return 0;
}
