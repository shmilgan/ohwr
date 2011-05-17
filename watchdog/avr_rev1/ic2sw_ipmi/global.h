/*
 * global.h
 *
 *  Created on: 16-Jun-2009
 *      Author: poliveir
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_


#ifndef cbi
	#define cbi(reg,bit)	reg &= ~(_BV(bit))
#endif
#ifndef sbi
	#define sbi(reg,bit)	reg |= (_BV(bit))
#endif

#define toogle_gpio(PORT , PIN ) PORT ^= _BV(PIN)
#define clr_gpio(PORT , PIN ) PORT &= _BV(PIN)
#define set_gpio(PORT , PIN ) PORT |= _BV(PIN)

extern volatile unsigned long Timer0Reg0;
#define MCH_LED_RED       PB4
#define MCH_LED_YEL       PB5


#endif /* GLOBAL_H_ */
