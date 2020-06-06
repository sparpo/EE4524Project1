#include <avr/io.h>
#include <avr/interrupt.h>
#define DLY_2_ms 141
/* We use 141 because we want Timer0 to count 125 counts, and 256-141 = 125  */
#define COUNT_FOR_10ms	5
unsigned int rgb = 1;
unsigned int timecount0;
unsigned int delayTime = 40;

unsigned int adc_reading[10];
unsigned int adc_average;
unsigned int adccount = 0;

unsigned int blue_led_threshold = 512; //threshold is 1024* (2.5/5) = 2.5V
unsigned int led_time_threshold = 768; // 1024* (3.75/5) = 3.75V

unsigned int buzzerDelay = 1;
unsigned int buzzerCount;


int main(void)
{
	ADMUX = (1<<6); //sets voltage ref
	ADCSRA  = 0b11101111; //enable adc, starts conversion, enable interrupt, sets prescalar 128
	ADCSRB = 0;// sets free running mode
	
	DDRB = 0x00;
	DDRB = (1<<1|1<<2|1<<3|1<<4|1<<5); //0b00111110;
 
	DDRD = 0x00; // set portd to inputs
	DDRD = (1<<5); //0b00100000, pin 5 is buzzer
	
	PORTB = 0;		/* Initialize to all off							*/
	PORTD = (1<<3|1<<2); //enable pullup on pin 3 & 2
	
	timecount0 = 0;		/* Initialise the overflow count. Note its scope	*/
	TCCR0B = (5<<CS00);	/* Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode	*/
	
	TCCR0A = 61;			/* Not strictly necessary as these are the reset states but it's good	*/
	/* practice to show what you're doing									*/
	TCNT0 = 0;			/* Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms		*/
	TIMSK0 = (1<<TOIE0);		/* Enable Timer 0 interrupt										*/
	
	sei();						/* Global interrupt enable (I=1)								*/
	
	
	while(1){ 
		int total = 0; //avg
		for(int i = 0;i<10;i++) {
			total+=adc_reading[i];
		}
		
		adc_average = total/10; //avg adc reading
		if(adc_average>led_time_threshold) { //if adc reading is above 3.75V
			delayTime = 40; //period is 1s
			} else { //if less than 3.75
			delayTime = 10; // period is 0.25s
		}
		if((PIND & (1<<2)) == (1<<2)) {//switch 1 high
			PORTB =  PORTB & ~(1<<4); // red off
		} else { //switch 1 low	
			PORTB = PORTB | (1<<4); //red on
			if(adc_average>blue_led_threshold) { //if adc reading is above 2.5V
				PORTB = PORTB | (1<<5); // blue on
				buzzerDelay = 2; // buzzer at 20Hz
			} else {
				PORTB =  PORTB & ~(1<<5); // blue off
				buzzerDelay = 1; // buzzer at 40Hz
			}
		}
	}					/* Do nothing loop												*/
}

ISR (ADC_vect)//handles ADC interrupts
{
	adccount++;
	if(adccount <= 9) {
			adc_reading[adccount] = ADC;   /* ADC is in Free Running Mode - you don't have to set up anything for 
						    the next conversion */
	} else {
		adccount = 0;
	}
	

}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 61;		/*	TCNT0 needs to be set to the start point each time				*/
	++timecount0;			/* count the number of times the interrupt has been reached			*/ 
	++buzzerCount;
	if (timecount0 >= delayTime)	/* 5 * 2ms = 10ms									*/
	{	
		PORTB = 0;
		if((PIND & (1<<3)) == (1<<3)) { //if switch 2 is high
			PORTB = 1<<rgb;
			if(rgb <= 3) {
				rgb++;
			} else {
				PORTB = 0;
				rgb = 1;
			}
		} else { //switch 2 low
			
			if(rgb<7) {
				rgb++;
			} else {
				rgb = 0;
			}
			PORTB = (PORTB | (rgb<<1));
		}
		

		timecount0 = 0;		/* Restart the overflow counter					*/	
	}
	if (timecount0 >= buzzerDelay)	{
		if((PIND & (1<<2)) == (1<<2)) { //if switch 1 is high
			PORTD = PORTD ^ (1<<5); // flips bit 5 to create sound
		}
		buzzerCount = 0;
	}
	
}



