/*
 * heaterFanControl.c
 *
 * Created: 8/19/2017 12:33:59 AM
 * Author : LOMAS
 */ 

#include <avr/io.h>
#define F_CPU 1000000UL		// Internal 1MHz Osc
#include <util/delay.h>
#include <avr/interrupt.h>


#define		SW_C			PIND2
#define		SW_B			PIND1
#define		SW_A			PIND0

#define		F_LED_CNTL		PORTD3
#define		H_LED_CNTL		PORTD4

#define		IO_PORTD_OUT	PORTD
#define		IO_PORTD_IN		PIND
#define		IO_PORTD_DIR	DDRD

#define		LED_SIG			PORTB2
#define		IO_PORTB_OUT	PORTB
#define		IO_PORTB_DIR	DDRB

#define		ANALOG_TMP_PIN	7

#define		PWM_FAN_PERCENT_LOW			7		// 33%
#define		PWM_FAN_PERCENT_MID			13		// 66%
#define		PWM_FAN_PERCENT_HIGH		20		// 100%

#define		PWM_HEATER_PERCENT_LOW		51		// 20%
#define		PWM_HEATER_PERCENT_MID		102		// 40%
#define		PWM_HEATER_PERCENT_HIGH		153		// 60%

#define		PWM_HEATER_PERCENT_OFF		0
#define		PWM_FAN_PERCENT_OFF			0

#define		TEMP_LOW_VAL				30.0F
#define		TEMP_MID_VAL				50.0F
#define		TEMP_HIGH_VAL				70.0F
#define		TEMP_CRITICAL_VAL			80.0F

#define		HIGH						1
#define		LOW							0

#if 0
	// 120 * 1000 (120 Seconds)
	#define		SENSOR_CHECK_TIME			120000UL			

	// 120 * 60 * 1000 (in Millisecond) -> 120 Minutes
	#define		TIMER_VAL_HEATER			7200000UL	

	// 120 * 60 * 1000 (in Millisecond) -> 120 Minutes
	#define		TIMER_VAL_FAN				7200000UL	

	// 30 * 60 * 1000  (in Millisecond) -> 30 Minutes
	#define		TIMER_VAL_LED				1800000UL	
#endif


// Dummy Time value during test
#if 1
	#define		SENSOR_CHECK_TIME			5000UL
	#define		TIMER_VAL_HEATER			50000UL
	#define		TIMER_VAL_FAN				30000UL
	#define		TIMER_VAL_LED				30000UL
#endif

	
static uint16_t PWM_FREQ = 25000;
static uint16_t TOP_VAL = 0;

volatile uint8_t count_sw_a = 0;
volatile uint8_t count_sw_b = 0;
volatile uint8_t count_sw_c = 0;

volatile uint8_t flag_check_sensor = 0;

volatile unsigned long milliseconds;

ISR(TIMER2_COMPA_vect) {
	++milliseconds;
	if((milliseconds % SENSOR_CHECK_TIME) == 0)
		flag_check_sensor = 1;
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
	// f_PWM = 1MHz / (256 * 510) = 7.65931 Hz
	TCCR0A = 0b10000001; // Phase Correct PWM 8 Bit, Clear OCA0 on Compare Match, Set on TOP
	TCCR0B = 0b00000100; // Used 256 Prescaler

	TCNT0 = 0;           // Reset TCNT0
	OCR0A = 0;           // Initial the Output Compare register A & B 
	
	
	/*********************************************/
	/*		For Fan control at PPB1				 */
	/*********************************************/		
	DDRB |= (1 << PORTB1);
	PORTB |= (1 << PORTB1);
	// Initial TIMER1 Phase and Frequency Correct PWM
	// Set the Timer/Counter Control Register
	// TOP = (1000000/(2 * 1 * 25000)) = 20
	TCCR1A = 0b11110000; // Clear OC1A when up counting, Set when down counting
	TCCR1B = 0b00010001; // Phase/Freq-correct PWM, top value = ICR1, Prescaler: 1
	
	TOP_VAL = (F_CPU / (2 * PWM_FREQ));
	
	ICR1 = TOP_VAL;	
	TCNT1 = 0;		
	OCR1A = 0;	
	return;
}

