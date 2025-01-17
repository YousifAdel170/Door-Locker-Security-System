/******************************************************************************
 * File Name: hmi_ecu_MC1.c
 * Description: (Human Machine Interface) in the Door Locker Security System
 * Author: Yousif Adel
 *******************************************************************************/

/*******************************************************************************
 *                                Header Files                                  *
 *******************************************************************************/
#include	"lcd.h"
#include	"keypad.h"
#include	"uart.h"
#include	"timer1.h"
#include	<util/delay.h>

/*******************************************************************************
 *                                Definitions                                  *
 *******************************************************************************/
#define ZERO							0

#ifdef TIMER1_COMPARE
#define COMPARE_VALUE					7812
#endif

/* UART Values Between MC1 & MC2 */
#define MC1_READY 						0x01	// MC1 is ready
#define MC2_READY 						0x02	// MC2 is ready

#define CHANGE_PASSWORD					0x13	// change the password
#define SAVE_PASSWORD 					0x04	// save the password
#define PASSWORD_SAVED					0x05	// password has been saved
#define UNLOCK_THE_DOOR					0x12	// open the door [door sequence]
#define BUZZER_ON_BYTE					0x11	// turn the buzzer on	[buzzer sequence]
#define SEND_PASSWORD_TO_BE_CHECKED 	0x06	// send the password to be checked
#define PASSWORD_CHECK					0x12	// check if the password correct
#define PASSWORD_DOESNT_MATCH			0x08	// password WRONG
#define PASSWORD_MATCH					0x09	// password correct
#define BEGGINING_OF_EEPROM_ADDRESS		0x70	// the first address of the password at EEPROM

/* Password Configurations */
#define PASSWORD_SIZE					5		// Size of the password
#define WRONG_PASSWORD					0		// Indicates that is a wrong password
#define RIGHT_PASSWORD					1		// Indicates that is a correct password

#define MAX_NO_OF_WRONG_TIMES			3		// Maximum no of wrong times before buzzer turned ON

/* Main Options Configurations*/
#define OPEN_DOOR_OPTION				'+'		// Option that enable Opening the door
#define CHANGE_PASSWORD_OPTION			'-'		// Option that change the password of the system

/* Password Symbols */
#define SUBMIT_PASSWORD					'='		// Indicates that the user want to submit the password
#define PASSWORD_MARK					'*'		// Password mark that appears on LCD

/* Door Times & Error */
#define UNLOCK_DOOR_TIME				15
#define OPEN_DOOR_TIME					3
#define LOCK_DOOR_TIME					15
#define ERROR_TIME						60

/* Delays Configurations */
#define LCD_DISPLAY_DELAY				1000
#define KEY_BUTTON_DELAY				500
/******************************************************************************/

/*******************************************************************************
 *                           Global Variables                                  *
 *******************************************************************************/
static uint8 volatile g_tick = 0;		/* to count ticks */

uint8 g_flagPassword; // to store the response

/*******************************************************************************
 *                           Structure Configurations                          *
 *******************************************************************************/
/* Set The UART Configurations */
UART_ConfigType UART_Configurations = {EIGHT_BITS, DISABLED, ONE_BIT, BD_9600, ASYNCHRONOUS, ASYNCHRONOUS_DOUBLE_SPEED};

/*
 * create configuration for timer to get interrupt every 1 second
 * To calculate 1second -> F = 1Hz
    	F = F(CPU)/N(1+x) = 8Mhz/ N(1+x) try different pre-scalar
    	N = 64  , x = 124999
    	N = 256 , x = 31249
    	N = 1024 , x = 7812
    so the better is N = 1024
 * Set The Timer 1 Configurations
 */
Timer1_ConfigType Timer1_Configuration = {ZERO, COMPARE_VALUE, F_CPU_1024, CTC_OCR1A};
/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/
/*
 * Description:
 * This is the call back function called by the Timer 1 driver.
 * It is used to calculate the required time for each event.
 */
void Timer_callBack(void);

/*
 * Description:
 * Function that view options [+,-] and process your choice
 * 	- if you press '+' this means the door will go through 3 steps
 	 	 1. Door Unlocking 15 seconds
 	 	 2. Door Open 3 seconds
 	 	 3. Door Locking 15 seconds
 * 	- if you press '-' this means that u will change the password
 */
void mainOptions(void);

/*
 * Description: [create new password]
	Function to allow you to insert password twice and
	check if the two password are matched or not*
 */
