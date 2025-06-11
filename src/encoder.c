#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "encoder.h"
#include "project.h"
#include "lcd.h"

 //global variables required for ISR
 volatile char input;
 volatile uint8_t a, b;
 volatile uint8_t new_state, old_state;
 volatile int8_t changed = 0;  // Flag for state change

 int8_t low;
 int8_t high;

 void pin_change_init(void);
 void initial_state(void);

 void encoder_init(){
    PORTD |= ((1<<PD2) | (1<<PD3)); //enable pull-ups for rotary encoder
    pin_change_init();
    initial_state();
    low = eeprom_read_byte((void *) 100);
    high = eeprom_read_byte((void *) 200);

    if (low <50 || low>90){ //if low is not in the range give it default value
        low = 50;
    }
    if (high<50 || high > 90){ //if high is not in the range give it default value
        high = 90;
    }

 }

 void initial_state(){
    input = PIND;
     if ((input & 0x04) != 0){ //if the mask isnt 0 then a is a 1
         a = 1;
     }
     else {
         a = 0;
     }
 
     if ((input & 0x08) != 0){ //if the mask isnt 0 then b is a 1
         b = 1;
     }
     else {
         b = 0;
     }
 
     if (!b && !a)
    old_state = 0;
     else if (!b && a)
    old_state = 1;
     else if (b && !a)
    old_state = 2;
     else
    old_state= 3;
    new_state = old_state;

 }
void display_initial_lim(){
    char l_icounter[8]; 
	snprintf(l_icounter, 8, "L=%d", low);
	lcd_moveto(0,1);
	lcd_stringout(l_icounter);

    char h_icounter[8]; 
	snprintf(h_icounter, 8, "H=%d", high);
	lcd_moveto(0,6);
	lcd_stringout(h_icounter);

}
 void display_limits(){
    if (changed){
        changed = 0;
        char l_counter[8]; 
	    snprintf(l_counter, 8, "L=%d", low);
	    lcd_moveto(0,1);
	    lcd_stringout(l_counter);

        char h_counter[8]; 
	    snprintf(h_counter, 8, "H=%d", high);
	    lcd_moveto(0,6);
	    lcd_stringout(h_counter);

        eeprom_update_byte((void *) 100, low); //update the low value in eeprom
        eeprom_update_byte((void *) 200, high); //update the high value in eeprom
    }

 }

 void pin_change_init(){ //enable pin change interrupts
    PCICR|= (1<<PCIE2);
    PCMSK2 |= (1<<PCINT18) | (1<<PCINT19);
 }
 
 ISR(PCINT2_vect)
 {
    if (state_BTN2==local){
     input = PIND;
     if ((input & 0x04) != 0){ //if the mask isnt 0 then a is a 1
         a = 1;
     }
     else {
         a = 0;
     }
     
     if ((input & 0x08) != 0){ //if the mask isnt 0 then b is a 1
         b = 1;
     }
     else {
         b = 0;
     }

     if (old_state == 0) { //current state is 00
         
         // Handle A and B inputs for state 0
         if (a ==1 && b ==0){ //if goes to 01, its clockwise so increment
            new_state = 1;
             if (state_BTN1==LO){
                if (low<high && low<90){
                    low++;
                }
             }
             else{
                if (high<90){
                    high++;
                }

             }
         }
         if (a == 0 && b == 1){ //if goes to 10, its counterclockwise so decrement
            new_state = 2;
             if (state_BTN1==LO){
                if (low>50){
                    low--;
                }
             }
             else{
                if (high>low && high>50){
                    high--;
                }
             }
         }
 
     }
 
     else if (old_state == 1) { //current state is 01
 
         // Handle A and B inputs for state 1
         if (a ==1 && b ==1){ //if goes to 11, its clockwise so increment
            new_state = 3;
             if (state_BTN1==LO){
                if (low<high && low<90){
                    low++;
                }
             }
             else{
                if (high<90){
                    high++;
                }

             }
         }
         if (a == 0 && b == 0){ //if goes to 00, its counterclockwise so decrement
            new_state = 0;
             if (state_BTN1==LO){
                if (low>50){
                    low--;
                }
             }
             else{
                if (high>low && high>50){
                    high--;
                }
             }
         }

     }
 
     else if (old_state == 2) { //current state is 10
 
         // Handle A and B inputs for state 2
         if (a ==0 && b ==0){  //if goes to 00, its clockwise so increment
            new_state = 0;
             if (state_BTN1==LO){
                if (low<high && low<90){
                    low++;
                }
             }
             else{
                if (high<90){
                    high++;
                }

             }
         }
         if (a == 1 && b == 1){  //if goes to 11, its counterclockwise so decrement
            new_state = 3;
             if (state_BTN1==LO){
                if (low>50){
                    low--;
                }
             }
             else{
                if (high>low && high>50){
                    high--;
                }
             }
         }
     
     }
     else {   // old state is 3 which is 11
         // Handle A and B inputs for state 3
         if (a ==0 && b == 1){  //if goes to 10, its clockwise so increment
            new_state = 2;
             if (state_BTN1==LO){
                if (low<high && low<90){
                    low++;
                }
             }
             else{
                if (high<90){
                    high++;
                }

             }
         }
         if (a == 1 && b == 0){ //if goes to 01, its counterclockwise so decrement
            new_state = 1;
             if (state_BTN1==LO){
                if (low>50){
                    low--;
                }
             }
             else{
                if (high>low && high>50){
                    high--;   
                }
             }
         }
     }
     if (new_state!=old_state){
        changed = 1;
        old_state = new_state;
     }
    }
 }