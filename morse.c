/*
 * morse.c
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <string.h>
#include <stdio.h>

#include "config.h"
#include "main.h"
#include "io.h"
#include "millis.h"
#include "morse.h"

// The number of dot lengths making up the standard word "PARIS"
#define STANDARD_WORD_LEN 50

// The length of a dash
#define DASH_LEN 3

// The length of the gap between characters
#define CHARACTER_GAP_LEN 3

// The length of the gap between words
// We start counting after the character gap has been detected
#define WORD_GAP_LEN (7 - CHARACTER_GAP_LEN)

// Morse words per minute
// This should be changed through the morseSetWpm function as that
// also updates the dot length which is needed for the correct timing
// wpm==0 means straight key mode
static uint8_t morseWpm;

// The length of a morse dot
// Everything else is a multiple of this
static uint16_t dotLen;

// Keyer mode (Iambic, Ultimatic etc)
static enum eMorseKeyerMode keyerMode;

// State of the morse machine
static enum eMorseState
{
    idle,
    sendingDot,	// Currently sending a dot
    sendingDash,// Currently sending a dash
    firstGap,   // First part of inter-symbol gap
    secondGap,  // Second part of inter-symbol gap
    tune,       // Tuning i.e. transmit until press key
} morseState = idle;

// Set and get the keyer mode
void morseSetKeyerMode( enum eMorseKeyerMode newKeyerMode )
{
    keyerMode = newKeyerMode;
}

enum eMorseKeyerMode morseGetKeyerMode()
{
    return keyerMode;
}

// Convert a character to its text equivalent
//
// Each character is encoded in binary
// 0b10 is a dash
// 0b01 is a dot
static char* morseChar( unsigned int character )
{
    char* morse;
    
    switch( character )
    {
        case 0b0110:
            morse = "a";
            break;
            
        case 0b10010101:
            morse = "b";
            break;
        
        case 0b10011001:
            morse = "c";
            break;
        
        case 0b100101:
            morse = "d";
            break;
        
        case 0b01:
            morse = "e";
            break;
        
        case 0b01011001:
            morse = "f";
            break;
        
        case 0b101001:
            morse = "g";
            break;
        
        case 0b01010101:
            morse = "h";
            break;
        
        case 0b0101:
            morse = "i";
            break;
        
        case 0b01101010:
            morse = "j";
            break;
        
        case 0b100110:
            morse = "k";
            break;
        
        case 0b01100101:
            morse = "l";
            break;
        
        case 0b1010:
            morse = "m";
            break;
        
        case 0b1001:
            morse = "n";
            break;
        
        case 0b101010:
            morse = "o";
            break;
        
        case 0b01101001:
            morse = "p";
            break;
        
        case 0b10100110:
            morse = "q";
            break;
        
        case 0b011001:
            morse = "r";
            break;
        
        case 0b010101:
            morse = "s";
            break;
        
        case 0b10:
            morse = "t";
            break;
        
        case 0b010110:
            morse = "u";
            break;
        
        case 0b01010110:
            morse = "v";
            break;
        
        case 0b011010:
            morse = "w";
            break;
        
        case 0b10010110:
            morse = "x";
            break;
        
        case 0b10011010:
            morse = "y";
            break;
        
        case 0b10100101:
            morse = "z";
            break;
        
        case 0b0110101010:
            morse = "1";
            break;
        
        case 0b0101101010:
            morse = "2";
            break;
        
        case 0b0101011010:
            morse = "3";
            break;
        
        case 0b0101010110:
            morse = "4";
            break;
        
        case 0b0101010101:
            morse = "5";
            break;
        
        case 0b1001010101:
            morse = "6";
            break;
        
        case 0b1010010101:
            morse = "7";
            break;
        
        case 0b1010100101:
            morse = "8";
            break;
        
        case 0b1010101001:
            morse = "9";
            break;
        
        case 0b1010101010:
            morse = "0";
            break;
        
        case 0b010110100101:
            morse = "?";
            break;
        
        case 0b1001010110:
            morse = "=";
            break;
        
        case 0b011001100110:
            morse = ".";
            break;
        
        case 0b101001011010:
            morse = ",";
            break;
        
        case 0b0110011001:
            morse = "ar";
            break;
        
        case 0b010101100110:
            morse = "sk";
            break;
        
        case 0b1001101001:
            morse = "kn";
            break;
        
        default:
            morse = "|";
            break;
    }
    
    return morse;
}

// Current character state
// Will build up as dots and dashes sent
// Once character finished then can decode
static unsigned int charState = 0;

// How long until we next want to be called
// Default to being called immediately
int requiredDelay;

// The state of the paddle. Although both paddles can be pressed together
// we will always take note of one in preference to the other.
// Which one depends on the keyer mode
enum eMorsePaddle
{
    paddleNone,
    paddleDot,
    paddleDash
};

// The paddle currently being regarded as pressed
static enum eMorsePaddle currentPaddle;

// The first paddle pressed in this sequence
static enum eMorsePaddle firstPaddle;

// Deal with the dot paddle being pressed
static bool dotPaddlePressed( bool dotPressed )
{
    bool bPressed;
    
    if( dotPressed )
    {
        // If the dot paddle has been pressed we can start the dot
        morseState = sendingDot;

        // Key the transmitter
        keyDown( true );

        // Add the dot to the character state
        charState <<= 2;
        charState |= 1;

        // Wait for the correct length
        requiredDelay = dotLen;
        
        // Current paddle is dot
        currentPaddle = paddleDot;
        
        // If the beginning of a sequence then note the first paddle was dot
        if( firstPaddle == paddleNone )
        {
            firstPaddle = paddleDot;
        }
        
        bPressed = true;
    }
    else
    {
        bPressed = false;
    }
    return bPressed;
}

// Deal with the dash paddle being pressed
static bool dashPaddlePressed( bool dashPressed )
{
    bool bPressed;

    if( dashPressed )
    {
        // If the dash paddle has been pressed we can start the dash
        morseState = sendingDash;

        // Key the transmitter
        keyDown( true );

        // Add the dash to the character state
        charState <<= 2;
        charState |= 2;

        // Wait for the correct length
        requiredDelay = dotLen * DASH_LEN;
        
        // Current paddle is dash
        currentPaddle = paddleDash;
        
        // If the beginning of a sequence then note the first paddle was dash
        if( firstPaddle == paddleNone )
        {
            firstPaddle = paddleDash;
        }
        
        bPressed = true;
    }
    else
    {
        bPressed = false;
    }
    return bPressed;
}

// Scan the dot and dash paddles and implement the correct keying mode
// to key the transmitter 
// Returns true if active, false if idle
bool morseScanPaddles( void )
{
    // To keep track of the length of a gap so we know when a character or word finishes
    // Start with the max possible so it doesn't start yet
    static unsigned long charGapStartTime = ULONG_MAX;
    static unsigned long wordGapStartTime = ULONG_MAX;
    
    // Time of next scan
    static unsigned long nextScan;

    // The time when the second gap after a dot or dash ends
    static int secondGapTime;

    // Early read of paddle states
    static bool dotPressedEarly;
    static bool dashPressedEarly;
    
    // True if paddles have been squeezed i.e. both pressed at once
    static bool bSqueezed;
    
    // Default to being called immediately
    requiredDelay = 0;

    // Read current paddle states
    bool dotPressed = ioReadDotPaddle();
    bool dashPressed = ioReadDashPaddle();
    
    // Note if paddles are squeezed
    if( dotPressed && dashPressed )
    {
        bSqueezed = true;
    }

    // In straight key mode just use the dot connection
    // except dash will take us into tune mode
    if( morseWpm == 0 )
    {
        // Currently key up so see if key now down
        switch( morseState )
        {
            case tune:
                // Dot will take us out of tune mode
                if( dotPressed )
                {
                    morseSetTuneMode( false );
                }
                break;

            case idle:
                // In straight key mode dash puts us into tune mode
                if( dashPressed )
                {
                    morseSetTuneMode( true );
                }
                else if( dotPressed )
                {
                    keyDown( true );
                    morseState = sendingDot;
                }
                break;

            default:
                // Currently key down so see if key now up
                if( !dotPressed )
                {
                    keyDown( false );
                    morseState = idle;
                }
        }
    }
    else
    {
        // If between characters then note if any paddles are pressed now
        // This is needed since we may miss the paddle being pressed for the second
        // character if we only look at the paddles when the gap is over
        if( morseState == secondGap )
        {
            // If paddles are pressed at any point then note this
            if( dotPressed )
            {
                dotPressedEarly = true;
            }
            if( dashPressed )
            {
                dashPressedEarly = true;
            }
        }
    
        // See if we should scan for a paddle press
        unsigned long currentTime = millis();
        if( currentTime > nextScan )
        {
            switch( morseState )
            {
                case sendingDot:
                case sendingDash:
                    // If currently sending a dot or dash then now in a gap which is one dot long
                    // But we split this in case the paddle is pressed early
                    keyDown( false );
                    morseState = firstGap;
                    requiredDelay = dotLen / 3;

                    // Note when the second gap should end - this allows for delays in
                    // the keyDown function
                    secondGapTime = currentTime + dotLen;

                    // Note the time when the gap started
                    charGapStartTime = currentTime;

                    // Prevent word gap from starting just yet
                    wordGapStartTime = ULONG_MAX;
                    break;

                case firstGap:
                    // Wait the rest of the gap
                    // This means we will note the state early
                    morseState = secondGap;

                    // Calculate the remainder of the gap
                    requiredDelay = secondGapTime - currentTime;
                    break;

                case secondGap:
                    // The gap is over so can now go idle
                    morseState = idle;
                    requiredDelay = 0;
                    
                    // If we are in iambic B mode then may need to add an additional
                    // dot or dash
                    if( keyerMode == morseKeyerIambicB )
                    {
                        // Only add the dot or dash if the paddles have been released
                        if( !(dotPressed || dotPressedEarly || dashPressed || dashPressedEarly) )
                        {
                            // Only add the dot or dash if the paddles were previously squeezed
                            if( bSqueezed )
                            {
                                // Have to add the opposite character
                                if( currentPaddle == paddleDot )
                                {
                                    dashPressedEarly = true;
                                }
                                else
                                {
                                    dotPressedEarly = true;
                                }
                            }
                        }
                    }
                    
                    bSqueezed = false;
                    break;

                case idle:
                    // Not currently sending anything
                    // Has a paddle been pressed?
                    if( dotPressed || dotPressedEarly || dashPressed || dashPressedEarly )
                    {
                        // Which paddle we look at first depends on the keyer mode
                        bool bDotFirst = true;
                        switch( keyerMode )
                        {
                            case morseKeyerIambicA:
                            case morseKeyerIambicB:
                                // In Iambic mode always look at the opposite paddle to the one
                                // currently considered as pressed
                                if( currentPaddle == paddleDot )
                                {
                                    bDotFirst = false;
                                }
                                break;

                            case morseKeyerUltimatic:
                            default:
                                // In Ultimatic mode always look first at the opposite paddle
                                // to the one that started this sequence
                                if( firstPaddle == paddleDot )
                                {
                                    bDotFirst = false;
                                }
                                break;

                        }
                        
                        // Scan the paddles in the correct order
                        if( bDotFirst )
                        {
                            if( !dotPaddlePressed( dotPressed || dotPressedEarly ) )
                            {
                                dashPaddlePressed( dashPressed || dashPressedEarly );
                            }
                        }
                        else
                        {
                            if( !dashPaddlePressed( dashPressed || dashPressedEarly ) )
                            {
                                dotPaddlePressed( dotPressed || dotPressedEarly );
                            }
                        }
                        
                        // Clear dot and dash memories
                        dotPressedEarly = dashPressedEarly = false;
                    }
                    else
                    {
                        // Nothing pressed so reset the paddle state
                        firstPaddle = currentPaddle = paddleNone;
                        
                        // If the gap since the previous character is long enough then character is over
                        if( (charGapStartTime != ULONG_MAX) &&
                            (currentTime - charGapStartTime) >= (CHARACTER_GAP_LEN * dotLen) )
                        {
                            // Now the character is complete look it up
                            displayMorse(morseChar(charState));
                    
                            // Start the character again
                            charState = 0;
                    
                            // Don't display this character again
                            charGapStartTime = ULONG_MAX;

                            // Now the character has finished we can start timing the word gap
                            wordGapStartTime = currentTime;
                        }
                    }
                    break;
                default:
                    // Should never get here!
                    break;
            }
    
            // Time of the next scan
            nextScan = currentTime + requiredDelay;
        }

        // If the gap since the previous character is long enough then word is over
        if( (morseState == idle) &&
            (wordGapStartTime != ULONG_MAX) &&
            ((currentTime - wordGapStartTime) >= (WORD_GAP_LEN * dotLen)) )
        {
            displayMorse(" ");
                        
            // Don't display this gap character again
            wordGapStartTime = ULONG_MAX;
        }
    }

    // Return true if not idle
    return morseState != idle;
}

// Enter or exit tune mode
void morseSetTuneMode( bool bTune )
{
    keyDown( bTune );

    if( bTune )
    {
        morseState = tune;
    }
    else
    {
        morseState = idle;
    }
}

// Returns true if in tune mode
bool morseInTuneMode()
{
    return morseState == tune;
}

uint8_t morseGetWpm()
{
    return morseWpm;
}

void morseSetWpm( uint8_t wpm )
{
    morseWpm = wpm;

    // Calculate the length of a dot but only if not straight key mode (wpm==0)
    if( morseWpm > 0 )
    {
        dotLen = 60000L/STANDARD_WORD_LEN/morseWpm;
    }
}

void morseInit()
{
}

