/*
 * AtTiny_13_cooler.c
 *
 * Created: 09.02.2017 11:45:29
 * Author : oleh_plotnikov
 * Program for ATiny 13A.
 * Project Solar Battery cooland.
 */
#define F_CPU 1150000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SKIP_ROM 0xCC // Skip ROM
#define CONVERT_T 0x44 // convert temperature
#define READ_SCRATCHPAD 0xBE // read temperature

// define port and bit ds18b20
#define OWI_PORT PORTB
#define OWI_DDR DDRB
#define OWI_PIN PINB
#define OWI_BIT 0
#define ALARM 3

#define OWI_LOW OWI_DDR |=1<<OWI_BIT // pulldown wire
#define OWI_HIGH OWI_DDR &= ~ (1<<OWI_BIT) // release wire

// Does device on wire
char OWI_find(void)
{
	char SR0=SREG; //save value register
	cli(); // forbid interrupt
	char device;
	OWI_LOW; // pulldown wire
	_delay_us(485);  //wait min 480us
	OWI_HIGH; // release wire
	_delay_us (65); // wait min 60 us and read state in wire

	if((OWI_PIN & (1<<OWI_BIT)) == 0x00) // wait answer
		device = 1; // device on wire
	else
		device = 0;
		SREG = SR0; // return previous value of register
		_delay_us(420); // wait remaining time
	
	return device; 
}

void OWI_write_bit (char bit) //this function send bit on device
{
	char SR1 = SREG; //save value register
	cli(); // forbid interrupt
	OWI_LOW; // pull down wire
	_delay_us(2);

	if(bit) OWI_HIGH; // release wire
		_delay_us(65);
		OWI_HIGH; // release wire
		SREG = SR1; // return previous value of register
}

// this function send byte on device
void OWI_write_byte(unsigned char c)
{
	char i;
	for (i = 0; i < 8; i++) // send 8 bit in cycle
	{
		if((c & (1 << i )) == 1 << i)c // if bit 1 send 1
			OWI_write_bit(1);
		else //esle send zero bit
			OWI_write_bit(0);
	}
}

// this function one bit
char OWI_read_bit(void)
{
	char SR2 = SREG; //save value register
	cli(); //forbid interrupt
	char OWI_rbit; // var for store bit
	OWI_LOW; // pulldown wire
	_delay_us(2);
	OWI_HIGH; // pullup wire
	_delay_us(13);
	OWI_rbit=(OWI_PIN & (1 << OWI_BIT)); //read bit
	_delay_us(45); 
	SREG = SR2; // return previous value of register
	return OWI_rbit;
}

// this function read one byte from device 1-wire
unsigned char OWI_read_byte()
{
	char data = 0,i;
	
	for (i = 0; i < 8; i++) // in this cycle read data on wire
		data |= OWI_read_bit() << i;
	
	return data;
}

// function convert recived data from device
int temp_18b20 ()
{
	unsigned char B;
	unsigned int ds18_temp = 0;
	if (OWI_find() == 1) // if device find on wire
	{
		OWI_write_byte(SKIP_ROM); // skip ROM, because we know that only one device in wire
		OWI_write_byte(CONVERT_T); // convert temperature
		_delay_us(750); // convert time for 12 bit 750ms
		OWI_find();
		OWI_write_byte(SKIP_ROM);
		OWI_write_byte(READ_SCRATCHPAD); //send command, in order to 18b20 send temp
		B = OWI_read_byte(); // read LS byte 
		ds18_temp = OWI_read_byte(); // read MS byte
		ds18_temp =(ds18_temp<<8)|B; // change places MS and LS byte
	}
	return ds18_temp; // return in format (MS, LS)
}

// function convert temp 
char convert (unsigned int td)
{
	char dat = td >> 4;
	return dat;
}

int main(void)
{
	//DDRB=0x02;
	PORTB=0x50;
	char T = 0;
	
	while(1)
	{
		T = convert(temp_18b20());
	
		if (T < 0x46) // if temperature is less from 70 (0x46) C, motor off.
		{
			DDRB |=(1<<ALARM);
			PORTB &= ~ (1 << ALARM);
			_delay_ms(2000);
		}
		else
		{
			DDRB |=(1<<ALARM);
			PORTB |=(1<<ALARM);
			_delay_ms(2000);
		}

	}
}