void createAndCheckPassword(void);

/*
 * Description:
	Function to check whether the password correct or wrong
 */
void checkPasswordAfterCreation(void);

/*
 * Description:
 * Function that after choosing the option +
 * and entered the password 3 times wrong
 	 	 It display ERROR for 60 seconds
 */
void buzzerSequence(void);
/*
 * Description:
 * Function that after choosing the option +
 * and entered the password correctly
 	 	 1. Door Unlocking 15 seconds
 	 	 2. Door Open 3 seconds
 	 	 3. Door Locking 15 seconds
 */
void doorSequence(void);

/*Description: Function to save the  password
 * Inputs:
	1. array: to store the password
	2. size: to know the size of the password [size of the array]
	3. Key: values that take from the keypad
 */
void savePassword(uint8 *arr_pass , uint8 arr_size, uint8 key_num);

/*******************************************************************************************************/
int main(void)
{
	/*Enable I-bit = 1*/
	S_REG.Bits.I_Bit = 1;

	/* initialize LCD driver*/
	LCD_init();

	/*initiate UART driver*/
	UART_init(&UART_Configurations);

	/*send byte to MC2 to tell him that MC1 is ready*/
	UART_sendByte(MC1_READY);

	/* initialize LCD driver*/
	Timer1_init(&Timer1_Configuration);
	/* set the call back to pointer in the Timer 1 */
	Timer1_setCallBack(Timer_callBack);


	createAndCheckPassword();

	while(1)
	{
		mainOptions();
	}

}
/*********************************************************************************************************/

/*
 * Description:
	Function to allow you to insert password twice and check if the two password are matched or not*
 */
void createAndCheckPassword(void)
{

	/* variable to get the pressed key */
	uint8 key;

	/*array to get the first password */
	uint8 arr_pass[PASSWORD_SIZE];

	/*array to get the confirmation password */
	uint8 arr_confirm_pass[PASSWORD_SIZE];

	/* variable to count from 0 to password size*/
	uint8 passCounter = 0;

	/* variable to check the state of the password whether right or wrong */
	uint8 password_checking_state = RIGHT_PASSWORD;

	/* Clear LCD & display Enter Pass*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("plz enter pass: ");
	LCD_moveCursor(1,0);

	/* save the entered Password*/
	savePassword(arr_pass,PASSWORD_SIZE,key);


	/* Clear LCD & display re-enter Pass*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("plz re-enter");
	LCD_moveCursor(1,0);
	LCD_displayString("pass:");

	/*
		- Enter password of PASSWORD_SIZE characters using keypad
	  	- save the re-entered password , confirmation
	*/
	savePassword(arr_confirm_pass,PASSWORD_SIZE,key);

	/* check if the two arrays of password are identical or not*/
	/* Check whether the two password are identical or not while creating new password
	 * Inputs :
			1. array contains the values of first password
			2. array contains the values of second password
			3. size: to know the size of the password [size of the array]
			4. state: check whether both matched or not
	 */
	while(passCounter < PASSWORD_SIZE)
	{
		if(arr_pass[passCounter] != arr_confirm_pass[passCounter])
		{
			/* if any character is different then both are not identical then it is not matched*/
			password_checking_state = WRONG_PASSWORD;
			break;
		}
		passCounter++;
	}

	if(password_checking_state == RIGHT_PASSWORD)
	{
		/* since the password is identical in both array we need to save it in the EEPROM*/
		UART_sendByte(SAVE_PASSWORD);
		_delay_ms(10);

		/* [send the array]send the password */
		for(passCounter = 0; passCounter < PASSWORD_SIZE; passCounter++)
		{

			/* Waits until MC2 ready to receive the password */
			while(UART_recieveByte() != MC2_READY);
			/* send the password byte by byte*/
			UART_sendByte(arr_pass[passCounter]);
		}

		/* wait until MC2 save the password in the EEPROM*/
		while(UART_recieveByte()!=PASSWORD_SAVED);
		LCD_clearScreen();
		LCD_moveCursor(0,1);
		LCD_displayString("Saved The Pass");
		_delay_ms(LCD_DISPLAY_DELAY);
	}
	else
	{
		LCD_clearScreen();
		LCD_moveCursor(0,1);
		LCD_displayString("WRONG PASS");
		LCD_moveCursor(1,1);
		LCD_displayString("	TRY AGAIN!!!");
		_delay_ms(LCD_DISPLAY_DELAY);
		createAndCheckPassword();
	}
}

