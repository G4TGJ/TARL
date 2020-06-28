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

// Record the oscillator and PLL frequencies
static uint32_t pllFreq[NUM_SYNTH_PLL];
static uint32_t oscFreq[NUM_CLOCKS];

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

// Enable/disable the RX clock outputs
void oscRXClockEnable( bool bEnable )
{
	si5351aOutputEnable( SI_CLK_ENABLE_0 | SI_CLK_ENABLE_1, bEnable );
}

// Enable/disable the TX clock output
void oscTXClockEnable( bool bEnable )
{
    si5351aOutputEnable( SI_CLK_ENABLE_2, bEnable );
}

// Get the multisynth divider for the frequency
// These have been chosen for the maximum range to avoid
// glitches while tuning. None of the transitions happen in
// an amateur band.
// These are so that the VCO is in the range 600-900MHz.
// However, for quadrature output the maximum divider is 126 so we
// have to use lower VCO frequencies for some bands.
static uint32_t getMultisynthDivider( uint32_t frequency )
{
    uint32_t divider;

    if( frequency < 5000000 )
    {
        divider = 126;
    }
    else if( frequency < 7500000 )
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
    return divider;
}
#if 0
// Set CLK0 output ON and to the specified frequency
// Set CLK1 output to the same but 90 degrees out of phase
// for quadrature reception
// Can set either CW normal or CW reverse
//
// This does not enable the outputs - call oscRXClockEnable( true ) to do this
//
// Frequency is in the range 1MHz to 150MHz
//
void oscSetRXFrequency(uint32_t frequency, bool bCWReverse )
{
	uint32_t divider;

    // We will reset the PLLs only when the divider or the CW mode changes
    static uint32_t prevDivider = 0;
    static bool prevCWReverse;

/*
Cool! Three rules for silky-smooth click-free tuning like on the QCX transceiver http://qrp-labs.com/qcx :

1) Fix the MultiSynth divider with an even integer, and vary the VCO with the fractional divider in its feedback loop.
   Anyway you have to do this, to get the 90-degree quadrature LO thing working.

2) Only set the PLL Reset register bit when you set the MultiSynth divider. Which normally means only once at start-up
  (unless you are designing it for a a wide-tuning or multi-band radio).

3) Don't be tempted to switch off outputs while making a tuning adjustment... this is just a nasty fudge to try to tackle
   audible tuning clicks, and is not necessary as long as you are doing 1) and 2).
*/

    // Get the predetermined multisynth divider for the frequency
	divider = getMultisynthDivider( frequency );

	// Set up PLL A with the calculated divider and ultimate frequency
	setupPLL(SYNTH_PLL_A, divider, frequency);

    // If the divider has changed then set everything up
    // This will usually only happen at power up
    // but will also happen if the frequency changes enough
    // Also need to do this if the CW mode changes as have to
    // pick up the phase change
    if( (divider != prevDivider) || (bCWReverse != prevCWReverse) )
    {
	    // Set up all the multiSynth dividers, with the calculated divider. 
	    // The final R division stage can divide by a power of two, from 1..128. 
	    // represented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
	    // If you want to output frequencies below 1MHz, you have to use the 
	    // final R division stage
	    setupMultisynth(SI_SYNTH_MS_0, divider, 0, 1, SI_R_DIV_1);
	    setupMultisynth(SI_SYNTH_MS_1, divider, 0, 1, SI_R_DIV_1);

        // Delay is needed as sometimes the phase difference is other than
        // 90 degrees without it
        delay(2);

        // Set the CW mode
        // This is done by setting CLK1 to be 90 degrees out of phase with CLK0
        // The register we use depends on the CW mode
        if( bCWReverse )
        {
            i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_PHOFF, 0);
            i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK1_PHOFF, divider);
        }
        else
        {
            i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_PHOFF, divider);
            i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK1_PHOFF, 0);
        }

	    // Switch on the CLK0 and CLK1 outputs
	    // and set the MultiSynth0 input to be PLL A
	    i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_CONTROL, 0x4C | SI_CLK_SRC_PLL_A);
	    i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK1_CONTROL, 0x4C | SI_CLK_SRC_PLL_A);

		// Reset the PLL. This causes a glitch in the output. For small changes to 
		// the parameters, you don't need to reset the PLL, and there is no glitch
	    i2cSendRegister(SI5351A_I2C_ADDRESS, SI_PLL_RESET, SI_PLL_RESET_A);	

        prevDivider = divider;
        prevCWReverse = bCWReverse;
    }
}

