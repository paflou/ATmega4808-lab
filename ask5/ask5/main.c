#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

void init_ADC();

uint8_t pwm_counter = 0;
uint8_t res;


int main(){
	PORTD.DIR |= PIN1_bm									//PIN 0-2 is output
	| PIN0_bm
	| PIN2_bm;

	PORTD.OUT |= PIN1_bm									//PIN 0-2 initialize OFF
	| PIN0_bm
	| PIN2_bm;

	PORTF.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc; //configure button 6
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc; //configure button 5

	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc;		// Configure clock source, not enabled
	
	init_ADC();

	sei();													//enable interrupts
	ADC0.COMMAND |= ADC_STCONV_bm;							//Start Conversion

	while(1){
		;
	}
}

ISR(ADC0_WCOMP_vect){
	cli();
	res = ADC0.RES;
	ADC0.INTFLAGS |= ADC_WCMP_bm;
	ADC0.CTRLA &= ~ADC_ENABLE_bm;							// Disable ADC so we can handle the issue
	
	if (res < ADC0.WINLT) {									// Below lower threshold (need watering)
		PORTD.OUTCLR = PIN0_bm;								// Turn LED0 on
	}
	else if (res > ADC0.WINHT) {							// Above upper threshold (too humid)
		PORTD.OUTCLR = PIN1_bm;								// Turn LED1 on
	}
	sei();
}

ISR(PORTF_PORT_vect) {
	cli();
	int flags = PORTF.INTFLAGS;												// Read once

	// set watering
	if( (flags & PIN5_bm ) && (PORTD.OUT == 0b00000110) && !(flags & PIN6_bm) ) {
		TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm;							// Clear possible interrupt
		TCA0.SINGLE.CTRLB &= ~TCA_SINGLE_WGMODE_SINGLESLOPE_gc;				// Disable PWM
		TCA0.SINGLE.CNT = 0;
		TCA0.SINGLE.PER = 0xFF;												// Big resolution

		TCA0.SINGLE.CMP1 = ADC0.WINLT - ADC0.RES;							// configure Timer
		TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;							// Enable interrupt for the timer

		TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;							// Start the timer
		PORTF.INTFLAGS |= PIN5_bm;											// Clear the interrupt flag
	}
	// set air
	else if( (flags & PIN6_bm) && (PORTD.OUT == 0b00000101) && !(flags & PIN5_bm)) {
		TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
		TCA0.SINGLE.PER = 20;												// Resolution (LPER+1)/ftimer = 1ms -> LPER = 19.5 ~= 20
		TCA0.SINGLE.CMP0 = 10;												// Duty Cycle 50%

		TCA0.SINGLE.CTRLB |= TCA_SINGLE_WGMODE_SINGLESLOPE_gc;				// Single-slope PWM
		TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;							// Enable interrupt for bit toggling
		
		TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;							// Start PWM
		// TCA0.SINGLE.INTCTRL |= TCA_SINGLE_CMP0_bm;						// Enable interrupt for handling if needed
		
		PORTF.INTFLAGS |= PIN6_bm;											// Clear the interrupt flag
	}
	else {
		PORTD.OUTCLR |= PIN1_bm | PIN0_bm| PIN2_bm;							//turn on ALL LEDs to indicate wrong handling
		printf(".");														// give it time to clear

		PORTD.OUT |= PIN2_bm;
		PORTD.OUT |= PIN1_bm;
		PORTD.OUT |= PIN0_bm;												//turn off ALL LEDs step by step
		PORTF.INTFLAGS |= PIN5_bm | PIN6_bm;								// Clear the interrupt flag
		
		ADC0.CTRLA |= ADC_ENABLE_bm;										// Enable ADC
		ADC0.COMMAND |= ADC_STCONV_bm;										//Start Conversion
	}
	sei();
}

ISR(TCA0_CMP1_vect) {
	cli();
	TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;				// Disable TCA0

	PORTD.OUTSET = PIN0_bm;									// Turn off LED 0
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm;				// Clear interrupt flag

	ADC0.CTRLA |= ADC_ENABLE_bm;							// Enable ADC
	ADC0.COMMAND |= ADC_STCONV_bm;							//Start Conversion
	
	sei();
}

ISR(TCA0_OVF_vect) {
	cli();
	PORTD.OUT ^= PIN2_bm;							// Toggle LED 2

	if(pwm_counter == 3) {
		PORTD.OUTSET = PIN1_bm;						// Turn off LED 1
		PORTD.OUTSET = PIN2_bm;						// Turn off LED 2
		pwm_counter = 0;							// Reset count
		TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;	// Disable TCA0
		ADC0.CTRLA |= ADC_ENABLE_bm;				// Enable ADC
		ADC0.COMMAND |= ADC_STCONV_bm;				// Start Conversion
		} else {
		pwm_counter++;
	}

	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;		// Clear interrupt flag
	sei();
}

void init_ADC() {
	//initialize the ADC for Free-Running mode
	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc;					// 10-bit resolution
	ADC0.CTRLA |= ADC_FREERUN_bm;						// Free-Running mode enabled
	ADC0.CTRLA |= ADC_ENABLE_bm;						// Enable ADC
	ADC0.MUXPOS |= ADC_MUXPOS_AIN7_gc;					// PD7
	ADC0.DBGCTRL |= ADC_DBGRUN_bm;						// Enable Debug Mode

	//Window Comparator Mode
	ADC0.WINLT = 50;									// Set low threshold
	ADC0.WINHT = 200;									// Set high threshold
	ADC0.INTCTRL |= ADC_WCMP_bm;						// Enable Interrupts for WCM
	ADC0.CTRLE |= ADC_WINCM_OUTSIDE_gc;					// Interrupt when RESULT out of bounds
}