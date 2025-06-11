#include <avr/io.h>
#include <avr/interrupt.h>
#include "timers.h"

volatile uint8_t ISR_count = 0;
volatile uint8_t servo_flag = 1;//flag I use for resetting the servo position

void timer1_init(void){ //timer for the green LED
    TCCR1A |= ((1<<WGM11) | (1<<WGM10));
    TCCR1B |= ((1<<WGM12) | (1<<WGM13));
    OCR1A = 2500; //value paired with prescalar to get correct period
    TCCR1B |= ((1<<CS11) | (1<<CS10)); //prescalar of 64
    TIMSK1 |= ((1<<OCIE1B) | (1<<TOIE1));
    OCR1B = 1250; //midway between 0 and 2500

}

void timer2_init(void){
    TCCR2A |= (0b11 << WGM20);  // Fast PWM mode, modulus = 256
    TCCR2A |= (0b10 << COM2A0); // Turn D11 on at 0x00 and off at OCR2A
    OCR2A = 23;        // Initial pulse width 
    TCCR2B |= (0b111 << CS20);  // Prescaler = 1024 for 16ms period
}



ISR(TIMER1_OVF_vect)
    {
        // Turn the PWM bit on
        PORTC |= (1<<3);
    }

ISR(TIMER1_COMPB_vect)
    {
        // Turn the PWM bit off
        PORTC &= ~(1<<3);
    }

ISR(TIMER2_OVF_vect)
    {
        if (servo_flag){
            OCR2A = 35; //reset to starting position 
            servo_flag = 0;
        }

        if (OCR2A != 12){
            ISR_count++;
            if (ISR_count==27){
                OCR2A--;
                ISR_count = 0;
            }
            
        }
    }