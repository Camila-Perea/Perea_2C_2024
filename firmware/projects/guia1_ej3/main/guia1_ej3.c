/*! @mainpage Template
 *
 * @section genDesc General Description
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
#include "led.h"

/*==================[macros and definitions]=================================*/

#define ON 1
#define OFF 2
#define TOGGLE 3
#define CONFIG_BLINK_PERIOD1 100

 

/*==================[internal data definition]===============================*/
struct leds
{
    uint8_t mode;        //ON, OFF, TOGGLE
	uint8_t n_led;       //indica el número de led a controlar
	uint8_t n_ciclos;    //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
} my_leds;

/*==================[internal functions declaration]=========================*/

void parpadeoLED (struct leds *LED){

switch(LED -> mode)
{
	case ON:
	if (LED -> n_led == 1) //enciende led1
		{LedOn (LED_1);}
	if (LED -> n_led == 2) //enciende led2
		{LedOn (LED_2);}
	if (LED -> n_led == 3) //enciende led3
		{LedOn (LED_3);}
		break;

	case OFF:
		if (LED -> n_led == 1) //apaga led1
			{LedOff (LED_1);}
		if (LED -> n_led == 2) //apaga led2
			{LedOff (LED_2);}
		if (LED -> n_led == 3) //apaga led3
			{LedOff (LED_3);}
			break;
 
	case TOGGLE: 
		
			for (int i = 0; i <LED->n_ciclos; i++)
			{printf("toogle\n");
				LedToggle (LED_1);
				for (int i = 0; i <LED->periodo/CONFIG_BLINK_PERIOD1; i++)
				{
					vTaskDelay(CONFIG_BLINK_PERIOD1/ portTICK_PERIOD_MS);
					printf("esperando\n");
				}
			}
			for (size_t i = 0; i <LED->n_ciclos; i++)
			{
				LedToggle (LED_2);
				for (int i = 0; i <LED->periodo/CONFIG_BLINK_PERIOD1; i++)
				{
					vTaskDelay(CONFIG_BLINK_PERIOD1/ portTICK_PERIOD_MS);
				}
			}
			for (int i = 0; i <LED->n_ciclos; i++)
			{
				LedToggle (LED_3);
				for (int i = 0; i <LED->periodo/CONFIG_BLINK_PERIOD1; i++)
				{
					vTaskDelay(CONFIG_BLINK_PERIOD1/ portTICK_PERIOD_MS);
				}
			}
		}
		}


/*==================[external functions definition]==========================*/
void app_main(void){

	struct leds *LED;
	LED = &my_leds;
	LedsInit();

	LED->mode=TOGGLE;
	LED->n_ciclos=40;
	LED->periodo=500;
	LED->n_led=1;
	parpadeoLED(LED);
	while(true){}
}
/*==================[end of file]============================================*/