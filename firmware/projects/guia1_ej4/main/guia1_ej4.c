/*! @mainpage Guia_1_EJ4
 *
 * @section Este programa tiene como función recibir un dato de 32 bits,
 * la cantidad de dígitos de salida y un puntero a un arreglo donde se almacan los n dígitos
 * Y convierte el dato recibido a BCD.
 * 
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 21/08/2024 | Document creation		                         |
 *
 * @author Camila Perea (camila.perea@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
	uint32_t data=123;
	uint8_t digits= 3;
	uint8_t bcd_number[digits];

	convertToBcdArray (data,digits,bcd_number);
}


/*==================[end of file]============================================*/
