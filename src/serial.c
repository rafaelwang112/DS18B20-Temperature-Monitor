#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "serial.h"
#include "lcd.h"

volatile char rx_buf [5];
char rx_buf_display [5]; //a separate buffer I declare to store the contents of rx_buf so sscanf doesn't take a volatile pointer
volatile uint8_t start_received = 0;
volatile uint8_t num_received = 0;
volatile uint8_t valid_string = 0;
int8_t rx_high;
int8_t rx_low;


void display_rx(){
    for (int i = 0; i<4; i++){ //filling the new buffer with whats in rx_buf
        rx_buf_display[i] = rx_buf[i];
    }
    rx_buf_display[4] = '\0'; //adding the null terminator for sscanf

    int num;
    sscanf(rx_buf_display, "%d", &num);
    rx_high = ((num/10)%10)*10+num%10;
    rx_low = num/100;

    char l_rxcounter[8]; 
	snprintf(l_rxcounter, 8, "L=%d", rx_low);
	lcd_moveto(1,1);
	lcd_stringout(l_rxcounter);

    char h_rxcounter[8]; 
	snprintf(h_rxcounter, 8, "H=%d", rx_high);
	lcd_moveto(1,6);
	lcd_stringout(h_rxcounter);

}



void serial_init(void){
    UBRR0 = 16000000/16/9600-1; //setting BUAD rate
    UCSR0B |= (1 << TXEN0 | 1 << RXEN0); // Enable RX and TX
    UCSR0C = (3 << UCSZ00); // Async., no parity, 1 stop bit, 8 data bits
    UCSR0B |= (1<<RXCIE0); //enable reciever interrupts

}

void char_tx (char c){
    while ((UCSR0A & (1<<UDRE0)) == 0) {} //poll until register is empty
    UDR0 = c;
}

ISR(USART_RX_vect){
    char rx_char = UDR0;
    
    if (rx_char=='<'){ //indicating start
        start_received = 1;
        num_received = 0;
        valid_string = 0;
    }

    else if (start_received == 1){
        if (rx_char<'0' || rx_char>'9'){ //receives something either than 0 to 9
            start_received = 0;
            if (rx_char =='>'){ //if whats received is > and num_received is greater than 0 then its valid string
                if (num_received>0){
                    valid_string = 1;

                }
            }
        }
        else{ //if whats received is a correct character
            rx_buf[num_received] = rx_char; 
            num_received++;
        }
        if (num_received>4){ //means thats index 4(or the spot after last digit) wasn't a '>' so length has been overrun
            start_received = 0;
        }
    
    }
} 