#include <avr/io.h>
#include <avr/interrupt.h>

// f_clk = 20MHz
// prescaler = 1024
// ftimer = 20MHz / 1024 = 19.531KHz
// T = value / ftimer

#define T2			39			// T = 39 / 19.5 = 2ms
#define T1			19.5		// T = 19.5 / 19.5 = 1ms
#define T3			58.5		// T = 58.5 / 19.5 = 3ms
#define threshold	25

volatile long int x = 0;             // Current X coordinate
volatile long int y = 0;             // Current Y coordinate

volatile int current_dir = 0;		// 0=north, 1=west, 2=south, 3=east
volatile int started = 0;			// Flag to check if movement has begun
volatile int returning = 0;

void turn_left() {
	PORTD.OUT = 0b00000011;
	current_dir = (current_dir + 1) % 4;			// Cycle: n ? w ? s ? e ? n
}

void turn_right() {
	PORTD.OUT = 0b00000110;
	current_dir = (current_dir - 1 + 4) % 4;		// Cycle: n ? e ? s ? w ? n
}

int main() {
	PORTD.DIR |= 0b00000111;					// PIN 0-2 is output
	

	PORTD.OUT |= 0b00000111;					// init LED (all off)
	
	// pullup enable and Interrupt enabled with sense on rising edge (button release)
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
	
	// split timer with prescaler 1024
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc | TCA_SPLIT_ENABLE_bm;
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;
	

	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc;			// 10 bit mode
	ADC0.CTRLA &= ~ADC_FREERUN_bm;				// Disable Free Running (Enable Single Mode)
	ADC0.CTRLA |= ADC_ENABLE_bm;				// Enable the ADC
	ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;			// Switch to Input 7
	ADC0.DBGCTRL |= ADC_DBGRUN_bm;				// Enable Debugging 
	
	ADC0.INTCTRL |= ADC_RESRDY_bm;				// Enable Single Conversion ISR
	
	ADC0.WINLT = threshold;						// Set threshold (for future use)
	ADC0.COMMAND = ADC_STCONV_bm;				// Start the conversion

	// Start T1 timer
	TCA0.SPLIT.HCNT = T1;
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
	sei();

	while (1) {
		if (y == 0 && x == 0 && started)		// Might need a small threshold for real world scenario
			break;
			
		if(	ADC0.MUXPOS == ADC_MUXPOS_AIN6_gc) {// Device moves while in front sensor mode
			cli();								// Disable interrupts to avoid race conditions (e.g. turning left while inside switch statement)
			if(started == 0) started = 1;
			
			PORTD.OUT = 0b00000101;				// Enable Front LED's
			switch (current_dir) {				// Increment coordinates
				case 0: y++; break;
				case 1: x--; break;
				case 2: y--; break;
				case 3: x++; break;
			}
			sei();								// Re enable Interrupts after incrementing coordinates
		}
	}
	PORTD.OUT = 0b00000111;						// Turn off the lights
	ADC0.CTRLA &= ~ADC_ENABLE_bm;				// Disable ADC
	cli();
}

// Right turn (left when returning)
ISR(ADC0_RESRDY_vect) {
	if (ADC0.RES > threshold) {					// if there isn't a wall on the left/right (returning/not), turn
		if (returning)			
			turn_left();
		else
			turn_right();
	}
	
	ADC0.INTFLAGS = ADC_RESRDY_bm;
}

// Left turn (right when returning)
ISR(ADC0_WCOMP_vect) {							// if this triggers it means there's a wall in front, so turn
	if (returning)
		turn_right();
	else
		turn_left();
	
	ADC0.INTFLAGS = ADC_WCMP_bm;
}

// Button pressed
ISR(PORTF_PORT_vect) {
	PORTD.OUTCLR = 0b00000111;					// turn on all the LEDs
	returning = 1;								// Toggle returning
	current_dir = (current_dir + 2) % 4;		// Turn Around
	
	
	TCA0.SPLIT.LCNT = T3;						// Start timer for LED lights		
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;	
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LUNF_bm;
	
	PORTF.INTFLAGS |= PORT_INT5_bm;
}

ISR(TCA0_HUNF_vect) {
	ADC0.INTCTRL &= ~ADC_RESRDY_bm;				// Disable RESRDY ISR
	
	ADC0.MUXPOS = ADC_MUXPOS_AIN6_gc;			// Switch to front sensor
		
	ADC0.CTRLA |= ADC_FREERUN_bm;				// Enable Free Running Mode
	ADC0.INTCTRL |= ADC_WCMP_bm;				// Enable WCMP ISR
	ADC0.CTRLE |= ADC_WINCM0_bm;				// Tells the ADC to use WCMP Mode
	
	ADC0.COMMAND |= ADC_STCONV_bm;				// Start Converting
		
	TCA0.SPLIT.LCNT = T2;						// Start T2
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LUNF_bm;
	
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_HUNF_bm;
}

ISR(TCA0_LUNF_vect) {
	//switch to side sensor
	if(!returning)
		ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;		// IN 7 = right sensor
	else
		ADC0.MUXPOS = ADC_MUXPOS_AIN5_gc;		// IN 5 = left sensor
		
	ADC0.CTRLA &= ~ADC_FREERUN_bm;				// Disable Free Running Mode (Enable Single Conversion)
	ADC0.INTCTRL |= ADC_RESRDY_bm;				// Enable RESRDY ISR (side sensor ISR)
	
	ADC0.INTCTRL &= ~ADC_WCMP_bm;				// Disable WCMP Interrupt
	ADC0.CTRLE &= ~ADC_WINCM0_bm;				// Disable Window Comparator Mode

	PORTD.OUT |= 0b00000111;					// Turn OFF LED's
	
	ADC0.COMMAND = ADC_STCONV_bm;				// Start Conversion
	
	TCA0.SPLIT.HCNT = T1;						// Start T1
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_HUNF_bm;
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
	
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;	// Clear Flags
}
