/*
 * heaterFanControl.c
 *
 * Created: 8/19/2017 12:33:59 AM
 * Author : LOMAS
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>


static uint16_t PWM_FREQ = 490;
static uint16_t TOP_VAL = 0;

/*static uint16_t top_value;*/

void pwm_init() {			
	// For Heater Control at PD6
	DDRD |= (1 << PORTD6);     // Set PD6 : Output
	PORTD &= ~(1 << PORTD6);
	// Initial TIMER0 Phase Correct PWM  
	// f_PWM = 16MHz / (64 * 510) = 490.196 Hz
	TCCR0A = 0b10000001; // Phase Correct PWM 8 Bit, Clear OCA0 on Compare Match, Set on TOP
	TCCR0B = 0b00000011; // Used 64 Prescaler

	TCNT0 = 0;           // Reset TCNT0
	OCR0A = 0;           // Initial the Output Compare register A & B 
	
	// *************************************************************
	// For Fan control at PPB1	
	
	DDRB |= (1 << PORTB1);
	PORTB &= ~(1 << PORTB1);
	// Initial TIMER1 Phase and Frequency Correct PWM
	// Set the Timer/Counter Control Register
	// TOP = (16000000/(2 * 64 * 490)) = 255.10
	TCCR1A = 0b11000000; // Set OC1A when up counting, Clear when down counting
	TCCR1B = 0b00010011; // Phase/Freq-correct PWM, top value = ICR1, Prescaler: 64
	
// 	ICR1H = 0x00;	 
// 	ICR1L = 0xFF;		// TOP value = 255

	TOP_VAL = (F_CPU / (128 * PWM_FREQ));
	
	ICR1 = TOP_VAL;	
	TCNT1 = 0;		
	OCR1A = 0;
	
// 	OCR1AH = 0;
// 	OCR1AL = 0;
	
	return;
}

void pwm_heater(uint8_t p) {
		
	/*33% PWM value -> 84*/
	/*66% 0f PWM value -> 166*/
	/*100% 0f pwm value -> 255*/
	
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
int main(void)
{
    /* Replace with your application code */
	pwm_init();
	pwm_fan(84);
	pwm_heater(84);
    while (1) 
    {
// 		pwm_fan(32);
// 		_delay_ms(5000);
// 		pwm_fan(64);
// 		_delay_ms(5000);
		/*pwm_fan(16384);*/
// 		_delay_ms(5000);
// 		pwm_fan(255);
// 		_delay_ms(5000);
		
    }
}

