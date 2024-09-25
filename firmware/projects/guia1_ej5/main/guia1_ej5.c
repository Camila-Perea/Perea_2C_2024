/*! @mainpage Guia_1_EJ5
 *
 * @section genDesc General Description
 *
 * Este programa tiene como función recibir un digito en BCD y un vector de estructuras del tipo gpioConf_t.
 * Luego cambia el estado de cada GPIO, a ‘0’ o a ‘1’, según el estado del bit correspondiente en el BCD ingresado
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Camila Perea (camila.perea@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for(int j=0; j<digits; j++)
	{
		bcd_number[j]=data%10;
		data=data/10;

		printf("Valor:%d\r\n",bcd_number[j]);
	}
	return 0;
}

typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


void mappearBits( uint8_t digit, gpioConf_t* vector_pines )
{
	for (int j=0; j<4; j++)
	{
		if(digit&1<<j)
			{GPIOOn(vector_pines[j].pin);}
		else
			GPIOOff(vector_pines[j].pin);
	}
}


/*==================[external functions definition]==========================*/
void app_main(void)
{

	printf("Hello world!\n");
	uint32_t data=123;
	uint8_t digits= 3;
	uint8_t bcd_number[digits];
	convertToBcdArray (data,digits,bcd_number);

	gpioConf_t vector_pines[]= {{GPIO_20,GPIO_OUTPUT}, {GPIO_21,GPIO_OUTPUT}, 
								{GPIO_22,GPIO_OUTPUT}, {GPIO_23,GPIO_OUTPUT},};

	for (int j=0; j<4; j++)
	{
		GPIOInit(vector_pines[j].pin, vector_pines[j].dir);
	}
	 
	 uint8_t digito  = 5;
	mappearBits (digito, vector_pines);

}
/*==================[end of file]============================================*/