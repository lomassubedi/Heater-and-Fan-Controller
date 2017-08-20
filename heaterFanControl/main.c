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

#define		ANALOG_TMP_PIN	7

void init_io() {
	// Initialize IOs at PORT D
	IO_PORTD_DIR |= (1 << PORTD3) | (1 << PORTD4);
	IO_PORTD_DIR &= ~((1 << SW_A) | (1 << SW_B) | (1 << SW_C));

	// Initialize IO at port B
	IO_PORTB_DIR |= (1 << DDRB);
}


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
	
	/*********************************************/
	/*		For Heater Control at PD6			 */
	/*********************************************/	
	DDRD |= (1 << PORTD6);     // Set PD6 : Output
	PORTD &= ~(1 << PORTD6);
	// Initial TIMER0 Phase Correct PWM  
	// f_PWM = 16MHz / (64 * 510) = 490.196 Hz
	TCCR0A = 0b10000001; // Phase Correct PWM 8 Bit, Clear OCA0 on Compare Match, Set on TOP
	TCCR0B = 0b00000011; // Used 64 Prescaler

	TCNT0 = 0;           // Reset TCNT0
	OCR0A = 0;           // Initial the Output Compare register A & B 
	
	
	/*********************************************/
	/*		For Fan control at PPB1				 */
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

void init_adc(void) {
	
	// 128 prescaller -> Fadc = 125KHz
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// Channel 7, left justified result, vref = AVcc
	ADMUX |= (1 << REFS0) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);	
	return;
}

uint16_t get_temp() {
	// Start conversion
	ADCSRA |= (1 << ADSC);
	
	// Wait until the conversion completes 
	while (ADCSRA & (1 << ADSC));
	
	return ADC;
}

uint8_t sw_a() {
	if(IO_PORTD_IN & (1 << SW_A)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t get_sw_a() {
	static uint8_t count_sw_a = 0;
	
	if(sw_a()) {		
		_delay_ms(20);  // De-bounce Time
		while(sw_a());
		count_sw_a++;	// Increament the counter
		
		if(count_sw_a > 4)	count_sw_a = 1; // Initial Value is 1
	}
	
	return count_sw_a;
}


uint8_t get_sw_b() {
	if(IO_PORTD_IN & (1 << SW_B)) {
		return 1;
	} else {
		return 0;
	}
}


uint8_t get_sw_c() {
	if(IO_PORTD_IN & (1 << SW_C)) {
		return 1;
	} else {
		return 0;
	}
}

int main(void) {    
	pwm_init();
	init_timerModule();
	sei();
	init_adc();

	uint8_t test = 0;
	uint8_t count_sw_b = 0;	
	uint8_t count_sw_c = 0;

	pwm_fan(84);
	pwm_heater(84);
	DDRC = 0xFF;
	
    while (1) {		
		
	test = get_sw_a();
	
	if(test == 1) {
		PORTC = 0x01;
	} else if(test == 2) {
		PORTC = 0x02;
	} else if(test == 3) {
		PORTC = 0x03;
	} else if(test == 4) {
		PORTC = 0x04;
	} else {
		PORTC = 0x00;
	}
	}
}

