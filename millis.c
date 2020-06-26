/* Counting Milliseconds with Timer1
 * ---------------------------------
 * For more information see
 * http://www.adnbr.co.uk/articles/counting-milliseconds
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
 
#include "config.h"

// Calculate the value needed for 
// the CTC match value in OCR1A.
// This is an 8 bit register so we have to divide the
// clock such that we don't overflow this.

#if F_CPU == 1000000
// 1MHz: clock divided by 4
#define CLOCK_DIV 4
#define CLOCK_SELECT  ((1 << CS11) | (1 << CS10))
#elif F_CPU == 8000000
// 8MHz: clock divided by 32
#define CLOCK_DIV 32
#define CLOCK_SELECT  ((1 << CS12) | (1 << CS11))
#endif

// Clock is running at fraction of CPU clock
#define CTC_MATCH_OVERFLOW (F_CPU / CLOCK_DIV / 1000)

volatile unsigned long timer1_ticks;
 
ISR (TIMER1_COMPA_vect)
{
    timer1_ticks++;
}

// Return the current millisecond tick count
unsigned long millis ()
{
    unsigned long ticks;

    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        ticks = timer1_ticks;
    }
    
    return ticks;
}

// Delay for a number of milliseconds
// This is a busy wait so use with care
void delay( uint16_t ms )
{
    volatile unsigned long endTime = millis() + ms;
    for( ; millis() < endTime ; );
}

// Initialise the timer
void setup_millis(void)
{
    // CTC mode
    TCCR1 = (1 << CTC1) | CLOCK_SELECT;
 
    // Load the high byte, then the low byte
    // into the output compare
    OCR1A = CTC_MATCH_OVERFLOW;
 
    // Enable the compare match interrupt
    TIMSK |= (1 << OCIE1A);
    
    // Now enable global interrupts
    sei();
}
