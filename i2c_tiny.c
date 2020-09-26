/*
 * i2c_tiny.c
 *
 * I2C driver for the ATtiny 1-series.
 * This should also work on the 0-series and 2-series and
 * probably the XMega too.
 *
 * Created: 03/09/2020
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#include "config.h"
#include "i2c.h"

#define MAX_ITERATIONS 1000

static uint8_t i2cStart(uint8_t address, bool bRead )
{
    int i;

    TWI0.MADDR = (address << 1) | bRead;

    for ( i = 0 ; (i < MAX_ITERATIONS) && !(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return false;
    }
    else
    {
        if (TWI0.MSTATUS & TWI_ARBLOST_bm)
        {
            return false;
        }

        return !(TWI0.MSTATUS & TWI_RXACK_bm);
    }
}

static void i2cStop()
{
//    int i;

    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
//	for ( i = 0 ; (i < MAX_ITERATIONS) && (TWCR & (1<<TWSTO)) ; i++);
}

static uint8_t i2cByteSend(uint8_t data)
{
    int i;

    for ( i = 0 ; ((i < MAX_ITERATIONS) && !(TWI0.MSTATUS & TWI_WIF_bm)) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return false;
    }
    else
    {
        TWI0.MDATA = data;
        TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;

        return !(TWI0.MSTATUS & TWI_RXACK_bm);
    }
}

static uint8_t i2cByteRead()
{
    int i;

    for ( i = 0 ; (i < MAX_ITERATIONS) && !(TWI0.MSTATUS & TWI_RIF_bm) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return 0;
    }
    else
    {
        uint8_t data = TWI0.MDATA;
        TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_RECVTRANS_gc;
        return data;
    }
}

uint8_t i2cWriteRegister(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t stts;
    
    stts = i2cStart(addr, false);
    if (!stts) return 1;

    stts = i2cByteSend(reg);
    if (!stts) return 3;

    stts = i2cByteSend(data);
    if (!stts) return 4;

    i2cStop();

    return 0;
}

uint8_t i2cReadRegister(uint8_t addr, uint8_t reg, uint8_t *data)
{
    uint8_t stts;
    
    stts = i2cStart(addr, false);
    if (!stts) return 1;

    stts = i2cByteSend(reg);
    if (!stts) return 3;

    stts = i2cStart(addr, true);
    if (!stts) return 4;

    *data = i2cByteRead();

    i2cStop();

    return 0;
}

// Init TWI (I2C)
//
void i2cInit()
{
    TWI0.MBAUD = ((F_CPU / I2C_CLOCK_RATE) - 10 ) / 2;
    TWI0.MCTRLA = TWI_ENABLE_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}
