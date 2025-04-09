#include <avr/io.h>
#include <avr/interrupt.h>

//f_clk = 20Mhz
//prescaler = 1024
//ftimer = 20MHz / 1024 = 19,531KHz
//T = value / ftimer


#define T1 255					// T = 255 / 19.5 = 13.07ms
#define T2 128					//  T = 128 / 19.5 = 6.56ms
#define T3 32					// T = 32 / 19.5 = 1.64ms

int state = 0;
/*
PF5 -> PEDESTRIAN BUTTON

(1 = RED 0 = GREEN)
P0 -> CAR LED
P1 -> TRAM SIMULATION
P2 -> PEDESTRIAN LED

T1 -> TRAM TIMER
T2 -> BUTTON TIMER
T3 -> BUTTON REACTIVATION TIMER
*/

int main() {
	PORTD.DIR |= 0b00000111; //PIN 0-2 is output
	
	//init LED (assume cars have priority)
	PORTD.OUTCLR = 0b00000001;
	PORTD.OUT |= 0b00000110;
	
	// pullup enable and Interrupt enabled with sense on rising edge (button release)
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;

	// split timer with prescaler 1024
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc | TCA_SPLIT_ENABLE_bm;
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

	// HIGH = BUTTON / REACTIVATION TIMER
	// LOW =  TRAM TIMER
	TCA0.SPLIT.LCNT = T2;
	TCA0.SPLIT.HCNT = T1;
	
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;	//Interrupt Enable for tram
	
	sei();										//begin accepting interrupt signals
	while (1) {
		;										//similar to a nop function
	}
}

// button pressed
ISR(PORTF_PORT_vect){
	PORTD.OUT |= 0b00000001;					//turn car led OFF (red light)
	PORTD.OUTCLR = 0b00000100;					//turn pedestrian led ON (green light)
	
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;	//clear flags
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LUNF_bm ;	//enable timer for lights
	
	
	PORTF.INTFLAGS |= PORT_INT5_bm;				//clear the interrupt flag
	PORTF.PIN5CTRL = PORT_ISC_INTDISABLE_gc;	//disable button interrupt
}

ISR(TCA0_HUNF_vect){
	PORTD.OUT |= 0b00000001;					//turn car led OFF (red light)
	PORTD.OUTCLR = 0b00000110;					//turn pedestrian/tram led ON (green light)
	
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;	//clear flags
	TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LUNF_bm ;	//enable timer for lights

	
	PORTF.PIN5CTRL = PORT_ISC_INTDISABLE_gc;	//disable button interrupt
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_HUNF_bm;	//clear flags
}

//pedestrian walk time ran out OR button timer ran out
ISR(TCA0_LUNF_vect){
	if(state == 0) {
		PORTD.OUT |= 0b00000110;					//turn pedestrian / tram led OFF (red light)
		PORTD.OUTCLR = 0b00000001;					//turn car led ON (green light)
		
		TCA0.SPLIT.LCNT = T3;						//start timer for 3
		state = 1;
	}
	else if(state == 1) {
		PORTF.PIN5CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;		//enable button
		TCA0.SPLIT.INTCTRL &= ~TCA_SPLIT_LCMP0_bm;					//disable timer
		state = 0;
	}
	TCA0.SPLIT.INTFLAGS |= TCA_SPLIT_LUNF_bm;						//clear flags
}
