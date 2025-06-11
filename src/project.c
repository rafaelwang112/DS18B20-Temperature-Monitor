#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdio.h> 

#include "lcd.h"
#include "ds18b20.h"
#include "encoder.h"
#include "project.h"
#include "timers.h"
#include "serial.h"

const int BTN1 = 5;
const int BTN2 = 5;
const int RIGHT = 0;
uint8_t state_BTN1;
uint8_t state_BTN1_prev;

int16_t output, F_whole, F_decimal;
int32_t output_4bytes, C_factored, F_factored;

volatile uint8_t BTN_change;
volatile uint8_t SEND_change;
volatile uint8_t BTN2_change;
volatile uint8_t state_BTN2;
uint8_t state_BTN2_prev;
uint8_t rx_val_flag;


uint8_t transmission_state = 0;

void debounce_PB(int); //I use a debounce for PIN B and C
void debounce_PC(int);
void LED_servo_control (int8_t, int8_t); //controlling LED and servo as function for two temp settings (remote vs local)


int main(void)
{

    // Initialize the LCD
    lcd_init();

    // Initialize the ds18b20
    ds_init();

    //turning on pullup on PB5 for button
    PORTB |= (1<<BTN1);

    //turning on pullup on PC5 for second button
    PORTC |= (1<<BTN2);

    //enabling pin change interrupt
    PCICR|= ((1<<PCIE0) | (1<<PCIE1));
    PCMSK0 |= (1<<PCINT5);
    PCMSK1 |= ((1<<PCINT8) | (1<<PCINT13));

    //properly initialize the encoder
    encoder_init();

    timer1_init(); //initialize the timer used for green LED
    timer2_init(); //initialize the timer for the servo

    //enabling global interrupts
    sei();

    // Write splash screen and delay for 1 second
    lcd_moveto(0,2); //make name centered
    lcd_stringout("Rafael Wang");
    lcd_moveto(1,1); //center cursor on second row
    lcd_stringout("EE109 Project");
    _delay_ms(1000);
    lcd_writecommand(1); //to clear screen

    uint8_t toggle = 0; //toggling button state var.
    state_BTN1 = LO; //initial state
    state_BTN1_prev = HI; //keep track of prev state, initialized to HI
    BTN_change = 0; //if button has been pressed
    unsigned char temperature[2]; //array to hold the temp.

    DDRC |= ((1<<2) | (1<<3) | (1<<4)); //making the LED pins output
    DDRB |= (1<<3); //setting servo PWM output

    output = 0;
    F_whole = 75; //initializing F_whole to a common temp on the sensor for first loop before ds_temp finishes conversion
    F_decimal = 0; 
    output_4bytes = 0;
    C_factored = 0;
    F_factored = 0;

    DDRB |= (1<<4); //making enable of tri state gate a 0
    PORTB &= ~(1<<4);
    serial_init(); //initialize the serial

    char transmission_string [11]; //holding the <xxyy> transmission string
    BTN2_change = 0;
    SEND_change = 0;
    uint8_t togglebtn2 = 0; //toggling button state var.
    state_BTN2 = local;
    state_BTN2_prev = remote; //keep track of prev state, initialized to remote
    rx_val_flag = 0;


    display_initial_lim(); //displaying initial thresholds
    ds_convert(); //initial temp. conversion

    while (1) {                 // Loop forever
        if (ds_temp(temperature)==1){
            output = (((temperature[1])<<8) | temperature[0]); //first combining into 16 bit number 
            output_4bytes = output; //to perform multplication by 10000 16 bits is not enough, requires 32
            C_factored = (output_4bytes * 10000)/16; //degrees in Celsius but factored up by 10,000
            F_factored = (C_factored*9)/5+320000; //degrees in F but factored up by 10,000
            F_whole = F_factored/10000; //getting whole number part of temp
            F_decimal = F_factored % 10000; //getting fractional part
            if (F_decimal<0){ //ensuring decimal parts is not seen as "negative"
                F_decimal = -F_decimal;
            }
            int8_t F_rounded = (F_decimal+500)/1000; //rounding to a tenth
            if (F_rounded==10){ //checking in case F_decimal is 9500 or higher
                F_whole++;
                F_rounded = 0;
        }
        char buf[8];
        snprintf(buf, 8, "%3d.%d", F_whole, F_rounded); //formatting and printing to LCD
        lcd_moveto(0,11);
        lcd_stringout(buf);
        ds_convert();
    }
        if (BTN_change==1 && state_BTN2 == local){
            debounce_PB(BTN1);
            BTN_change = 0;
            toggle ^= 0x01;
            if (toggle == 0){
                state_BTN1 = LO;
                lcd_moveto(0,5);
                lcd_writedata(' ');
            }
            else{
                state_BTN1 = HI;
                lcd_moveto(0,0);
                lcd_writedata(' ');
            }
        }
        if ((state_BTN1 == LO && state_BTN1_prev != LO) && state_BTN2 == local){
            lcd_moveto(0,0);
            lcd_writedata('>');
            state_BTN1_prev = state_BTN1;
        }
        else if ((state_BTN1 == HI && state_BTN1_prev != HI) && state_BTN2 == local){
            lcd_moveto(0,5);
            lcd_writedata('>');
            state_BTN1_prev = state_BTN1;
        }

        display_limits(); //display encoder limits

        if (state_BTN2==local){ //choosing which temp values to use to control the LED and servo
            LED_servo_control(low, high);
        }
        else{
            LED_servo_control (rx_low, rx_high);
        }

        if (SEND_change ==1 ){
            debounce_PC(RIGHT);
            SEND_change = 0;
            transmission_state = 1;   
        }
        if (transmission_state){
            snprintf(transmission_string, 11, "<%02d%02d>", low, high); //transmitting data
            for (int i = 0; i<6;i++){
                char_tx(transmission_string[i]);
            }
            transmission_state = 0;
        }

        if (valid_string == 1){
            valid_string = 0;
            display_rx(); //display remote limits
            rx_val_flag = 1;
        }

        if (BTN2_change==1){
            debounce_PC(BTN2);
            BTN2_change = 0;
            if (rx_val_flag==1){ //so if the button 2 is pressed before any data has been received nothing happens
                togglebtn2 ^= 0x01;
                if (togglebtn2 == 0){
                    state_BTN2 = local;
                    lcd_moveto(1,10);
                    lcd_writedata(' ');

                }
                else{
                    state_BTN2 = remote;
                    lcd_moveto(0,10);
                    lcd_writedata(' ');
            }

            }
        }
        if (state_BTN2 == local &&  state_BTN2_prev != local){
            lcd_moveto(0,10);
            lcd_writedata('<');
            state_BTN2_prev = state_BTN2;
        }
        else if (state_BTN2 == remote && state_BTN2_prev != remote){
            lcd_moveto(1,10);
            lcd_writedata('<');
            state_BTN2_prev = state_BTN2;
        }

    }
    return 0;   /* never reached */
}