/*Description: Function to save the  password
 * Inputs:
	1. array: to store the password
	2. size: to know the size of the password [size of the array]
	3. Key: values that take from the keypad
 */
void savePassword(uint8 *arr_pass , uint8 arr_size, uint8 key_num)
{
	/* send the 5bytes of the Password*/
	for(uint8 passCounter = 0; passCounter < PASSWORD_SIZE; passCounter++)
	{
		/* Get the pressed key number, if any switch pressed for more than 500 ms
		 * it will considered more than one press */
		key_num = KEYPAD_getPressedKey();
		_delay_ms(10);

		/* if the user enter any character except from 0 to 9 don't accept it*/
		do
		{
			key_num = KEYPAD_getPressedKey();
			_delay_ms(10);
		}while((!((key_num>=0) && (key_num<=9))));

		/*when this loop end this means that the user entered from 0 to 9*/

		/*set the pass in the confirmation array*/
		arr_pass[passCounter] = key_num;
		/* display on the LCD as ASCII '*' */
		LCD_displayCharacter(PASSWORD_MARK);
		_delay_ms(KEY_BUTTON_DELAY);	/*press time*/
	}
	do
	{
		/* won't save the password until the person press =*/
		key_num = KEYPAD_getPressedKey();
		_delay_ms(50);
	}while(key_num != SUBMIT_PASSWORD);
}

/*
 * Description:
	Function to check whether the password correct or wrong
 */
void checkPasswordAfterCreation(void)
{
	/* to get the pressed key */
	uint8 key;

	/* array to get the pass */
	uint8 arr_pass[PASSWORD_SIZE];

	/* variable to count the size of actual password*/
	uint8 passCounter = 0;

	/* variable to store the value while receiving if the password is wrong or not */
	uint8 password_receivig ;

	uint8 wrong_times = 0;	/* variable to store the wrong times that password have been submitted*/

	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("plz enter pass: ");
	LCD_moveCursor(1,0);

	/*enter your password that contains of 5 characters using keypad*/
	savePassword(arr_pass,PASSWORD_SIZE,key);
	LCD_clearScreen();

	/* send the password to EEPROM to check whether the password is right or wrong*/
	UART_sendByte(PASSWORD_CHECK);
	_delay_ms(10);

	/* loop to send the password to the EEPROM to be checked*/
	passCounter = 0;


	/* check if the two arrays of password are identical or not*/
	while(passCounter < PASSWORD_SIZE)
	{
		/* waits until MC2 receive*/
		while(UART_recieveByte()!=MC2_READY);

		/*send each byte*/
		UART_sendByte(arr_pass[passCounter]);

		password_receivig = UART_recieveByte();
		if(password_receivig == PASSWORD_DOESNT_MATCH)
		{
			wrong_times++;
		}
		passCounter++;
	}

	/* this means that u have entered the password correct*/
	if(wrong_times == 0)
	{
		LCD_clearScreen();
		LCD_moveCursor(0,1);
		LCD_displayString("Correct");
		LCD_moveCursor(1,0);
		LCD_displayString("Password");
		_delay_ms(LCD_DISPLAY_DELAY);
	}
	else
	{
		_delay_ms(500);
	}
	g_flagPassword = password_receivig;	// to store the response
}

/*
 * Description:
 * Function that after choosing the option +
 * and entered the password correctly
 	 	 1. Door Unlocking 15 seconds
 	 	 2. Door Open 3 seconds
 	 	 3. Door Locking 15 seconds
 */
void doorSequence(void)
{
	Timer1_init(&Timer1_Configuration);
	Timer1_setCallBack(Timer_callBack);

	/*1. Display The Door is Unlocking*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("Door is ");
		LCD_moveCursor(1,0);
		LCD_displayString("Unlocking");

	/*set g_tick = 0 to clear it*/
	g_tick = 0;
	/* waits until the door unlock time be done then break the loop*/
	while (g_tick != UNLOCK_DOOR_TIME) {} ;

	/*2. Display The Door is OPEN*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("Door is open");

	/*set g_tick = 0 to clear it*/
	g_tick = 0;
	/* waits until the door open time be done then break the loop*/
	while (g_tick != OPEN_DOOR_TIME) {} ;


	/*3. Display The Door is Locking*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("Door is Locking");

	/*set g_tick = 0 to clear it*/
	g_tick = 0;
	/* waits until the door lock time be done then break the loop*/
	while (g_tick != LOCK_DOOR_TIME) {} ;
	Timer1_deInit();		/* stop the timer */

	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("Door is Locked");
	_delay_ms(LCD_DISPLAY_DELAY);	// to see this message

}

