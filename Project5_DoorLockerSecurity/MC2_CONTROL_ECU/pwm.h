/******************************************************************************
 * Module: PWM
 * File Name: pwm.h
 * Description: Header file for the AVR PWM TIMER0 driver
 * Author:	Yousif Adel
 *******************************************************************************/
#ifndef PWM_H_

#define PWM_H_
#include "std_types.h"
#include "common_macros.h"
/*******************************************************************************
 *                         Types Declaration                                   *
 *******************************************************************************/

#define PWM_PORT_ID					DDRB
#define PWM_PIN_ID					PB3


/*******************************************************************************
 *                              Functions Prototypes                         *
 *******************************************************************************/

/*
 * Description:
➢ The function responsible for trigger the Timer0 with the PWM Mode.
➢ Setup the PWM mode with Non-Inverting.
➢ Setup the prescaler with F_CPU/8.
 * F_PWM=(F_CPU)/(256*N) = (10^6)/(256*8) = 500Hz , Compare register , updating by duty cycle
➢ Setup the compare value based on the required input duty cycle
➢ Setup the direction for OC0 as output pin through the GPIO driver.
➢ The generated PWM signal frequency will be 500Hz to control the DC Motor speed.
 */
void PWM_Timer0_Start(uint8 duty_cycle);

#endif /*PWM_H_*/
