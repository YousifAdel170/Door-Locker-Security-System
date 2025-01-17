/******************************************************************************
 * Module: Buzzer
 * File Name: buzzer.c
 * Description: Source file for the Buzzer driver
 * Author:	Yousif Adel
 *******************************************************************************/
#include	"buzzer.h"

/*******************************************************************************
 *                              Functions Definitions                         *
 *******************************************************************************/

/*
 ● Description
	⮚ Setup the direction for the buzzer pin as output pin through the GPIO driver.
	⮚ Turn off the buzzer through the GPIO.
● Inputs: None
● Return: None
*/
void Buzzer_init(void)
{
	/*Setup the direction for the buzzer pin as output pin*/
	GPIO_setupPinDirection(BUZZER_PORT_ID, BUZZER_PIN_ID, PIN_OUTPUT);

	/* Turn off the buzzer */
	GPIO_writePin(BUZZER_PORT_ID, BUZZER_PIN_ID, BUZZER_OFF);
}


/*
● Description
	⮚ Function to enable the Buzzer through the GPIO.
● Inputs: None
● Return: None
*/
void Buzzer_on(void)
{
	/* Turn ON the buzzer */
	GPIO_writePin(BUZZER_PORT_ID, BUZZER_PIN_ID, BUZZER_ON);
}


/*
● Description
	⮚ Function to disable the Buzzer through the GPIO.
● Inputs: None
● Return: No
*/
void Buzzer_off(void)
{
	/* Turn OFF the buzzer */
	GPIO_writePin(BUZZER_PORT_ID, BUZZER_PIN_ID, BUZZER_OFF);
}
