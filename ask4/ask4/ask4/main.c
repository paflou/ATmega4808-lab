#include <avr/io.h>
#include <avr/interrupt.h>


// LED 0 = blade movement
// LED 1 = rotation movement
int point = 0;


void enable() {
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LUNF_bm;		// Clear any interrupts
	// TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0_bm;							// Enable PWM output compare
	TCA0.SPLIT.INTCTRL = TCA_SPLIT_LUNF_bm /*| TCA_SPLIT_LCMP0_bm */;	// Enable interrupts for underflow
	TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;							// Enable the timer
}

int main(void)
{
	PORTD.DIR |= PIN0_bm | PIN1_bm;										// Set PIN 0,1 as output
	PORTD.OUT |= PIN0_bm | PIN1_bm;										// Initialize both LEDs OFF

	
	PORTF.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;			// Configure button on PIN 6 PORTF	
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;								// Enable TCA Split Mode
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV256_gc;						// Configure clock source, not enabled
	sei();																// Enable interrupts globally
	while (1)
	{
	}

	cli();																// Disable interrupts (will never be reached)
}

// Button Press ISR
ISR(PORTF_PORT_vect) {
	// Clear the interrupt flag
	int y = PORTF.INTFLAGS;
	PORTF.INTFLAGS = y;

	TCA0.SPLIT.CTRLA &= ~TCA_SPLIT_ENABLE_bm;							// Disable timer for configuration
	
	// f_clk = 20MHz
	// prescaler = 256
	// ftimer = 20MHz / 256 = 38,022KHz ~= 38KHz
	// Tb = 2ms
	// fpwm_ss = (f_clk / N ) / (PER + 1) = ftimer / (PER + 1)
	
	// HPER :
	// HPER + 1 = ftimer / fpwm_ss;
	// HPER + 1 = 38 * 2
	// HPER = 75
	
	// LPER :
	// LPER + 1 = ftimer / fpwm_ss;
	// LPER + 1 = 38 * 1
	// LPER = 37
	switch (point) {
		case 0:
			// blade slow (60%) with 1ms period
			TCA0.SPLIT.HPER = 75;        			// High period
			TCA0.SPLIT.HCMP0 = 30;       			// 40% duty cycle
			
			TCA0.SPLIT.LPER = 37;        			// Low period
			TCA0.SPLIT.LCMP0 = 22;       			// 60% duty cycle
		
			TCA0.SPLIT.LCNT = TCA0.SPLIT.LPER;		// Restart low counter
			TCA0.SPLIT.HCNT = TCA0.SPLIT.HPER;		// Restart high counter
			PORTD.OUT ^= PIN0_bm;        			// Toggle LED0
			PORTD.OUT ^= PIN1_bm;        			// Toggle LED1
			
			enable();
			break;
		case 1:
			// blade fast (50%) with 0.5ms period
			// LPER' = LPER / 2 = 18.5 ~= 19
			TCA0.SPLIT.LPER = 19;        			// Low period
			TCA0.SPLIT.LCMP0 = 10;        			// 50% duty cycle
			TCA0.SPLIT.LCNT = TCA0.SPLIT.LPER;  	// Restart low counter
			TCA0.SPLIT.HCNT = TCA0.SPLIT.HPER;  	// Restart high counter
			
			enable();
			break;
		case 2:
			
			PORTD.OUT |= PIN0_bm | PIN1_bm;				// Turn off LEDs
			TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LUNF_bm;	// Disable interrupt for underflow
			break;
	}
	
	point++;
	point %= 3;															// Loop through 0, 1, 2
}

// Low counter underflow ISR
ISR(TCA0_LUNF_vect) {
	PORTD.OUT ^= PIN0_bm;												// Toggle PORTD0 (LED0)
	
	if(TCA0.SPLIT.INTFLAGS & TCA_SPLIT_HUNF_bm ) {
		PORTD.OUT ^= PIN1_bm;											// Toggle PORTD1 (LED1)
		TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm; 						// Clear the interrupt flag for high counter underflow

	}

	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LUNF_bm; 							// Clear the interrupt flag for low counter underflow
}





//	Alternative solution (better scalability, slightly worse time)
//	Implementation			->	LUNF to HUNF delay ~= 1?s
//	Alternative solution	->	LUNF to HUNF delay ~= 50?s
//	Alternative solution has better scalability because each ISR takes up less time,
//	so the system can respond quicker to other interrupts if needed
//	and it makes the code easier to understand

//	For this to work we would also need to enable the HUNF interrupt
/* 
// High counter underflow ISR
ISR(TCA0_HUNF_vect) {
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;							// Clear the interrupt flag for high counter underflow
	PORTD.OUT ^= PIN1_bm;												// Toggle PORTD1 (LED1)
}
*/


//	We would do this if we actually used the duty cycle
//	which we can't do for HUNF using TCA0 in split mode
//	due to it not having an equivalent TCA0_HCMP0_vect.
//	The changes required for this solution are commented out

/*
ISR(TCA0_LCMP0_vect) {
	// Clear the interrupt flag for low counter underflow
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LCMP0_bm;
	PORTD.OUT ^= PIN0_bm;  // Toggle PORTD0 (LED0)
}
*/
