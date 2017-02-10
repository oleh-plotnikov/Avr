/*
 * AtTiny_13_cooler.c
 *
 * Created: 09.02.2017 11:45:29
 * Author : oleh_plotnikov
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
char SR0=SREG; //save date register
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
SREG = SR0; // return previous value
_delay_us(420);
return device;
}

void OWI_write_bit (char bit)
{
char SR1 = SREG;
cli();
OWI_LOW;
_delay_us(2);
if(bit) OWI_HIGH;
_delay_us(65);
OWI_HIGH;
SREG = SR1;
}

void OWI_write_byte(unsigned char c)
{
	char i;
	for (i = 0; i < 8; i++)
	{
		if((c & (1 << i )) == 1 << i)
		OWI_write_bit(1);
		else
		OWI_write_bit(0);
	}
}

char OWI_read_bit(void)
{
	char SR2 = SREG;
	cli();
	char OWI_rbit;
	OWI_LOW;
	_delay_us(2);
	OWI_HIGH;
	_delay_us(13);
	OWI_rbit=(OWI_PIN & (1 << OWI_BIT));
	_delay_us(45);
	SREG = SR2;
	return OWI_rbit;
}

unsigned char OWI_read_byte()
{
	char data = 0,i;
	for (i = 0; i < 8; i++)
	data |= OWI_read_bit() << i;
	return data;
}

int temp_18b20 ()
{
	unsigned char B;
	unsigned int ds18_temp = 0;
	if (OWI_find() == 1)
	{
		OWI_write_byte(SKIP_ROM);
		OWI_write_byte(CONVERT_T);
		_delay_us(750);
		OWI_find();
		OWI_write_byte(SKIP_ROM);
		OWI_write_byte(READ_SCRATCHPAD);
		
		B = OWI_read_byte();
		ds18_temp = OWI_read_byte();
		ds18_temp =(ds18_temp<<8)|B;
	}
	return ds18_temp;
}

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
		if (T < 0x32)
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