/*
 * timer.c
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#include <avr/interrupt.h>
#include "global.h"
#include "timer.h"
#include <stdio.h>

volatile unsigned long Timer0Reg0;

unsigned long lbolt;

void timerInit(void)
{
	// initialize all timers
	timer0_initialize();

}



//------------------------------------------------------------------------------------
// timer_initialize() -- make Timer0 to go @100ms period.
//
// Setup the Timer Counter 0 Interrupt
void timer0_initialize( void )
{
//
	// initialize timer 0
		timer0SetPrescaler( TIMER_CLK_DIV1024 );	// set prescaler
		TCNT0= 0;							// reset TCNT0
		sbi(TIMSK, TOIE0);						// enable TCNT0 overflow interrupt

		timer0_ClearOverflowCount();				// initialize time registers
} // timer_initialize

void timer0SetPrescaler(uint8_t prescale)
{
	// set prescaler on timer 0
	TCCR0= (((TCCR0) & ~TIMER_PRESCALE_MASK) | prescale);

}


void timer0_ClearOverflowCount(void)
{
	// clear the timer overflow counter registers
	Timer0Reg0 = 0;	// initialize time registers
}

//! Interrupt handler for tcnt0 overflow interrupt
SIGNAL(SIG_OVERFLOW0)
{
	//Timer0Reg0++;			// increment low-order counter
	lbolt++;

}
