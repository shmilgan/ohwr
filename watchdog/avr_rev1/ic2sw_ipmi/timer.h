/*
 * timer.h
 *
 *  Created on: 07-Jul-2009
 *      Author: poliveir
 */

#ifndef TIMER_H_
#define TIMER_H_

#define TIMER_CLK_STOP			0x00	///< Timer Stopped
#define TIMER_CLK_DIV1			0x01	///< Timer clocked at F_CPU
#define TIMER_CLK_DIV8			0x02	///< Timer clocked at F_CPU/8
#define TIMER_CLK_DIV32			0x03	///< Timer clocked at F_CPU/32
#define TIMER_CLK_DIV64			0x04	///< Timer clocked at F_CPU/64
#define TIMER_CLK_DIV128		0x05	///< Timer clocked at F_CPU/128
#define TIMER_CLK_DIV256		0x06	///< Timer clocked at F_CPU/256
#define TIMER_CLK_DIV1024		0x07	///< Timer clocked at F_CPU/1024
#define TIMER_PRESCALE_MASK		0x07	///< Timer Prescaler Bit-Mask

// default prescale settings for the timers
// these settings are applied when you call
// timerInit or any of the timer<x>Init
#define TIMER0PRESCALE		TIMER_CLK_DIV8		///< timer 0 prescaler default
#define TIMER1PRESCALE		TIMER_CLK_DIV64		///< timer 1 prescaler default
#define TIMER2PRESCALE		TIMERRTC_CLK_DIV64	///< timer 2 prescaler default

void timerInit(void);
void timer0_initialize(void);
void timer0_ClearOverflowCount(void);
void timer0SetPrescaler(uint8_t prescale);

#endif /* TIMER_H_ */
