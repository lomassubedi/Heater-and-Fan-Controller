/*
 * heaterFanControl.c
 *
 * Created: 8/19/2017 12:33:59 AM
 * Author : LOMAS
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>


#define		SW_A			PIND2
#define		SW_B			PIND1
#define		SW_C			PIND0

#define		F_LED_CNTL		PORTD3
#define		H_LED_CNTL		PORTD4

#define		IO_PORTD_OUT	PORTD
#define		IO_PORTD_IN		PIND
#define		IO_PORTD_DIR	DDRD

#define		LED_SIG			PORTB2
#define		IO_PORTB_OUT	PORTB
#define		IO_PORTB_DIR	DDRB


#define		PWM_PERCENT_33		84
#define		PWM_PERCENT_66		166
#define		PWM_PERCENT_100		255
	
static uint16_t PWM_FREQ = 490;
static uint16_t TOP_VAL = 0;

volatile unsigned long milliseconds;

ISR(TIMER2_COMPA_vect) {
	++milliseconds;
}

unsigned long millis(void) {
	return milliseconds;
}

void init_timerModule() {
	TIMSK2 |= (1 << OCIE2A);					// Enable the compare match interrupt	
	TCCR2A |= (1 << WGM21);						// CTC mode
	TCCR2B |= (1 << CS22) | (1 << CS21);		// Clock/256
	OCR2A = 0x3F;								// Load the offset
	return;
}
void pwm_init() {			
	
	/********************************************
	 *		For Heater Control at PD6
	/*********************************************/	
	DDRD |= (1 << PORTD6);     // Set PD6 : Output
	PORTD &= ~(1 << PORTD6);
	// Initial TIMER0 Phase Correct PWM  
	// f_PWM = 16MHz / (64 * 510) = 490.196 Hz
	TCCR0A = 0b10000001; // Phase Correct PWM 8 Bit, Clear OCA0 on Compare Match, Set on TOP
	TCCR0B = 0b00000011; // Used 64 Prescaler

	TCNT0 = 0;           // Reset TCNT0
	OCR0A = 0;           // Initial the Output Compare register A & B 
	
	
	/********************************************
	 *		For Fan control at PPB1
	/*********************************************/		
	DDRB |= (1 << PORTB1);
	PORTB &= ~(1 << PORTB1);
	// Initial TIMER1 Phase and Frequency Correct PWM
	// Set the Timer/Counter Control Register
	// TOP = (16000000/(2 * 64 * 490)) = 255.10
	TCCR1A = 0b10000000; // Clear OC1A when up counting, Set when down counting
	TCCR1B = 0b00010011; // Phase/Freq-correct PWM, top value = ICR1, Prescaler: 64
	
	TOP_VAL = (F_CPU / (128 * PWM_FREQ));
	
	ICR1 = TOP_VAL;	
	TCNT1 = 0;		
	OCR1A = 0;	
	return;
}

void pwm_heater(uint8_t p) {				
	/*OCR0A = (uint8_t)((p / 100) * 255);*/
	OCR0A = p;
	return;
}

void pwm_fan(uint8_t p) {	
	/*OCR1A = (uint16_t)((p / 100) *  TOP_VAL);*/
	/*OCR1A = TOP_VAL;*/
	OCR1A = p;	
	return;	
}

int main(void) {    
	pwm_init();
	init_timerModule();
	sei();
	pwm_fan(84);
	pwm_heater(84);
	DDRC = 0xFF;
    while (1) {
		
		if((millis() % 1000) == 0) {
			PORTC ^= 0xAA;
		}
		
		
    }
}

