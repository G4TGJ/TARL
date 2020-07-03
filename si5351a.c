// Original software:
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
//
// A very very simple Si5351a demonstration
// using the Si5351a module kit http://www.hanssummers.com/synth
// Please also refer to SiLabs AN619 which describes all the registers to use
//
// Modified by Richard Tomlinson G4TGJ 2018/2019
// Eliminated float to just use 32 bit integers.
//
#include <inttypes.h>
#include <stdio.h>

#include "config.h"
#include "i2c.h"
#include "osc.h"
#include "nvram.h"
#include "millis.h"

// Register definitions
#define SI_DEVICE_STATUS 0

#define SI_CLK_ENABLE    3
#define SI_CLK_ENABLE_0  0x01
#define SI_CLK_ENABLE_1  0x02
#define SI_CLK_ENABLE_2  0x04

#define SI_CLK0_CONTROL	16
#define SI_CLK1_CONTROL	17
#define SI_CLK2_CONTROL	18
#define SI_SYNTH_PLL_A	26
#define SI_SYNTH_PLL_B	34
#define SI_SYNTH_MS_0	42
#define SI_SYNTH_MS_1	50
#define SI_SYNTH_MS_2	58
#define SI_CLK0_PHOFF   165
#define SI_CLK1_PHOFF   166

#define SI_PLL_RESET	177
#define SI_PLL_RESET_A	0x20
#define SI_PLL_RESET_B	0x80

#define SI_XTAL_LOAD    183
#define SI_XTAL_LOAD_DEFAULT    0b010010
#define SI_XTAL_LOAD_6PF         ((1<<6)|SI_XTAL_LOAD_DEFAULT)
#define SI_XTAL_LOAD_8PF         ((2<<6)|SI_XTAL_LOAD_DEFAULT)
#define SI_XTAL_LOAD_10PF        ((3<<6)|SI_XTAL_LOAD_DEFAULT)

// SYS_INIT bit in SI_DEVICE_STATUS register
#define SYS_INIT 0x80

#define SI_R_DIV_1		0b00000000			// R-division ratio definitions
#define SI_R_DIV_2		0b00010000
#define SI_R_DIV_4		0b00100000
#define SI_R_DIV_8		0b00110000
#define SI_R_DIV_16		0b01000000
#define SI_R_DIV_32		0b01010000
#define SI_R_DIV_64		0b01100000
#define SI_R_DIV_128	0b01110000

#define SI_CLK_SRC_PLL_A	0b00000000
#define SI_CLK_SRC_PLL_B	0b00100000

// The number of clocks on the chip
#define NUM_CLOCKS 3

// Maximum number of times to poll for the system init bit clearing
#define MAX_INIT_TRIES 10000

// Enum for the two PLLs along with a mapping to the actual registers
enum eSynthPLL
{
    SYNTH_PLL_A,
    SYNTH_PLL_B,
    NUM_SYNTH_PLL
};
const uint8_t synthPLL[NUM_SYNTH_PLL] = { SI_SYNTH_PLL_A, SI_SYNTH_PLL_B };

// Record the clock and PLL frequencies
static uint32_t pllFreq[NUM_SYNTH_PLL];
static uint32_t clockFreq[NUM_CLOCKS];

// The crystal frequency which is initialised from NVRAM
static uint32_t xtalFreq;