void init_io() {
	// Initialize IOs at PORT D
	IO_PORTD_DIR |= (1 << F_LED_CNTL) | (1 << H_LED_CNTL);
	IO_PORTD_OUT &= ~((1 << F_LED_CNTL) | (1 << H_LED_CNTL));
	
	// Make Switch Pins Input
	IO_PORTD_DIR &= ~((1 << SW_A) | (1 << SW_B) | (1 << SW_C));
	
	// Enable internal pull ups
	IO_PORTD_OUT |= ((1 << SW_A) | (1 << SW_B) | (1 << SW_C));

	// Initialize IO at port B
	IO_PORTB_DIR |= (1 << LED_SIG);
	IO_PORTB_OUT &= ~(1 << LED_SIG);
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

float get_temp() {
	float temperature = 0.0;
	// Start conversion
	ADCSRA |= (1 << ADSC);
	
	// Wait until the conversion completes 
	while (ADCSRA & (1 << ADSC));

	// Temperature calculation goes follows
	// 1 Degree Centigrade = 2.049 steps 
	temperature = ADC / 2.049;

	return temperature;
}

uint8_t sw_a() {
	if(IO_PORTD_IN & (1 << SW_A)) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t get_sw_a() {
		static uint8_t switch_state = LOW;
		static uint8_t switch_state_prev = LOW;
		
		switch_state = sw_a();
		
		_delay_ms(20);  // De-bounce Time
		
		if(((switch_state == HIGH) && (switch_state_prev == LOW)) || ((switch_state == LOW) && (switch_state_prev == HIGH)))
		{
			switch_state_prev = switch_state;
			count_sw_a++;	// Increment the counter
		}
		
		if(count_sw_a > 4)	count_sw_a = 1; // Initial Value is 1
		
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
	
	static uint8_t switch_state = LOW;
	static uint8_t switch_state_prev = LOW;
	
	switch_state = sw_c();
	
	_delay_ms(20);  // De-bounce Time
	
	if(((switch_state == HIGH) && (switch_state_prev == LOW)) || ((switch_state == LOW) && (switch_state_prev == HIGH)))
	{
		switch_state_prev = switch_state;
		count_sw_c++;	// Increment the counter
	}
	
	if(count_sw_c > 4)	count_sw_c = 1; // Initial Value is 1
	
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

int main(void) {    
	
	init_timerModule();
	init_adc();
	init_io();
	pwm_init();
	sei();
	
	uint8_t swa_val = 0;
	uint8_t swa_prev_val = 0;

	uint8_t swb_val = 0;
	uint8_t swb_prev_val = 0;

	uint8_t swc_val = 0;
	uint8_t swc_prev_val = 0;

	unsigned long time_val_heater_start = 0;
	unsigned long time_val_fan_start = 0;
	unsigned long time_val_led_start = 0;
	
	float temp_avg  = 0.0;
	double temp_sum = 0.0;

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
					pwm_heater(PWM_HEATER_PERCENT_LOW);
					led_heater_on();
				break;
				
				case 2:
					pwm_heater(PWM_HEATER_PERCENT_MID);
					led_heater_on();
				break;
				
				case 3:
					pwm_heater(PWM_HEATER_PERCENT_HIGH);
					led_heater_on();
				break;
				
				case 4:
					pwm_heater(PWM_HEATER_PERCENT_OFF);
					led_heater_off();
				break;
				
				default:
					pwm_heater(PWM_HEATER_PERCENT_OFF);
					led_heater_off();
				break;
			}
		}

		if((millis() - time_val_heater_start) > TIMER_VAL_HEATER) {
			pwm_heater(PWM_HEATER_PERCENT_OFF);
			led_heater_off();
			count_sw_a = 0; swa_prev_val = 0;
		}
		// ------- End Heater Control -------------
	

		//----------- LED Control -----------------
		swb_val = get_sw_b();
		
		if(swb_val != swb_prev_val) {
			
			// --- Clear all other stuffs (heater/fan)-----
			pwm_heater(PWM_HEATER_PERCENT_OFF);
			led_heater_off();
			
			pwm_fan(PWM_FAN_PERCENT_OFF);
			led_fan_off();
			

			// Clear the tap history
			count_sw_a = 0; swa_prev_val = 0;
			count_sw_c = 0; swc_prev_val = 0;
			
			// ---- Proceed to LED work
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
			count_sw_b = 0; swb_prev_val = 0;
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
					pwm_fan(PWM_FAN_PERCENT_LOW);
					led_fan_on();
				break;
		
				case 2:
					pwm_fan(PWM_FAN_PERCENT_MID);
					led_fan_on();
				break;
		
				case 3:
					pwm_fan(PWM_FAN_PERCENT_HIGH);
					led_fan_on();
				break;
		
				case 4:
					pwm_fan(PWM_FAN_PERCENT_OFF);
					led_fan_off();
				break;
		
				default:
					pwm_fan(PWM_FAN_PERCENT_OFF);
					led_fan_off();
				break;
			}
		}

		if((millis() - time_val_fan_start) > TIMER_VAL_FAN) {
			pwm_fan(PWM_FAN_PERCENT_OFF);
			led_fan_off();
			count_sw_c = 0; swc_prev_val = 0;
		}
		// ------- End fan control ---------------------------------
	
		
		// --------------- Temperature sensing and Control ----------
		// ---- Check sensor each 120 seconds -------------------------
		if(flag_check_sensor) {
			flag_check_sensor = 0;
			
			for(uint8_t i = 0; i < 5; i++) {
				temp_sum += get_temp();
			}

			temp_avg = temp_sum / 5;
			
			temp_sum = 0.0;

			if(temp_avg < TEMP_CRITICAL_VAL) {

				switch (swa_val) {

					case 1:
						if(temp_avg >= TEMP_LOW_VAL) {
							pwm_heater(PWM_HEATER_PERCENT_OFF);
							led_heater_off();
						} else {
							pwm_heater(PWM_HEATER_PERCENT_LOW);
							led_heater_on();
						}
					break;
						
					case 2:
						if(temp_avg >= TEMP_MID_VAL) {
							pwm_heater(PWM_HEATER_PERCENT_OFF);
							led_heater_off();
						} else {
							pwm_heater(PWM_HEATER_PERCENT_MID);
							led_heater_on();
						}
					break;
						
					case 3:
						if(temp_avg >= TEMP_HIGH_VAL) {
							pwm_heater(PWM_HEATER_PERCENT_OFF);
							led_heater_off();
						} else {
							pwm_heater(PWM_HEATER_PERCENT_HIGH);
							led_heater_on();
						}
					break;
						
					case 4:
						pwm_heater(PWM_HEATER_PERCENT_OFF);
						led_heater_off();
					break;
						
					default:
						pwm_heater(PWM_HEATER_PERCENT_OFF);
						led_heater_off();
					break;
				}
			} else {
				pwm_heater(PWM_HEATER_PERCENT_OFF);
				led_heater_off();
			}
		} 
	}
	return 0;
}

