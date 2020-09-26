#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "i2c.h"

#define I2C_START 0x08
#define I2C_START_RPT 0x10
#define I2C_SLA_W_ACK 0x18
#define I2C_SLA_R_ACK 0x40
#define I2C_DATA_ACK 0x28

#define I2C_TIMEOUT 0xFF

#define MAX_ITERATIONS 1000

static uint8_t i2cStart()
{
    int i;

    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

    for ( i = 0 ; (i < MAX_ITERATIONS) && !(TWCR & (1<<TWINT)) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return I2C_TIMEOUT;
    }
    else
    {
        return (TWSR & 0xF8);
    }
}

static void i2cStop()
{
    int i;

    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

    for ( i = 0 ; (i < MAX_ITERATIONS) && (TWCR & (1<<TWSTO)) ; i++);
}

static uint8_t i2cByteSend(uint8_t data)
{
    int i;

    TWDR = data;

    TWCR = (1<<TWINT) | (1<<TWEN);

    for ( i = 0 ; (i < MAX_ITERATIONS) && !(TWCR & (1<<TWINT)) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return I2C_TIMEOUT;
    }
    else
    {
        return (TWSR & 0xF8);
    }
}

static uint8_t i2cByteRead()
{
    int i;

    TWCR = (1<<TWINT) | (1<<TWEN);

    for ( i = 0 ; (i < MAX_ITERATIONS) && !(TWCR & (1<<TWINT)) ; i++);

    if( i == MAX_ITERATIONS )
    {
        return I2C_TIMEOUT;
    }
    else
    {
        return (TWDR);
    }
}

uint8_t i2cWriteRegister(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t stts;
    
    stts = i2cStart();
    if (stts != I2C_START) return 1;

    stts = i2cByteSend((addr<<1)|0);
    if (stts != I2C_SLA_W_ACK)
    {
        i2cStop();
        return 2;
    }

    stts = i2cByteSend(reg);
    if (stts != I2C_DATA_ACK) return 3;

    stts = i2cByteSend(data);
    if (stts != I2C_DATA_ACK) return 4;

    i2cStop();

    return 0;
}

uint8_t i2cReadRegister(uint8_t addr, uint8_t reg, uint8_t *data)
{
    uint8_t stts;
    
    stts = i2cStart();
    if (stts != I2C_START) return 1;

    stts = i2cByteSend((addr<<1)|0);
    if (stts != I2C_SLA_W_ACK)
    {
        i2cStop();
        return 2;
    }
 
    stts = i2cByteSend(reg);
    if (stts != I2C_DATA_ACK) return 3;

    stts = i2cStart();
    if (stts != I2C_START_RPT) return 4;

    stts = i2cByteSend((addr<<1)|1);
    if (stts != I2C_SLA_R_ACK)
    {
        i2cStop();
        return 5;
    }

    *data = i2cByteRead();

    i2cStop();

    return 0;
}

// Init TWI (I2C)
//
void i2cInit()
{
    TWBR = 2; // 800kbps
    TWSR = 0;
    TWDR = 0xFF;
    PRR = 0;
}
