/*
 * attiny_adc_lib.c
 *
 * Created: 4/15/2018 9:42:28 PM
 * Author : Manny
 * Wiring of the devices
 *  Attiny85 pin	||	Arduino Uno pin
 *	DO	(pin 6)		||	MISO	(pin 12)
 *	DI	(pin 5)		||	MOSI	(pin 11)
 *	USCK(pin 7)		||	SCK		(pin 13)
 */ 
#define F_CPU 8000000L
#define _20x	1

#define SINGLE	1
#define DIFF	2

#define VCC		1
#define AREF	2
#define Internal_1V1	3
#define Interal_2V56	4
#define Interal_2V56_BpCap	5

#define _8BIT	1
#define _10BIT	2

#define ADC0	1
#define ADC1	2
#define ADC2	3
#define ADC3	4

#define DIV2	1
#define DIV4	2
#define DIV256	7
#include <avr/io.h>
#include <util/delay.h>
void spi_begin();


struct adc_config{
	int8_t	Vref;
	int8_t channel;
	int8_t	mode;
	int8_t gain;
	int8_t	resolution;
	int8_t	pre_scale;
	};
struct adc_config adc;


void adc_config(struct adc_config);
void adc_start();
void adc_enable();
int8_t adc_avaliable();
int16_t adc_getvalue();

int SS_flage=1;
struct buffer{
	int8_t buffer[100];		// LIFO buffer
	int8_t pointer;		// points to the location to be write, the read position is (pointer-1)
	int8_t full;
	int8_t empty;
	};
struct buffer temp={{0},0,0,1};
int8_t push(int8_t data,struct buffer *temp)
{
	if(temp->pointer>9)
	{
		temp->full = 1;
	}
	else
	{
		*(temp->buffer+temp->pointer)=data;
		temp->full = 0;
		temp->pointer++;
	}
	if(temp->full)
		return 0;
	else
		return 1;
}

int8_t pull(struct buffer *temp)
{
	if(temp->pointer<=0)
	{
		temp->empty=1;
		return 0;
	}
	else
	{
		temp->pointer--;
		temp->empty = 0; 
		return *(temp->buffer+temp->pointer);			// load data
	}
}
int main(void)
{
	spi_begin();	// Set device as spi slave
	USIDR = 0X32;	// load address
	adc.Vref = Internal_1V1;
	adc.mode= SINGLE;
	adc.channel = ADC2;
	adc.resolution = _8BIT;
	adc.pre_scale = DIV256;
	adc_config(adc);
	adc_enable();
	adc_start();
	/*
	int32_t a=0xABCDEF12;
	//float a=30.14;
	int8_t *x;
	x = (int8_t*)&a;
	// Load data LSB first on LIFO buffer
	*(buffer+pointer)=*x;//0x32;
	if((pointer<100))
	pointer++;
	
	*(buffer+pointer)=*(x+1);//0x45;
	if((pointer<100))
	pointer++;
	
	*(buffer+pointer)=*(x+2);//0x45;
	if((pointer<100))
	pointer++;
	
	*(buffer+pointer)=*(x+3);//0x11;
	if((pointer<100))
	pointer++;
	// Note the receiver will receive MSB first
	*/
	for(int i=0;i<20;i++)
	{
		push(i,&temp);
	}
	_delay_ms(100);
	while (1)
    {
		if(adc_avaliable())
		{
			// both the ADCL and ADCH registeres must be read, other wise they will not be updated
			// read ADCL first if more then 8-bit resulution is required.
			adc_start();		// start next convention
		}
		if((PINB&(1<<PINB3))&&SS_flage)
		{
			SS_flage=0;
			USIDR = pull(&temp);
			//*(buffer+pointer)=0x00;	// empty buffer
		}
		else if(!(PINB&(1<<PINB3)))
		{
			SS_flage=1;
		}
    }
}

// SPI in slave mode //
void spi_begin()
{
	DDRB |= (1<<PB1);
	USICR |= (1<<USIWM0)|(1<<USICS1);					// sample at faling edge
	//USICR |= (1<<USIWM0)|(1<<USICS1)|(1<<USICS0);		// sample at rising edge
}
// ADC library
int16_t adc_getvalue()
{
	int16_t adc_value;
	switch(adc.resolution)
	{
		case _8BIT:
			adc_value=ADCH;
			break;
		case _10BIT:
			adc_value=ADCL;
			adc_value=(ADCH<<8);
			break;
		default:
			adc_value = 0;
			break;
	}
	return adc_value;
}
int8_t adc_avaliable()
{
	int8_t adc_flage;
	adc_flage = ADCSRA & (1<<ADIF);		// get the value of the intrupt falge
	if(adc_flage)						// if flage set 
		ADCSRA|= (1<<ADIF);				// clear falge
	return adc_flage;
}
void adc_config(struct adc_config adc)
{
	switch(adc.channel)
	{
		case ADC0:
			ADMUX &= 0xF0;		// clear the LSB Hex, set to channel 0
			break;
		case ADC1:
			ADMUX &= 0xF0;
			ADMUX |= (1<<MUX0);
			break;
		case ADC2:
			ADMUX &=0xF0;
			ADMUX |= (1<<MUX1);
			break;
		case ADC3:
			ADMUX &=0xF0;
			ADMUX |= (1<<MUX0)|(1<<MUX1);
			break;
		default:
			ADMUX &= 0xF0;
			break;
	}
	switch(adc.Vref)
	{
		case Internal_1V1:
			ADMUX |= (1<<REFS1);		// set reference to 1.1V internal
			break;
		case Interal_2V56:
			break;
		case Interal_2V56_BpCap:
			break;
		case AREF:
			break;
		case VCC:
			break;
		default:
			break;
	}
	switch(adc.pre_scale)
	{
		case DIV2:
			break;
		case DIV4:
			break;
		case DIV256:
			ADCSRA|= (1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
			break;
		default:
			break;
	}
	switch(adc.resolution)
	{
		case _8BIT:
			ADMUX |= (1<<ADLAR);	// Left adjust the 10-bit adc result
			break;
		case _10BIT:
			ADMUX &= ~(1<<ADLAR);	// Right adjust the 10-bit adc result
			break;
		default:
			break;
	}
}

void adc_start()
{
	ADCSRA|= (1<<ADSC);							// start convention
}

void adc_enable()
{
	ADCSRA|= (1<<ADEN);							// enable adc
}