//
// Set up specified PLL with the specified divider and frequency
//
static void setupPLL(uint8_t pll, uint32_t divider, uint32_t frequency)
{
	// a, b and c as defined in AN619
	uint32_t a, b, c;

	// PLL config registers
	uint32_t P1;
	uint32_t P2;
	uint32_t P3;

    // We are only going to send bytes that have changed to minimise noise
    // from the I2C bus.
    #define NUM_PLL_BYTES 8
    static uint8_t prevPll[NUM_SYNTH_PLL][NUM_PLL_BYTES];
    uint8_t newPll[NUM_SYNTH_PLL][NUM_PLL_BYTES];

    // Ensure PLL is within range
    if( pll < NUM_SYNTH_PLL )
    {
	    // We will set the denominator as the crystal frequency divided by 27 as we
        // want it to be about a million so it is as large as possible for greatest resolution.
        // (The maximum denominator is 1048575.)
	    // This sets a maximum crystal of over 28MHz (crystal should be 25MHz or 27MHz)
	    // This allows us to use 32 bit integers.
	    // The error in the resulting frequency will be less than 1Hz
	    #define DENOM_RATIO 27
	    c = xtalFreq / DENOM_RATIO;
	    
	    // Calculate the pllFrequency: the divider * desired output frequency
	    pllFreq[pll] = divider * frequency;

	    // Determine the multiplier to get to the required pllFrequency
	    // Integer part is easy
	    a = pllFreq[pll] / xtalFreq;
	    
	    // Work out the fractional part (b/c)
	    // c is the denominator set above
	    // Can easily get b because we set c as a fraction of xtalFreq
	    // b = (pllFreq % xtalFreq) * c / xtalFreq
	    // but c is xtalFreq/27 so we get:
	    b = (pllFreq[pll] % xtalFreq) / DENOM_RATIO;

	    // Calculate the values as defined in AN619
	    uint32_t p = 128 * b / c;
	    P1 = 128 * a + p - 512;
	    P2 = 128 * b - c * p;
	    P3 = c;
	    
	    // Work out the new register values
	    newPll[pll][0] = (P3 & 0x0000FF00) >> 8;
	    newPll[pll][1] = (P3 & 0x000000FF);
	    newPll[pll][2] = (P1 & 0x00030000) >> 16;
	    newPll[pll][3] = (P1 & 0x0000FF00) >> 8;
	    newPll[pll][4] = (P1 & 0x000000FF);
	    newPll[pll][5] = ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16);
	    newPll[pll][6] = (P2 & 0x0000FF00) >> 8;
	    newPll[pll][7] = (P2 & 0x000000FF);

        // Write those registers that have changed
        for( int i = 0 ; i < NUM_PLL_BYTES ; i++ )
        {
            // Write changed registers and always register 7. It appears that writing
            // this last register latches in the new values.
            if( i == 7 || (newPll[pll][i] != prevPll[pll][i]) )
            {
                i2cSendRegister(SI5351A_I2C_ADDRESS, synthPLL[pll] + i, newPll[pll][i]);
                prevPll[pll][i] = newPll[pll][i];
            }
        }
    }
}

//
// Set up MultiSynth with divider a+b/c and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it is a #define in si5351a.h
// 
//
static void setupMultisynth(uint8_t synth, uint32_t a, uint32_t b, uint32_t c, uint8_t rDiv)
{
	uint32_t P1;					// Synth config register P1
	uint32_t P2;					// Synth config register P2
	uint32_t P3;					// Synth config register P3
    uint8_t  Div4 = 0;              // Divide by 4 bits

	// Calculate the values as defined in AN619
	uint32_t p = 128 * b / c;
	P1 = 128 * a + p - 512;
	P2 = 128 * b - c * p;
	P3 = c;

    // If the divider is 4 then special bits to set
    if( a == 4 )
    {
        Div4 = 0x0c;
    }

	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 0,   (P3 & 0x0000FF00) >> 8);
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 1,   (P3 & 0x000000FF));
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv | Div4 );
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 3,   (P1 & 0x0000FF00) >> 8);
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 4,   (P1 & 0x000000FF));
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 6,   (P2 & 0x0000FF00) >> 8);
	i2cSendRegister(SI5351A_I2C_ADDRESS, synth + 7,   (P2 & 0x000000FF));
}

//
// Switches off Si5351a output
// Example: si5351aOutputOff(SI_CLK0_CONTROL);
// will switch off output CLK0
//
static void si5351aOutputOff(uint8_t clk)
{
	i2cSendRegister(SI5351A_I2C_ADDRESS, clk, 0x80);		// Refer to SiLabs AN619 to see bit values - 0x80 turns off the output stage
}

// Enable/disable the clock output
//
// clk The clock bit (or bits)
// bEnable true to enable and false to disable
static void si5351aOutputEnable( uint8_t clk, bool bEnable )
{
    uint8_t reg;

	// Read the existing register
	if( i2cReadRegister(SI5351A_I2C_ADDRESS, SI_CLK_ENABLE, &reg) == 0 )
	{
        if( bEnable )
        {
            // Enable by clearing the bit
            reg &= ~clk;
        }
        else
        {
            // Disable by setting the bit
            reg |= clk;
        }
    	i2cSendRegister( SI5351A_I2C_ADDRESS, SI_CLK_ENABLE, reg );
	}
}

