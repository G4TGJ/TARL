/*
 * cat.c
 *
 * Created: 31/01/2020 14:09:22
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "main.h"
#include "millis.h"
#include "serial.h"
#include "display.h"
#include "nvram.h"

// CAT Commands
#define CAT_BUF_LEN 50
#define CAT_READ_LEN 3
#define CAT_PARAM_POS 2

// Characters representing the operating modes that are supported
#define CW_REVERSE_TEXT '7'
#define CW_TEXT         '3'

// Return the operating mode character
static char getModeChar()
{
    return nvramReadCWReverse() ? CW_REVERSE_TEXT : CW_TEXT;
}

#define CMD_FREQUENCY_VFO_A ('F' + ('A'<<8))
#define CMD_FREQUENCY_VFO_A_LEN 11

// Handle CAT Frequency VFO-A command
static bool catFrequencyA( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the frequency into the reply
        sprintf( cmdText, "FA%08lu;", getVFOFreq( VFO_A ) );

        // Send the answer
        serialTXString( cmdText );
        //displayText( MENU_LINE, cmdText, false );
        bSuccess = true;
    }
    else if( len == CMD_FREQUENCY_VFO_A_LEN )
    {
        // Null terminate the frequency and read it from the command param
        cmdText[CMD_FREQUENCY_VFO_A_LEN-1] = '\0';
        char *p;
        uint32_t freq = strtoul( &cmdText[CAT_PARAM_POS], &p, 10 );

        setVFOFrequency( VFO_A, freq );
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_FREQUENCY_VFO_B ('F' + ('B'<<8))
#define CMD_FREQUENCY_VFO_B_LEN 11

// Handle CAT Frequency VFO-B command
static bool catFrequencyB( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the frequency into the reply
        sprintf( cmdText, "FB%08lu;", getVFOFreq( VFO_B ) );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_FREQUENCY_VFO_B_LEN )
    {
        // Null terminate the frequency and read it from the command param
        cmdText[CMD_FREQUENCY_VFO_B_LEN-1] = '\0';
        char *p;
        uint32_t freq = strtoul( &cmdText[CAT_PARAM_POS], &p, 10 );

        setVFOFrequency( VFO_B, freq );
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_INFORMATION ('I' + ('F'<<8))

// Handle CAT Information command
static bool catInformation( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "IF000%08lu%c%04u%u%u%c00000;", getCurrentVFOFreq(),
                                                         ((getCurrentVFOOffset() >= 0) ? '+' : '-'),
                                                         ((getCurrentVFOOffset() >= 0) ? getCurrentVFOOffset() : -getCurrentVFOOffset()),
                                                         getCurrentVFORIT(),
                                                         getCurrentVFOXIT(),
                                                         getModeChar()
                                                          );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_OPPOSITE_INFORMATION ('O' + ('I'<<8))

// Handle CAT Opposite Band Information command
static bool catOppositeInformation( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "IF000%08lu%c%04u%u%u%c00000;", getOtherVFOFreq(),
                                                         ((getOtherVFOOffset() >= 0) ? '+' : '-'),
                                                         ((getOtherVFOOffset() >= 0) ? getOtherVFOOffset() : -getOtherVFOOffset()),
                                                         getOtherVFORIT(),
                                                         getOtherVFOXIT(),
                                                         getModeChar()
                                                          );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_VFO_SELECT ('V' + ('S'<<8))
#define CMD_VFO_SELECT_LEN 4

#define CAT_VFO_POS 2

// Handle CAT Information command
static bool catVFOSelect( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "VS%d;", getCurrentVFO() );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_VFO_SELECT_LEN )
    {
        // Set the VFO from the string
        setCurrentVFO( cmdText[CAT_VFO_POS] - '0');
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_OPERATING_MODE ('M' + ('D'<<8))
#define CMD_OPERATING_MODE_LEN 5

#define CAT_MODE_POS 3

// Handle CAT Operating Mode command
static bool catOperatingMode( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "MD0%c;", getModeChar() );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_OPERATING_MODE_LEN )
    {
        // Get the operating mode from the string
        char opMode = cmdText[CAT_MODE_POS];

        // Get the current CW mode
        char bReverseCW = nvramReadCWReverse();

        // Set the new mode if valid
        switch( opMode )
        {
            case CW_TEXT:
                if( bReverseCW )
                {
                    // Want normal mode but currently in reverse so change it
                    setCWReverse( false );
                }
                break;
            case CW_REVERSE_TEXT:
                if( !bReverseCW )
                {
                    // Want reverse mode but currently normal so change it
                    setCWReverse( true );
                }
                break;
            default:
                // Anything else we ignore
                break;
        }
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_KEY_PITCH ('K' + ('P'<<8))
#define CMD_KEY_PITCH_LEN 4

// Handle CAT Key Pitch command
static bool catKeyPitch( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        // The pitch is fixed by the hardware
        sprintf( cmdText, "KP08;" );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_KEY_PITCH_LEN )
    {
        // Cannot change the pitch as it is fixed by the hardware
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_FUNCTION_TX ('F' + ('T'<<8))
#define CMD_FUNCTION_TX_LEN 4
#define CMD_FUNCTION_TX_VFO_POS 2

// Handle CAT Function TX command
// This reports which VFO we will transmit on
static bool catFunctionTX( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, getVFOSplit() ^ getCurrentVFO() ? "FT1;" : "FT0;" );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_FUNCTION_TX_LEN )
    {
        // If this command specifies the other VFO then we go into split mode
        if( cmdText[CMD_FUNCTION_TX_VFO_POS] == '0' )
        {
            // Command says transmit on VFO A
            if( getCurrentVFO() == VFO_B )
            {
                // Actually on VFO B so go to split
                setVFOSplit( true );
            }
            else
            {
                setVFOSplit( false );
            }
        }
        else
        {
            // Command says transmit on VFO B
            if( getCurrentVFO() == VFO_A )
            {
                // Actually on VFO A so go to split
                setVFOSplit( true );
            }
            else
            {
                setVFOSplit( false );
            }
        }
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_CLARIFIER ('R' + ('T'<<8))
#define CMD_CLARIFIER_LEN 4
#define CMD_CLARIFIER_POS 2

// Handle CAT Clarifier command
// This turns RIT on or off
static bool catClarifier( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "RT%d;", getCurrentVFORIT() );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_CLARIFIER_LEN )
    {
        switch( cmdText[CMD_FUNCTION_TX_VFO_POS] )
        {
            case '0':
                setCurrentVFORIT( false );
                bSuccess = true;
                break;
            case '1':
                setCurrentVFORIT( true );
                bSuccess = true;
                break;
            default:
                break;
        }
    }
    return bSuccess;
}

#define CMD_TX_SET ('T' + ('X'<<8))
#define CMD_TX_SET_LEN 4

// Handle CAT TX Set command
static bool catTXSet( char *cmdText, int len )
{
    bool bSuccess = false;

    // Read or set command?
    if( len == CAT_READ_LEN )
    {
        // Write the information into the reply
        sprintf( cmdText, "TX%c;", getTransmitting() ? '2' : '0' );

        // Send the answer
        serialTXString( cmdText );

        bSuccess = true;
    }
    else if( len == CMD_TX_SET_LEN )
    {
        // Don't allow CAT to transmit
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_SWAP_VFO ('S' + ('V'<<8))

// Handle CAT Swap VFO command
static bool catSwapVFO( char *cmdText, int len )
{
    bool bSuccess = false;

    // Only a set command
    if( len == CAT_READ_LEN )
    {
        // Swap the VFO
        vfoSwap();

        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_VFO_TO_VFO ('V' + ('V'<<8))

// Handle CAT VFO to VFO command
static bool catVFOToVFO( char *cmdText, int len )
{
    bool bSuccess = false;

    // Only a set command
    if( len == CAT_READ_LEN )
    {
        // Set the VFOs equal
        vfoEqual();

        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_CLAR_CLEAR ('R' + ('C'<<8))

// Handle CAT Clar Clear command
static bool catClarClear( char *cmdText, int len )
{
    bool bSuccess = false;

    // Only a set command
    if( len == CAT_READ_LEN )
    {
        // Set RIT to 0
        setCurrentVFOOffset(0);
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_CLAR_PLUS_OFFSET ('R' + ('U'<<8))
#define CMD_CLAR_PLUS_OFFSET_LEN 7

// Handle CAT Clarifier Plus Offset command
static bool catClarPlusOffset( char *cmdText, int len )
{
    bool bSuccess = false;

    // Set command only
    if( len == CMD_CLAR_PLUS_OFFSET_LEN )
    {
        // Null terminate the offset and read it from the command param
        cmdText[CMD_CLAR_PLUS_OFFSET_LEN] = '\0';
        setCurrentVFOOffset( atoi( &cmdText[CAT_PARAM_POS-1] ) );
        bSuccess = true;
    }
    return bSuccess;
}

#define CMD_CLAR_MINUS_OFFSET ('R' + ('D'<<8))
#define CMD_CLAR_MINUS_OFFSET_LEN 7

// Handle CAT Clarifier Minus Offset command
static bool catClarMinusOffset( char *cmdText, int len )
{
    bool bSuccess = false;

    // Set command only
    if( len == CMD_CLAR_MINUS_OFFSET_LEN )
    {
        // Null terminate the offset and read it from the command param
        cmdText[CMD_CLAR_MINUS_OFFSET_LEN-1] = '\0';
        setCurrentVFOOffset( -atoi( &cmdText[CAT_PARAM_POS] ) );
        bSuccess = true;
    }
    return bSuccess;
}

// Process a CAT command received over the serial bus
static void catProcess( char *cmdText, int len )
{
    bool bSuccess = false;

    // Null terminate the string
    cmdText[len] = '\0';
    //displayText( MENU_LINE, cmdText, false );
    
    // Commands must be at least 3 characters long
    if( len >= 3 )
    {
        // Form the command into 16 bits
        uint16_t cmd = cmdText[0] + (cmdText[1]<<8);

        // Find the command
        switch( cmd )
        {
            case CMD_FREQUENCY_VFO_A:
                bSuccess = catFrequencyA( cmdText, len );
                break;
            case CMD_FREQUENCY_VFO_B:
                bSuccess = catFrequencyB( cmdText, len );
                break;
            case CMD_INFORMATION:
                bSuccess = catInformation( cmdText, len );
                break;
            case CMD_OPPOSITE_INFORMATION:
                bSuccess = catOppositeInformation( cmdText, len );
                break;
            case CMD_VFO_SELECT:
                bSuccess = catVFOSelect( cmdText, len );
                break;
            case CMD_KEY_PITCH:
                bSuccess = catKeyPitch( cmdText, len );
                break;
            case CMD_FUNCTION_TX:
                bSuccess = catFunctionTX( cmdText, len );
                break;
            case CMD_TX_SET:
                bSuccess = catTXSet( cmdText, len );
                break;
            case CMD_SWAP_VFO:
                bSuccess = catSwapVFO( cmdText, len );
                break;
            case CMD_VFO_TO_VFO:
                bSuccess = catVFOToVFO( cmdText, len );
                break;
            case CMD_OPERATING_MODE:
                bSuccess = catOperatingMode( cmdText, len );
                break;
            case CMD_CLARIFIER:
                bSuccess = catClarifier( cmdText, len );
                break;
            case CMD_CLAR_CLEAR:
                bSuccess = catClarClear( cmdText, len );
                break;
            case CMD_CLAR_PLUS_OFFSET:
                bSuccess = catClarPlusOffset( cmdText, len );
                break;
            case CMD_CLAR_MINUS_OFFSET:
                bSuccess = catClarMinusOffset( cmdText, len );
                break;
        }
    }

    // If the command wasn't successfully handled then reply with the error string
    if( !bSuccess )
    {
        serialTXString( "?;" );
        //displayText( MENU_LINE, cmdText, false );
    }
}

// Listen for CAT control commands
void catControl()
{
    // Character read from serial port
    uint8_t c;

    // Command buffer to hold the incoming characters forming a command
    static char cmdBuf[CAT_BUF_LEN];

    // Current position in the buffer
    static int bufPos = 0;

    // Read a character from the serial device. Zero means no character is in the buffer.
    if( (c = serialReceive()) != 0 )
    {
        // Store each character in a buffer
        cmdBuf[ bufPos ] = c;

        // Move to next position
        bufPos++;

        // If the buffer is full, or the latest character is a semicolon,
        // it is the end of the command
        if( (c == ';') || (bufPos >= (CAT_BUF_LEN-1)) )
        {
            // Process the command
            catProcess( cmdBuf, bufPos );

            // Start building next command
            bufPos = 0;
        }
    }
}

// Initialise CAT control
void catInit()
{
    // Initialise the serial port
    serialInit( SERIAL_BAUD );
}
