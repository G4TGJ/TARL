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
#ifdef OCR1AH

// 16 bit timer
#if F_CPU == 1000000
// 1MHz: clock divided by 1
#define CLOCK_DIV 1
#define CLOCK_SELECT  ((1 << CS10))
#elif F_CPU == 8000000
// 8MHz: clock divided by 8
#define CLOCK_DIV 8
#define CLOCK_SELECT  ((1 << CS11))
#elif F_CPU == 16000000
// 16MHz: clock divided by 8
#define CLOCK_DIV 8
#define CLOCK_SELECT  ((1 << CS11))
#endif

#else
// 8 bit timer
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
#endif

// For the ATtiny 1-series CLOCK_DIV should be defined in config.h
#if defined TCA0
#if CLOCK_DIV == 1
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV1_gc
#elif CLOCK_DIV == 2
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV2_gc
#elif CLOCK_DIV == 4
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV4_gc
#elif CLOCK_DIV == 8
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV8_gc
#elif CLOCK_DIV == 16
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV16_gc
#elif CLOCK_DIV == 64
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV64_gc
#elif CLOCK_DIV == 256
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV256_gc
#elif CLOCK_DIV == 1024
#define CLOCK_SELECT TCA_SINGLE_CLKSEL_DIV1024_gc
#endif
#endif

// Clock is running at fraction of CPU clock
// Calculate the number of ticks for one millisecond
#define CTC_MATCH_OVERFLOW (F_CPU / CLOCK_DIV / 1000)

volatile uint32_t timer1_ticks;
 
#if defined TCA0
ISR(TCA0_OVF_vect)
{
    timer1_ticks++;

    /* The interrupt flag has to be cleared manually */
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}
#else
#ifdef TIMER1_COMPA_vect
ISR (TIMER1_COMPA_vect)
#elif defined TIM1_COMPA_vect 
ISR (TIM1_COMPA_vect)
#endif
{
    timer1_ticks++;
}
#endif

// Return the current millisecond tick count
uint32_t millis ()
{
    uint32_t ticks;

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
    volatile uint32_t endTime = millis() + ms;
    for( ; millis() < endTime ; );
}

// Initialise the timer
void millisInit(void)
{
#ifdef OCR1AH
    // 16 bit timer
    // CTC mode, Clock/8
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | CLOCK_SELECT;
    
    // Load the high byte, then the low byte
    // into the output compare
    OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
    OCR1AL = (CTC_MATCH_OVERFLOW & 0xFF);

    // Enable the compare match interrupt
    TIMSK1 |= (1 << OCIE1A);
#elif defined OCR1A
    // 8 bit timer
    // CTC mode
    TCCR1 = (1 << CTC1) | CLOCK_SELECT;
 
    // Load the high byte, then the low byte
    // into the output compare
    OCR1A = CTC_MATCH_OVERFLOW;

    // Enable the compare match interrupt
    TIMSK |= (1 << OCIE1A);
#elif defined TCA0
    TCA0.SINGLE.INTCTRL = 0 << TCA_SINGLE_CMP0_bp /* Compare 0 Interrupt: disabled */
                        | 0 << TCA_SINGLE_CMP1_bp /* Compare 1 Interrupt: disabled */
                        | 0 << TCA_SINGLE_CMP2_bp /* Compare 2 Interrupt: disabled */
                        | 1 << TCA_SINGLE_OVF_bp; /* Overflow Interrupt: enabled */

    TCA0.SINGLE.PER = CTC_MATCH_OVERFLOW;

    TCA0.SINGLE.CTRLA = CLOCK_SELECT 
                        | 1 << TCA_SINGLE_ENABLE_bp /* Module Enable: enabled */;

#else
    #error No timer support for this chip
#endif
    
    // Now enable global interrupts
    sei();
}