// Enable/disable a clock output
void oscClockEnable( uint8_t clock, bool bEnable )
{
    if( clock < NUM_CLOCKS )
    {
    	si5351aOutputEnable( SI_CLK_ENABLE_0 << clock, bEnable );
    }
}

// Get the multisynth divider for the frequency
// These have been chosen for the maximum range to avoid
// glitches while tuning. None of the transitions happen in
// an amateur band.
// These are so that the VCO is in the range 600-900MHz.
static uint32_t getMultisynthDivider( uint32_t frequency, bool bQuadrature )
{
    uint32_t divider = 0;

    // For quadrature output the maximum divider is 126 so we
    // have to use lower VCO frequencies for some bands.
    // This will limit the possible frequency range.
    if( bQuadrature )
    {
        if( frequency < 5000000 )
        {
            divider = 126;
        }
    }
    else
    {
        // If not requiring quadrature then can use any divider up to 900
        if( frequency < 800000 )
        {
            divider = 900;
        }
        else if( frequency < 1200000 )
        {
            divider = 750;
        }
        else if( frequency < 1700000 )
        {
            divider = 528;
        }
        else if( frequency < 2500000 )
        {
            divider = 360;
        }
        else if( frequency < 3400000 )
        {
            divider = 264;
        }
        else if( frequency < 5000000 )
        {
            divider = 180;
        }
    }

    if( divider == 0 )
    {
        if( frequency < 7500000 )
        {
            divider = 120;
        }
        else if( frequency < 10000000 )
        {
            divider = 86;
        }
        else if( frequency < 15000000 )
        {
            divider = 60;
        }
        else if( frequency < 20000000 )
        {
            divider = 40;
        }
        else if( frequency < 30000000 )
        {
            divider = 30;
        }
        else if( frequency < 45000000 )
        {
            divider = 20;
        }
        else if( frequency < 64000000 )
        {
            divider = 14;
        }
        else if( frequency < 90000000 )
        {
            divider = 10;
        }
        else if( frequency < 110000000 )
        {
            divider = 8;
        }
        else if( frequency < 150000000 )
        {
            divider = 6;
        }
        else
        {
            divider = 4;
        }
    }
    return divider;
}

// Calculate the divider (a+b/c) for a given clock frequency and PLL frequency
static void calcDivider( uint32_t clockFreq, uint32_t pllFreq, uint32_t *pa, uint32_t *pb, uint32_t *pc )
{
    // Intermediate calculations
    uint32_t d, r;

    // Firstly work out the divider a and the remainder
    *pa = pllFreq / clockFreq;
    r = pllFreq % clockFreq;

    // b/c == r/frequency but we can't use these directly since
    // c can only be up to 1048575 so have to scale for this
    // We will scale by d to achieve this
    // (1000000/48575 is about 21)
    if( clockFreq < 21000000 )
    {
        d = 21;
    }
    else
    {
        d = clockFreq / 1000000;
    }

    *pb = r / d;
    *pc = clockFreq / d;

    // Maximum possible divider is 900
    if( *pa >= 900 )
    {
        *pa = 900;
        *pb = 0;
        *pc = 1;
    }

    // Below 8 only 4 or 6 are legal
    else if( *pa < 8 )
    {
        if( *pa == 7 )
        {
            *pa = 8;
        }
        else if( *pa == 5 )
        {
            *pa = 6;
        }
        else
        {
            *pa = 4;
        }
        *pb = 0;
        *pc = 1;
    }
}