/*
 * Description:
 * Function that after choosing the option +
 * and entered the password 3 times wrong
 	 	 It display ERROR for 60 seconds
 */
void buzzerSequence(void)
{
	/* create configuration for timer to get interrupt every 1 second */
	Timer1_init(&Timer1_Configuration);
	Timer1_setCallBack(Timer_callBack);


	/*Display ERROR cause u have entered the max no allowed of passwords wrong*/
	LCD_clearScreen();
	LCD_moveCursor(0,0);
	LCD_displayString("ERROR! 3 times");
	LCD_moveCursor(1,0);
	LCD_displayString("wait 60 sec");
	_delay_ms(LCD_DISPLAY_DELAY);
	/* start the timer */
	Timer1_init(&Timer1_Configuration);
	/* waits until the error time be done then break the loop*/

	/*set g_tick = 0 to clear it*/
	g_tick = 0;
	/* waits until the door open time be done then break the loop*/
	while (g_tick != ERROR_TIME) {} ;

	Timer1_deInit();		/* stop the timer */
}


/*
 * Description:
 * Function that view options [+,-] and process your choice
 * 	- if you press '+' this means the door will go through 3 steps
 	 	 1. Door Unlocking 15 seconds
 	 	 2. Door Open 3 seconds
 	 	 3. Door Locking 15 seconds
 * 	- if you press '-' this means that u will change the password
 */
void mainOptions(void)
{
	/* to get the pressed key */
	uint8 option;

	LCD_clearScreen();
	/*Display Options :		+ => Open Door		, - => set a new password*/
	LCD_moveCursor(0,1);
	LCD_displayString("+ : Open Door");
	LCD_moveCursor(1,1);
	LCD_displayString("- : Change Pass");
	/* note that we should use do while here cause he must press + or -
	 * if the person pressed any other button won't get out of this loop*/
	/* make him must choose + or - any button else make him in the loop*/
	_delay_ms(500);
	do
	{
		/* get the pressed key value */
		option = KEYPAD_getPressedKey();
		_delay_ms(KEY_BUTTON_DELAY);
		if((option == OPEN_DOOR_OPTION) || (option == CHANGE_PASSWORD_OPTION))
			break;
	}while(1);

	LCD_clearScreen();

	/* make loop that decrease from 3 to 0 to check no. of wrong times have beed inserted*/
	uint8 count = MAX_NO_OF_WRONG_TIMES;

	while(count>0)
	{
		/*send to MC2 OPEN the door so we need to insert password */
		UART_sendByte(SEND_PASSWORD_TO_BE_CHECKED);

		checkPasswordAfterCreation();

		/* using the previous function[check pass] will effect the global flag status*/
		if(g_flagPassword == PASSWORD_MATCH)
		{
			/*there nothing wrong with the password [correct]*/
			break;
		}
		/* if we didn't break the loop this means that the password is wrong */
		LCD_clearScreen();
		LCD_moveCursor(0,1);
		LCD_displayString("Wrong Pass");
		_delay_ms(LCD_DISPLAY_DELAY);
		count--;
	}
	if(g_flagPassword == PASSWORD_DOESNT_MATCH)
	{
		/* send command to the MC2 to turn the buzzer on */
		UART_sendByte(BUZZER_ON_BYTE);
		buzzerSequence();
	}
	else if(g_flagPassword == PASSWORD_MATCH)
	{
		switch (option)
		{
		case OPEN_DOOR_OPTION:
			/*send to MC2 to open the door [rotate the DC-motor]*/
			UART_sendByte(UNLOCK_THE_DOOR);
			doorSequence();
			break;
		case CHANGE_PASSWORD_OPTION:
			/*send to MC2 to change the password in the EEPROM */
			UART_sendByte(CHANGE_PASSWORD);
			createAndCheckPassword();
			break;

		}
	}
}



/* Description:
 * 	used to set the required time in each task.
 */
void Timer_callBack(void)
{
	g_tick++;
	/* clear the register*/
	TCNT1_REG.TwoBytes = 0;
}
