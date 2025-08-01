/*
  Interface routines for the DS18B20

*/

#include <avr/io.h>
#include <util/delay.h>

#include "ds18b20.h"

char ds_reset(void);
void ds_write1bit(void);
void ds_write0bit(void);
unsigned char ds_readbit(void);
void ds_writebyte(unsigned char);
unsigned char ds_readbyte(void);

// DS18B20 function commands
#define DS_SKIP_ROM         0xcc
#define DS_CONVERT_T        0x44
#define DS_COPY_SCRATCHPAD  0x48
#define DS_WRITE_SCRATCHPAD 0x4e
#define DS_READ_SCRATCHPAD  0xbe

// I make the 1-Wire bus high by setting port bit for input
// and the external pull-up resistor will make it go high.
// DDR bit = 0, PORT bit = 0
//
// I make the 1-Wire bus low by setting port bit for output,
// and with a zero in the PORT bit it will go low.
// DDR bit = 1, PORT bit = 0

/*
  ds_init - Initializes the port bit used for the 1-Wire bus.
*/
char ds_init(void)
{
    unsigned char i, scratchpad[9];

    // Set the 1-Wire port bit for input with the internal pull-up turned off.
    DDRC &= ~(1 << PC1);        // Put bus in high-Z state
    PORTC &= ~(1 << PC1);       // Put a zero in the port bit

    _delay_ms(100);             // Wait for pull-up to take effect
    if ((PINC & (1 << PC1)) == 0)  // Check that bus is high
	return(0);              // If not, pull-up probably missing

    // Set the configuration register for 12-bit conversions
    // just in case some chips got set to lower resolution.
    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);    // Send a "Skip ROM" command
	ds_writebyte(DS_READ_SCRATCHPAD); // Send a "Read Scratchpad" command
	for (i = 0; i < 9; i++) {      // Read 9 bytes of scratchpad data
	    scratchpad[i] = ds_readbyte();
	}
    }
    else
	return(0);

    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);     // Send a "Skip ROM" command
	ds_writebyte(DS_WRITE_SCRATCHPAD); // Send a "Write Scratchpad"
	ds_writebyte(scratchpad[2]);
	ds_writebyte(scratchpad[3]);
	ds_writebyte(0x7f);    // Set for 12-bit conversions
    }
    else
	return(0);

    _delay_us(100);

    if (ds_reset()) {           // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);     // Send a "Skip ROM" command
	ds_writebyte(DS_COPY_SCRATCHPAD);  // Send a "Copy Scratchpad"
    }
    else
	return(0);

    _delay_ms(20);

    return(1);
}

/*
  ds_convert - Sends a "CONVERT_T" command to start a conversion.  Doesn't
  wait for it to finish.
*/
void ds_convert()
{
    if (ds_reset()) {                   // Reset the DS18B20
	ds_writebyte(DS_SKIP_ROM);  // Send a "Skip ROM" command
	ds_writebyte(DS_CONVERT_T); // Send a "Convert T" command
    }
}

/*
  ds_temp - Reads a bit from the DS18B20.  Returns 0 if the bit is a zero
  (conversion not complete).  If the bit is a one read the 9 bytes in
  the scratchpad and return the first two in an array.
*/
char ds_temp(unsigned char *tdata)
{
    unsigned char i;

    if (ds_readbit()) {                 // Wait for a read to return a one
	if (ds_reset()) {               // Reset the DS18B20
	    ds_writebyte(DS_SKIP_ROM);  // Send a "Skip ROM" command
	    ds_writebyte(DS_READ_SCRATCHPAD);  // Send a "Read Scratchpad" cmd
	    *tdata++ = ds_readbyte();   // Read the first byte (LSB)
	    *tdata = ds_readbyte();     // Read the second byte (MSB)
	    for (i = 0; i < 7; i++) {   // Read the rest of the scratchpad data
		(void) ds_readbyte();
	    }
	}
	return(1);
    }
    else
	return(0);
}

/*
  ds_reset - Sends a reset pulse out the bus and then check for
  the "presence" pulse that a device on the bus should send back.
  The presense pulse could start 15usec after the bus goes high
  and be gone by 75usec, or could start in 60usec and be gone by
  120usec. I split the difference between 60 and 75usec and
  wait 67usec.
*/
char ds_reset(void)
{
    DDRC |= (1 << PC1);         // Pull bus low
    _delay_us(600);             // Delay >480usec
    DDRC &= ~(1 << PC1);        // Let bus go high
    _delay_us(67);              // Wait for Presense pulse
    if ((PINC & (1 << PC1)) == 0) {
	while ((PINC & (1 << PC1)) == 0) {} // Wait for pulse to end
	_delay_us(50);          // Probably don't need to do this
        return(1);              // Return status OK
    }
    else
	return(0);              // Return reset failed

}

/*
  ds_writebyte - Writes an 8-bit byte out the bus
*/
void ds_writebyte(unsigned char x)
{
    unsigned char i;

    i = 8;
    while (i != 0) {
        if (x & 1)              // Check the LSB
            ds_write1bit();     // Send a one
        else
            ds_write0bit();     // Send a zero
        x >>= 1;                // Shift x over towards the LSB
        i--;
    }
}

/*
  ds_readbyte - Reads an 8-bit byte from some device on the bus
*/
unsigned char ds_readbyte()
{
    unsigned char i, x, m;

    x = 0;
    m = 1;                      // m = mask for sticking bits in x
    i = 8;
    while (i != 0) {
        if (ds_readbit())       // Get a bit
            x |= m;             // If a one, put a one in x
        m <<= 1;                // Shift the mask towards the MSB
        i--;
    }
    return(x);
}

/*
  ds_write1bit - Writes a single 1 bit out the bus
*/
void ds_write1bit(void)
{
    PORTC &= ~(1 << PC1);       // just to ensure its a 0
    DDRC |= (1<<PC1); //pulling bus low for 2 us
    _delay_us(2);
    DDRC &= ~(1<<PC1); //pulling it high for 60 us
    _delay_us(60);

}

/*
  ds_write0bit - Writes a single 0 bit out the bus
*/
void ds_write0bit(void)
{
    PORTC &= ~(1 << PC1);       // just to ensure its a 0
    DDRC |= (1<<PC1); //pulling bus low for 60 us
    _delay_us(60);
    DDRC &= ~(1<<PC1); //pulling it high for 2 us
    _delay_us(2);

}

/*
  ds_readbit - Reads a single bit from the bus.  Return
  value is either 0 or non-zero.
*/
unsigned char ds_readbit(void)
{
    PORTC &= ~(1 << PC1);       // just to ensure its a 0
    DDRC |= (1<<PC1); //pulling bus low for 2 us
    _delay_us(2);
    DDRC &= ~(1<<PC1); //releasing it high
    _delay_us(10);//wait the 10 seconds
    unsigned char result = (PINC & 0x02);
    _delay_us(50);
    if (result ==0){
        return 0;
    }
    else{
        return 1;
    }

}
