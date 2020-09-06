/*
 * serial.c
 *
 * Created: 30/01/2020 19:21:04
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "config.h"

// Buffer to hold characters received from the serial port
volatile static uint8_t serialRXBuf[SERIAL_RX_BUF_LEN];

// Buffer to hold characters to send to the serial port
volatile static uint8_t serialTXBuf[SERIAL_TX_BUF_LEN];

// Current write and read positions in the buffer
// Buffer is empty when they are the same
volatile static uint8_t posRXWrite, posRXRead;
volatile static uint8_t posTXWrite, posTXRead;

// Receive complete interrupt handler
#ifdef USART0
ISR(USART0_RXC_vect)
#else
ISR (USART_RX_vect)
#endif
{
    // Get the serial character into the buffer
#ifdef USART0
    serialRXBuf[posRXWrite] = USART0.RXDATAL;
#else
    serialRXBuf[posRXWrite] = UDR0;
#endif

    // Move to the next write position wrapping if necessary
    posRXWrite = (posRXWrite + 1) % SERIAL_RX_BUF_LEN;
}

// Sends the next byte in the transmit buffer
static void sendData()
{
    /* Put data into transmit hardware buffer, sends the data */
#ifdef USART0
    USART0.TXDATAL = serialTXBuf[posTXRead];
#else
    UDR0 = serialTXBuf[posTXRead];
#endif

    // Move to the next read position wrapping if necessary
    posTXRead = (posTXRead + 1) % SERIAL_TX_BUF_LEN;
}

// Transmit data empty interrupt handler
#ifdef USART0
ISR(USART0_DRE_vect)
#else
ISR (USART_UDRE_vect)
#endif
{
    // If the read position is the same as the write position then nothing in
    // the buffer
    if( posTXRead == posTXWrite )
    {
        // Nothing in buffer so disable the interrupt
#ifdef USART0
        USART0.CTRLA &= ~USART_DREIE_bm;
#else
        UCSR0B &= ~(1<<UDRIE0);
#endif
    }
    else
    {
        sendData();
    }
}

void serialInit( uint32_t baud)
{
#ifdef USART0
    /*Set baud rate */
    uint16_t ubrr = 4*F_CPU/baud;
    USART0.BAUDH = (uint8_t)(ubrr>>8);
    USART0.BAUDL = (uint8_t)ubrr;

    /* Enable receive complete interrupts */
    USART0.CTRLA = USART_RXCIE_bm;
    
    /* Enable receiver and transmitter */
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

    /* Set frame format: 8 data, 1 stop bit */
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;

    // Enable TXD as output and RXD as input */
    SERIAL_DIR_REG |=  (1<<SERIAL_TXD_PIN);
    SERIAL_DIR_REG &= ~(1<<SERIAL_RXD_PIN);

    // Enable the alternative pins if required
#ifdef SERIAL_ALTERNATIVE_PINS
    PORTMUX.CTRLB |= PORTMUX_USART0_bm;
#endif

#else
    /*Set baud rate */
    uint16_t ubrr = F_CPU/16/baud-1;
    UBRR0H = (uint8_t)(ubrr>>8);
    UBRR0L = (uint8_t)ubrr;
 
    /* Enable receiver and transmitter and receive complete interrupts */
    UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
 
    /* Set frame format: 8 data, 1 stop bit */
    UCSR0C = (0<<USBS0)|(3<<UCSZ00);
#endif
}

void serialTransmit( uint8_t data )
{
    // Access the serial buffer but ensure not interrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        serialTXBuf[posTXWrite] = data;

        // Move to the next write position wrapping if necessary
        posTXWrite = (posTXWrite + 1) % SERIAL_TX_BUF_LEN;

        // Send the first byte if the transmit hardware buffer is empty
#ifdef USART0
        if( USART0.STATUS & USART_DREIF_bm )
        {
            sendData();

            // Enable the transmit data empty interrupt
            USART0.CTRLA |= USART_DREIE_bm;
        }
#else
        if( UCSR0A & (1<<UDRE0) )
        {
            sendData();

            // Enable the transmit data empty interrupt
            UCSR0B |= (1<<UDRIE0);
        }
#endif
    }
}

// Send a string ending with a NULL
void serialTXString( char *string )
{
    int i;
    for( i = 0 ; string[i] ; i++ )
    {
        serialTransmit( string[i] );
    }
}

uint8_t serialReceive( void )
{
    // If nothing in the buffer then return a NULL
    uint8_t data = 0;

    // Access the serial buffer but ensure not interrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        // If the read position is the same as the write position then nothing in
        // the buffer
        if( posRXRead != posRXWrite )
        {
            data = serialRXBuf[posRXRead];

            // Move to the next read position wrapping if necessary
            posRXRead = (posRXRead + 1) % SERIAL_RX_BUF_LEN;
        }
    }
    return data;
}
