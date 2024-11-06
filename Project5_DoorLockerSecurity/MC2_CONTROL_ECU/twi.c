 /******************************************************************************
 * Module: TWI
 * File Name: twi.c
 * Description: Source file for the AVR TWI driver
 * Author: Yousif Adel
 *******************************************************************************/
#include	"twi.h"
#include	<avr/io.h>
#include	"common_macros.h"


/*******************************************************************************
 *                      Functions Definitions                                   *
 *******************************************************************************/
void TWI_init(const TWI_ConfigType * Config_Ptr)
{

	/* calculate the TWBR using zero pre-scaler TWPS=00 and F_CPU, bit rate  */
	TWSR = 0x00;
	TWBR = (uint16)(((F_CPU / Config_Ptr->bit_rate )-16) / 8);

	/* Two Wire Bus address my address if any master device want to call me: 0x1 (used in case this MC is a slave device)
	       General Call Recognition: Off */
	TWAR = Config_Ptr->address; // my address = 0x01 :)

	/* enable the TWI*/
	TWCR = (1<<TWEN);
}

void TWI_start(void)
{
    /*
	 * Clear the TWINT flag before sending the start bit TWINT=1
	 * send the start bit by TWSTA=1
	 * Enable TWI Module TWEN=1
	 */
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    /* Wait for TWINT flag set in TWCR Register (start bit is send successfully) */
    while(BIT_IS_CLEAR(TWCR, TWINT));
}

void TWI_stop(void)
{
    /*
	 * Clear the TWINT flag before sending the stop bit TWINT=1
	 * send the stop bit by TWSTO=1
	 * Enable TWI Module TWEN=1
	 */
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void TWI_writeByte(uint8 data)
{
    /* Put data On TWI data Register */
	TWDR = data;

    /*
	 * Clear the TWINT flag before sending the data TWINT=1
	 * Enable TWI Module TWEN=1
	 */
    TWCR = (1 << TWINT) | (1 << TWEN);

    /* Wait for TWINT flag set in TWCR Register(data is send successfully) */
    while(BIT_IS_CLEAR(TWCR,TWINT));
}

uint8 TWI_readByteWithACK(void)
{
    /*
	 * Clear the TWINT flag before sending the data TWINT=1
	 * Enable TWI Module TWEN=1
	 */
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    /* Wait for TWINT flag set in TWCR Register (data received successfully) */
    while(BIT_IS_CLEAR(TWCR,TWINT));

    /* Return the Data */
    return TWCR;
}

uint8 TWI_readByteWithNACK(void)
{
	/*
	 * Clear the TWINT flag before reading the data TWINT=1
	 * Enable TWI Module TWEN=1
	 */
    TWCR = (1 << TWINT) | (1 << TWEN);
    /* Wait for TWINT flag set in TWCR Register (data received successfully) */
    while(BIT_IS_CLEAR(TWCR, TWINT));
    /* Read Data */
    return TWDR;
}

uint8 TWI_getStatus(void)
{
    uint8 status;
    /* masking to eliminate first 3 bits and get the last 5 bits (status bits) */
    status = TWSR & 0xF8;		/* 1111 1000*/

    return status;
}