// Get the R Div for the frequency. Only used for low frequencies
// below 1MHz. With this extra divider have to change the clock
// frequency too.
static uint8_t getRDiv( uint32_t *pFreq )
{
    uint8_t rDiv;

    if( *pFreq < 16000 )
    {
        // R Divide by 128
        rDiv = SI_R_DIV_128;

        // So need to *128 the clock frequency
        *pFreq *= 128;
    }
    else if( *pFreq < 32000 )
    {
        // R Divide by 64
        rDiv = SI_R_DIV_64;

        // So need to *64 the clock frequency
        *pFreq *= 64;
    }
    else if( *pFreq < 64000 )
    {
        // R Divide by 32
        rDiv = SI_R_DIV_32;

        // So need to *32 the clock frequency
        *pFreq *= 32;
    }
    else if( *pFreq < 125000 )
    {
        // R Divide by 16
        rDiv = SI_R_DIV_16;

        // So need to *16 the clock frequency
        *pFreq *= 16;
    }
    else if( *pFreq < 250000 )
    {
        // R Divide by 8
        rDiv = SI_R_DIV_8;

        // So need to *8 the clock frequency
        *pFreq *= 8;
    }
    else if( *pFreq < 500000 )
    {
        // R Divide by 4
        rDiv = SI_R_DIV_4;

        // So need to *4 the clock frequency
        *pFreq *= 4;
    }
    else if( *pFreq < 1000000 )
    {
        // R Divide by 2
        rDiv = SI_R_DIV_2;

        // So need to double the clock frequency
        *pFreq *= 2;
    }
    else
    {
        rDiv = SI_R_DIV_1;
    }
    return rDiv;
}

// Set the clock to the given frequency with optional quadrature.
//
// quadrature is only used for clock 1 - it is ignored for the others
// +ve is CLK0 leads CLK1 by 90 degrees
// -ve is CLK0 lags  CLK1 by 90 degrees
// 0 is no quadrature i.e. set the frequency as normal
// When quadrature is set for clock 1 then it is set to the same frequency as clock 0
void oscSetFrequency( uint8_t clock, uint32_t frequency, int8_t q )
{
    // Whether quadrature has been enabled
    static int8_t quadrature;

    // To get the output frequency the PLL is divided by a+b/c
	uint32_t a, b, c;

    // Clocks 0 and 1 share a PLL so need a divider for the other clock
    uint32_t a1, b1, c1;

    // The PLL clock and reset bits for this clock
    uint8_t pll_clock, pll_reset;

    // The first (or only) clock we are setting
    uint8_t firstClock;

	// We will reset the PLLs only when the divider or quadrature changes
	static uint32_t prevDivider[NUM_CLOCKS];
    static int8_t prevQuadrature;

    static uint8_t rDiv[NUM_CLOCKS];

    if( clock < NUM_CLOCKS )
    {
        // Lower frequencies need an extra R Divider
        // in which case we have to increase the actual clock frequency
        rDiv[clock] = getRDiv( &frequency );

        // Keep track of each clock's frequency
        clockFreq[clock] = frequency;

        // For clock 1 we note the quadrature setting - this can also affect clock 0 because
        // we are limited in the dividers we can use in quadrature
        if( clock == 1 )
        {
            quadrature = q;

            // If the quadrature has changed then we set the previous divider to zero to 
            // force the PLL to be reset
            if( quadrature != prevQuadrature )
            {
                prevDivider[clock] = 0;
                prevQuadrature = quadrature;
            }
        }

	    // Get the predetermined multisynth divider for the frequency
        if( clock == 2 )
        {
    	    a = getMultisynthDivider( frequency, false );
    	    b = 0;
    	    c = 1;
            pll_reset = SI_PLL_RESET_B;
            pll_clock = SI_CLK_SRC_PLL_B;
            firstClock = 2;

	        // Set up the PLL
	        setupPLL(SYNTH_PLL_B, a, frequency);
        }
        else
        {
            // In quadrature set clock 1 frequency to the same as clock 0
            if( quadrature )
            {
                clockFreq[1] = clockFreq[0];
                rDiv[1] = rDiv[0];
            }

            // Clocks 0 and 1 share PLL A so we set the divider based
            // on the higher clock frequency.
            // We will always set clock 0 first
            firstClock = 0;
            if( clockFreq[0] >= clockFreq[1] )
            {
                // Clock 0 is the higher frequency so get its integer divider
    	        a = getMultisynthDivider( clockFreq[0], quadrature != 0 );
    	        b = 0;
    	        c = 1;

	            // Set up the PLL
	            setupPLL(SYNTH_PLL_A, a, clockFreq[0]);

                // Work out the required divider for clock 1
                if( quadrature )
                {
                    // In quadrature clock 1 is the same frequency as clock 0
                    a1 = a;
                    b1 = b;
                    c1 = c;
                }
                else
                {
                    calcDivider( clockFreq[1], pllFreq[SYNTH_PLL_A], &a1, &b1, &c1 );
                }
            }
            else
            {
                // Clock 1 is the higher frequency so get its integer divider
                // In quadrature mode won't get here as the oscillator frequencies are equal
    	        a1 = getMultisynthDivider( clockFreq[1], false );
    	        b1 = 0;
    	        c1 = 1;

	            // Set up the PLL
	            setupPLL(SYNTH_PLL_A, a1, clockFreq[1]);

                // Work out the required divider for clock 0
                calcDivider( clockFreq[0], pllFreq[SYNTH_PLL_A], &a, &b, &c );
            }
            pll_reset = SI_PLL_RESET_A;
            pll_clock = SI_CLK_SRC_PLL_A;
        }

        // Set up the multiSynth divider, with the calculated divider.
        // The final R division stage can divide by a power of two, from 1..128.
        // represented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
        // If you want to output frequencies below 1MHz, you have to use the
        // final R division stage
        setupMultisynth(SI_SYNTH_MS_0+(8*firstClock), a, b, c, rDiv[firstClock]);

        // Delay needed for it to take changes
        delay(5);

        // Set quadrature mode if applicable (only for clock 0 or clock 1)
        if( (clock != 2) && (quadrature != 0) )
        {
            if( quadrature > 0)
            {
                i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_PHOFF, 0);
                i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK1_PHOFF, a);
            }
            else
            {
                i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_PHOFF, a);
                i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK1_PHOFF, 0);
            }
        }

        // Switch on the clock
        i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_CONTROL+clock, 0x4F | pll_clock);

        // If we are setting clock 0 then we need to also set the multisynth divider for
        // clock 1 because it also uses PLL A
        if( firstClock == 0 )
        {
            setupMultisynth(SI_SYNTH_MS_1, a1, b1, c1, rDiv[1]);

            delay(5);
        }

	    // If the divider has changed then set everything up
	    // This will usually only happen at power up
	    // but will also happen if the frequency changes enough
	    if( a != prevDivider[clock] )
	    {
    	    // Reset the PLLs. This causes a glitch in the output. For small changes to
    	    // the parameters, you don't need to reset the PLL, and there is no glitch
    	    i2cSendRegister(SI5351A_I2C_ADDRESS, SI_PLL_RESET, pll_reset);

    	    prevDivider[clock] = a;
	    }
    }
}