// Set CLK2 to the TX frequency
//
// This does not enable the output - call oscTXClockEnable( true ) to do this
//
// Frequency is in the range 1MHz to 150MHz
//
void oscSetTXFrequency(uint32_t frequency)
{
	uint32_t divider;

	// We will reset the PLLs only when the divider changes
	static uint32_t prevDivider = 0;

    // Get the predetermined multisynth divider for the frequency
    divider = getMultisynthDivider( frequency );

	// Set up PLL B 
	setupPLL(SYNTH_PLL_B, divider, frequency);

	// If the divider has changed then set everything up
	// This will usually only happen at power up
	// but will also happen if the frequency changes enough
	if( divider != prevDivider )
	{
		// Set up the multiSynth divider, with the calculated divider.
		// The final R division stage can divide by a power of two, from 1..128.
		// represented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
		// If you want to output frequencies below 1MHz, you have to use the
		// final R division stage
		setupMultisynth(SI_SYNTH_MS_2, divider, 0, 1, SI_R_DIV_1);

        // Delay sometimes needed for it to take changes
        delay(2);

		// Switch on CLK2 for the TX frequency which is derived from PLL B
		i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK2_CONTROL, 0x4F | SI_CLK_SRC_PLL_B);

		// Reset the PLLs. This causes a glitch in the output. For small changes to
		// the parameters, you don't need to reset the PLL, and there is no glitch
		i2cSendRegister(SI5351A_I2C_ADDRESS, SI_PLL_RESET, SI_PLL_RESET_B);

		prevDivider = divider;
	}
}
#endif
// Calculate the divider (a+b/c) for a given oscillator frequency and PLL frequency
static void calcDivider( uint32_t frequency, uint32_t pllFreq, uint32_t *pa, uint32_t *pb, uint32_t *pc )
{
    // Intermediate calculations
    uint32_t d, r;

    // Firstly work out the divider a and the remainder
    *pa = pllFreq / frequency;
    r = pllFreq % frequency;

    // b/c == r/frequency but we can't use these directly since
    // c can only be up to 1048575 so have to scale for this
    // We will scale by d to achieve this
    // (1000000/48575 is about 21)
    if( frequency < 21000000 )
    {
        d = 21;
    }
    else
    {
        d = frequency / 1000000;
    }

    *pb = r / d;
    *pc = frequency / d;
}

void oscSetFrequency( uint8_t clock, uint32_t frequency )
{
    // To get the output frequency the PLL is divided by a+b/c
	uint32_t a, b, c;

    // The register, clock and reset bit for the PLL for this oscillator
    uint8_t pll, pll_clock, pll_reset;

	// We will reset the PLLs only when the divider changes
	static uint32_t prevDivider[NUM_CLOCKS];

    // Keep track of each clock's frequency
    oscFreq[clock] = frequency;

	// Get the predetermined multisynth divider for the frequency
    // Except that clock 1 uses the same divider as clock 0
    // because they share a PLL
    if( clock == 1 )
    {
        // Clock 1 is using PLL A which has been set up for clock 0
        // We need to calculate our own divider (a+b/c)
        calcDivider( frequency, pllFreq[SYNTH_PLL_A], &a, &b, &c );
    }
    else
    {
        // Clocks 0 and 2 set the PLL up with a predetermined integer divider
    	a = getMultisynthDivider( frequency );
        b = 0;
        c = 1;
    }

    // Choose the PLL.
    // Clocks 0 and 1 use PLL A
    // Clock 2 uses PLL B
    if( clock == 2 )
    {
        pll = SYNTH_PLL_B;
        pll_reset = SI_PLL_RESET_B;
        pll_clock = SI_CLK_SRC_PLL_B;
    }
    else
    {
        pll = SYNTH_PLL_A;
        pll_reset = SI_PLL_RESET_A;
        pll_clock = SI_CLK_SRC_PLL_A;
    }

	// Set up the PLL
    // Clock 1 uses the same PLL as clock 0
    if( clock != 1 )
    {
	    setupPLL(pll, a, frequency);
    }

    // Set up the multiSynth divider, with the calculated divider.
    // The final R division stage can divide by a power of two, from 1..128.
    // represented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
    // If you want to output frequencies below 1MHz, you have to use the
    // final R division stage
    setupMultisynth(SI_SYNTH_MS_0+(8*clock), a, b, c, SI_R_DIV_1);

    // Delay needed for it to take changes
    delay(5);

    // Switch on the clock
    i2cSendRegister(SI5351A_I2C_ADDRESS, SI_CLK0_CONTROL+clock, 0x4F | pll_clock);

    // If we are setting clock 0 then we need to also set the multisynth divider for
    // clock 1 because it also uses PLL A
    if( clock == 0 )
    {
        uint32_t a1, b1, c1;
        calcDivider( oscFreq[1], pllFreq[SYNTH_PLL_A], &a1, &b1, &c1 );

        setupMultisynth(SI_SYNTH_MS_1, a1, b1, c1, SI_R_DIV_1);

        delay(5);
    }

	// If the divider has changed then set everything up
	// This will usually only happen at power up
	// but will also happen if the frequency changes enough
    // Never reset the PLL for clock 1 as it uses clock 0's PLL
	if( (clock != 1) && (a != prevDivider[clock]) )
	{
    	// Reset the PLLs. This causes a glitch in the output. For small changes to
    	// the parameters, you don't need to reset the PLL, and there is no glitch
    	i2cSendRegister(SI5351A_I2C_ADDRESS, SI_PLL_RESET, pll_reset);

    	prevDivider[clock] = a;
	}
}


// Set the crystal frequency. This is initialised from NVRAM but can be
// changed from the menu.
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
    
    // Load the crystal frequency from NVRAM
    oscSetXtalFrequency( nvramReadXtalFreq() );

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

