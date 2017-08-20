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

#define		PWM_FAN_PERCENT_33			84
#define		PWM_FAN_PERCENT_66			166
#define		PWM_FAN_PERCENT_100			255

#define		PWM_HEATER_PERCENT_33		84
#define		PWM_HEATER_PERCENT_66		166
#define		PWM_HEATER_PERCENT_100		255

#define		PWM_HEATER_PERCENT_20		51
#define		PWM_HEATER_PERCENT_40		102
#define		PWM_HEATER_PERCENT_60		153

#define		PWM_HEATER_PERCENT_OFF		0
#define		PWM_FAN_PERCENT_OFF			0

//#define		TIMER_VAL_HEATER			7200000UL	// 120 * 60 * 1000 (in Millisecond)
//#define		TIMER_VAL_FAN				7200000UL	// 120 * 60 * 1000 (in Millisecond)
#define		TIMER_VAL_LED				1800000UL	// 30 * 60 * 1000  (in Millisecond)

// Dummy 
#define		TIMER_VAL_HEATER			10000UL	
#define		TIMER_VAL_FAN				10000UL	

	
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

void init_io() {
	// Initialize IOs at PORT D
	IO_PORTD_DIR |= (1 << PORTD3) | (1 << PORTD4);
	IO_PORTD_DIR &= ~((1 << SW_A) | (1 << SW_B) | (1 << SW_C));

	// Initialize IO at port B
	IO_PORTB_DIR |= (1 << DDRB);
}

void init_adc(void) {
	
	// 128 prescaller -> Fadc = 125KHz
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// Channel 7, left justified result, Vref = AVcc
	ADMUX |= (1 << REFS0) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);	
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

uint16_t get_temp() {
	// Start conversion
	ADCSRA |= (1 << ADSC);
	
	// Wait until the conversion completes 
	while (ADCSRA & (1 << ADSC));
	
	return ADC;
}

volatile uint8_t count_sw_a = 0;
volatile uint8_t count_sw_b = 0;
volatile uint8_t count_sw_c = 0;

