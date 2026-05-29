#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// ================= UART =================
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / 16 / BAUD) - 1)

// ================= PIN =================
#define LED_POMPA PB7
#define LED_IDLE  PB6
#define BUZZER    PB5

#define SW1 PB4
#define SW3 PH5
#define SW4 PH4

uint8_t pump_status = 0;

// ========================================
// UART
// ========================================
void UART_init()
{
    UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
    UBRR0L = (unsigned char)UBRR_VALUE;

    UCSR0B = (1 << TXEN0);

    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00);
}

void UART_sendChar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));

    UDR0 = c;
}

void UART_sendString(const char *str)
{
    while (*str)
    {
        UART_sendChar(*str++);
    }
}

// ========================================
// BUZZER
// ========================================
void beep()
{
    PORTB |= (1 << BUZZER);

    _delay_ms(150);

    PORTB &= ~(1 << BUZZER);
}

// ========================================
// PUMP ON
// ========================================
void pumpON()
{
    pump_status = 1;

    PORTB |= (1 << LED_POMPA);
    PORTB &= ~(1 << LED_IDLE);

    PORTA = 0xFF;

    UART_sendString(
        "Kelembaban:Rendah,Pompa:ON\r\n"
    );

    beep();
}

// ========================================
// PUMP OFF
// ========================================
void pumpOFF()
{
    pump_status = 0;

    PORTB &= ~(1 << LED_POMPA);
    PORTB |= (1 << LED_IDLE);

    PORTA = 0x00;

    UART_sendString(
        "Kelembaban:Tinggi,Pompa:OFF\r\n"
    );

    beep();
}

// ========================================
// MAIN
// ========================================
int main(void)
{
    // OUTPUT
    DDRB |=
        (1 << LED_POMPA) |
        (1 << LED_IDLE) |
        (1 << BUZZER);

    DDRA = 0xFF;

    // INPUT
    DDRB &= ~(1 << SW1);

    DDRH &= ~(
        (1 << SW3) |
        (1 << SW4)
    );

    // INTERNAL PULLUP
    PORTB |= (1 << SW1);

    PORTH |=
        (1 << SW3) |
        (1 << SW4);

    UART_init();

    // Kondisi awal
    PORTB |= (1 << LED_IDLE);
    PORTB &= ~(1 << LED_POMPA);

    PORTA = 0x00;

    UART_sendString(
        "System Smart Farming Ready\r\n"
    );

    while (1)
    {
        // SW3
        if (!(PINH & (1 << SW3)) &&
            pump_status == 0)
        {
            pumpON();

            _delay_ms(300);
        }

        // SW4
        if (!(PINH & (1 << SW4)) &&
            pump_status == 1)
        {
            pumpOFF();

            _delay_ms(300);
        }

        // SW1
        if (!(PINB & (1 << SW1)))
        {
            if (pump_status == 0)
            {
                PORTB |=
                    (1 << LED_POMPA);

                PORTB &=
                    ~(1 << LED_IDLE);

                PORTA = 0xFF;

                pump_status = 1;

                UART_sendString(
                    "Manual:Pompa:ON\r\n"
                );
            }
            else
            {
                PORTB &=
                    ~(1 << LED_POMPA);

                PORTB |=
                    (1 << LED_IDLE);

                PORTA = 0x00;

                pump_status = 0;

                UART_sendString(
                    "Manual:Pompa:OFF\r\n"
                );
            }

            beep();

            _delay_ms(300);
        }
    }
}