void LED_servo_control (int8_t l, int8_t h){
    if (F_whole<l){ //turning blue LED on
        PORTC |= (1<<4);
        OCR1B = OCR1A;
        PORTC &= ~(1<<2);

        //enable
        TIMSK2 |= (1<<TOIE2);
        
    }
    else if (F_whole>h){
        PORTC |= (1<<2);
        OCR1B = OCR1A;
        PORTC &= ~(1<<4);
        //enable
        TIMSK2 |= (1<<TOIE2);
    }
    else{
        PORTC |= ((1<<2) | (1<<4));
        if (l==h){
            OCR1B = 1875; //halfway between full bright and dim

        }
        else{
            OCR1B = 1250/(l-h)*(F_whole-l)+OCR1A; //equation derived using two points and then finding the proper slope
        }
        //disable
        TIMSK2 &= ~(1<<TOIE2);
        servo_flag = 1; //make servo_flag 1 so when it becomes too hot/cold ISR will reset servo to initial position
    }
}

ISR(PCINT0_vect) {
    if (state_BTN2==local){ //modification so the flag won't trigger while in the remote state
        BTN_change = 1;
    }
}

ISR(PCINT1_vect){
    char input = PINC; //to check whether btn2 or send was pressed
    if ((input & (1<<BTN2))==0){
        BTN2_change = 1;
    }
    else if ((input & (1<<RIGHT))==0){
        SEND_change = 1;
    }
}

void debounce_PB(int btn){
    _delay_ms(5);
    while((PINB&(1<<btn))==0){}
    _delay_ms(5);
}
void debounce_PC(int btn){
    _delay_ms(5);
    while((PINC&(1<<btn))==0){}
    _delay_ms(5);
}