uint8_t sw_a() {
	if(IO_PORTD_IN & (1 << SW_A)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t get_sw_a() {
	/*static uint8_t count_sw_a = 0;*/
	
	if(sw_a()) {		
		_delay_ms(20);  // De-bounce Time
		while(sw_a());
		count_sw_a++;	// Increment the counter
		
		if(count_sw_a > 4)	count_sw_a = 1; // Initial Value is 1
	}
	
	return count_sw_a;
}


uint8_t sw_b() {
	if(IO_PORTD_IN & (1 << SW_B)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t get_sw_b() {
	
	/*static uint8_t count_sw_b = 0;*/
	
	if(sw_b()) {
		_delay_ms(20);  // De-bounce Time
		while(sw_b());
		count_sw_b++;	// Increment the counter
		
		if(count_sw_b > 2)	count_sw_b = 1; // Initial Value is 1
	}
	
	return count_sw_b;
}



uint8_t sw_c() {
	if(IO_PORTD_IN & (1 << SW_C)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t get_sw_c() {
	
	/*static uint8_t count_sw_c = 0;*/
	
	if(sw_c()) {
		_delay_ms(20);  // De-bounce Time
		while(sw_c());
		count_sw_c++;	// Increment the counter
		
		if(count_sw_c > 4)	count_sw_c = 1; // Initial Value is 1
	}
	
	return count_sw_c;
}

void led_heater_on() {
	IO_PORTD_OUT |= (1 << H_LED_CNTL);
	return;
}

void led_heater_off() {
	IO_PORTD_OUT &= ~(1 << H_LED_CNTL);
	return;
}

void led_fan_on() {
	IO_PORTD_OUT |= (1 << F_LED_CNTL);
	return;
}

void led_fan_off() {
	IO_PORTD_OUT &= ~(1 << F_LED_CNTL);
	return;
}

void led_sig_on() {
	IO_PORTB_OUT |= (1 << LED_SIG);
	return;
}

void led_sig_off() {
	IO_PORTB_OUT &= ~(1 << LED_SIG);
	return;
}

uint8_t flag_start_timer_heater_120min = 0;
uint8_t flag_reset_timer_heater_120min = 0;
uint8_t flag_heater_off = 0;

uint8_t flag_start_timer_fan_120min = 0;
uint8_t flag_reset_timer_fan_120min = 0;

uint8_t flag_start_timer_led_30min = 0;
uint8_t flag_reset_timer_led_30min = 0;

uint8_t flag_start_read_temp_timer = 0;
uint8_t flag_reset_read_temp_timer = 0;

int main(void) {    
	pwm_init();
	init_timerModule();
	sei();
	init_adc();
	init_io();
	
	DDRC = 0xFF;
	
	uint8_t swa_val = 0;
	uint8_t swa_prev_val = 0;

	uint8_t swb_val = 0;
	uint8_t swb_prev_val = 0;

	uint8_t swc_val = 0;
	uint8_t swc_prev_val = 0;

	unsigned long time_val_heater_start = 0;
	unsigned long time_val_fan_start = 0;
	unsigned long time_val_led_start = 0;

    while (1) {		
		
		swa_val = get_sw_a();

		// ------- Heater Control -------------
		if(swa_val != swa_prev_val) {
			
			// --- Clear all other stuffs (fan/led)-----
			pwm_fan(PWM_FAN_PERCENT_OFF);
			led_fan_off();
			led_sig_off();

			// Clear the tap history
			count_sw_b = 0; swb_prev_val = 0;
			count_sw_c = 0; swc_prev_val = 0;
			
			// ---- Proceed to heater work
			time_val_heater_start = millis(); 
			swa_prev_val = swa_val;

			switch (swa_val) {
				case 1:
					pwm_heater(PWM_HEATER_PERCENT_20);
					led_heater_on();
				break;
				
				case 2:
					pwm_heater(PWM_HEATER_PERCENT_40);
					led_heater_on();
					PORTC = 0x02;
				break;
				
				case 3:
					pwm_heater(PWM_HEATER_PERCENT_60);
					led_heater_on();
					PORTC = 0x03;
				break;
				
				case 4:
					pwm_heater(PWM_HEATER_PERCENT_OFF);
					led_heater_off();
					PORTC = 0x04;
				break;
				
				default:
					pwm_heater(PWM_HEATER_PERCENT_OFF);
					led_heater_off();
					PORTC = 0x00;
				break;
			}
		}

		if((millis() - time_val_heater_start) > TIMER_VAL_HEATER) {
			pwm_heater(PWM_HEATER_PERCENT_OFF);
			led_heater_off();
		}

		// ------- End Heater Control -------------
		


		//----------- LED Control -----------------
		swb_val = get_sw_b();

		if(swb_val != swb_prev_val) {
			
			// --- Clear all other stuffs (heater/led)-----
			pwm_heater(PWM_HEATER_PERCENT_OFF);
			led_heater_off();
			
			pwm_fan(PWM_FAN_PERCENT_OFF);
			led_fan_off();
			

			// Clear the tap history
			count_sw_a = 0; swa_prev_val = 0;
			count_sw_c = 0; swc_prev_val = 0;
			
			// ---- Proceed to fan work
			time_val_led_start = millis();
			swb_prev_val = swb_val;

			switch (swb_val) {
				case 1:
					led_sig_on();
				break;
				
				case 2:
					led_sig_off();
				break;
				
				default:
					led_sig_off();
				break;
			}
		}

		if((millis() - time_val_led_start) > TIMER_VAL_LED) {
			led_sig_off();
		}
		// ------- End SIG LED control ---------------------------------


		//----------- Fan Control -----------------
		swc_val = get_sw_c();
		if(swc_val != swc_prev_val) {
	
			// --- Clear all other stuffs (heater/led)-----
			pwm_heater(PWM_HEATER_PERCENT_OFF);
			led_heater_off();
			led_sig_off();

			// Clear the tap history
			count_sw_a = 0; swa_prev_val = 0;
			count_sw_b = 0; swb_prev_val = 0;
	
			// ---- Proceed to fan work
			time_val_fan_start = millis();
			swc_prev_val = swc_val;

			switch (swc_val) {
				case 1:
					pwm_fan(PWM_FAN_PERCENT_33);
					led_fan_on();
				break;
		
				case 2:
					pwm_fan(PWM_FAN_PERCENT_66);
					led_fan_on();
				break;
		
				case 3:
					pwm_fan(PWM_FAN_PERCENT_100);
					led_fan_on();
				break;
		
				case 4:
					pwm_fan(PWM_FAN_PERCENT_OFF);
					led_fan_off();
					PORTC = 0x04;
				break;
		
				default:
					pwm_fan(PWM_FAN_PERCENT_OFF);
					led_fan_off();
					PORTC = 0x00;
				break;
			}
		}

		if((millis() - time_val_fan_start) > TIMER_VAL_FAN) {
			pwm_fan(PWM_FAN_PERCENT_OFF);
			led_fan_off();
		}
	// ------- End fan control ---------------------------------
	
	}
}

