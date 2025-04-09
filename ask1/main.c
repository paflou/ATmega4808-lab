#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int main() {
	PORTD.DIR |= 0b00000111; //BITS 0-2 OUTPUT (0 = ERR)
	PORTD.OUT |= 0b00000111; //LED's are off
	
	// pullup enable and Interrupt enabled with sense on rising edge (button release)
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
	PORTF.PIN6CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
	sei(); //enable interrupts
	
	while (1) {
		printf("."); // print statement for easy debugging (breakpoint at each cycle)
	}
}

ISR(PORTF_PORT_vect){
	int y = PORTF.INTFLAGS;
	
	// Both buttons pressed (error)
	if(y == 0b01100000) {
		PORTD.OUTCLR = 0b00000001;
		_delay_ms(10);
		PORTD.OUT |= 0b00000001;

	}
	// Down button (PF5)
	else if (y == 0b00100000) {
		// if not at ground floor
		if (PORTD.OUT != 0b00000111) {
			// ground floor ? LED2 OFF : LED1 && LED2 OFF
			PORTD.OUT |= (PORTD.OUT == 0b00000001)? 0b00000100 : 0b00000110;
		}
	}
	// Up button (PF6)
	else if (y == 0b01000000) {
		// if not at 2nd floor
		if (PORTD.OUT != 0b00000001) {
			// 2nd floor ? LED1 ON : LED1 && LED2 ON
			PORTD.OUTCLR = (PORTD.OUT == 0b00000111)? 0b00000010 : 0b00000110;
		}
	}

	PORTF.INTFLAGS = y; //clear the interrupt flag
}