// Set the crystal frequency.
void oscSetXtalFrequency( uint32_t xtal_freq )
{
    xtalFreq = xtal_freq;
}

// Initialise the si5351a chip
// Returns true if successful
// Returns false if unable to talk to it or it doesn't initialise properly
bool oscInit( void )
{
    int i;
    uint8_t regVal;
    
    // We talk to the chip over I2C
    i2cInit();

    // Wait for the device to be ready
    for( i = 0 ; i < MAX_INIT_TRIES ; i++ )
    {
        // Poll the device status register until the system init bit clears
        if( (i2cReadRegister( SI5351A_I2C_ADDRESS, SI_DEVICE_STATUS, &regVal ) == 0) &&
            !(regVal & SYS_INIT) )
        {
            break;
        }
    }

    // See if we have timed out
    if( i == MAX_INIT_TRIES )
    {
        return false;
    }
    else
    {
        // Chip is present and initialised
        //
        // Start by turning everything off as described in
        // figure 12 of the data sheet.
        // They will be enabled when the frequency is set.
        //
        // Disable all the outputs
        si5351aOutputEnable( SI_CLK_ENABLE_0 | SI_CLK_ENABLE_1 | SI_CLK_ENABLE_2, false );

        // Power down all the output drivers
	    si5351aOutputOff(SI_CLK0_CONTROL);
	    si5351aOutputOff(SI_CLK1_CONTROL);
	    si5351aOutputOff(SI_CLK2_CONTROL);

        // Set the crystal load capacitance
        i2cSendRegister( SI5351A_I2C_ADDRESS, SI_XTAL_LOAD, SI_XTAL_LOAD_CAP );

        return true;
    }
}

