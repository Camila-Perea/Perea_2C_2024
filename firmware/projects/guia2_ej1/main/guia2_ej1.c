/*! @mainpage Guia_2_EJ1
 *
 * @section genDesc General Description
 *Este programa tiene como función leer la distancia que mide un sensor y mostrarla por display. Ademas cambia los leds que estan prendidos o
 *apagados dependiendo que valor tiene esa distancia. Por otro lado tambien realiza determinadas acciones según apreto una tecla u otra.
 * 
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * |     ECHO       |   GPIO_3      |
 * |   	+5V 	    | 	 +5V		|
 * |   	GND 	    | 	 GND		|
 * 
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 29/08/2024 | Document creation		                         |
 *
 * @author Camila Perea (camila.perea@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "hc_sr04.h"
#include "lcditse0803.h"
#include <led.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "switch.h"

/*==================[macros and definitions]=================================*/

/**Variable que almacena el valor que mide de distancia.*/
uint16_t distancia;

/**Variable que almacena el valor que mide de distancia.*/
uint8_t teclas;

/**Si es TRUE prende el display, si es FALSE lo apaga.*/
bool on=true;

/**Si es TRUE congela el valor que se muestra por display.*/
bool hold=false;

/**Periodo al que quiero que se muestre la distancia.*/
#define CONFIG_BLINK_PERIOD 500

/**Periodo al que quiero medir la distancia.*/
#define CONFIG_BLINK_PERIOD_MEDIR 1000

/**Periodo al que quiero que lea lo que le mando por las teclas.*/
#define CONFIG_BLINK_PERIOD_TECLAS 200

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
/**
 * @fn void MedirDistancia();
 * @brief Escribe en distancia lo que sensa.
 * @param[in] 
 * @return 
*/
void MedirDistancia(void *pvParameter);

/**
 * @fn void actualizarLed();
 * @brief Prende o apaga los leds dependiendo que distancia este midiendo el sensor.
 * @param[in] 
 * @return 
*/
void ActualizarLed();

/**
 * @fn void MostrarDistancia();
 * @brief Actualiza los leds y el display dependiendo lo que mide el sensor, y si ON u HOLD son TRUE o FALSE.
 * @param[in] 
 * @return 
*/
void MostrarDistancia(void *pvParameter);

/**
 * @fn void LeerTeclas();
 * @brief Determina la accion a realizar si se apreta una tecla u otra.
 * @param[in]
 * @return
*/
void LeerTeclas(void *pvParameter);

/*==================[external functions definition]==========================*/

void ActualizarLed()
{
	if(distancia<10)
	{
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if(distancia>=10 && distancia<20)
	{ 
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if(distancia>=20 && distancia<30)
	{ 
		LedOn(LED_1);
		LedOn(LED_2);
		LedOff(LED_3);
	}
	else if(distancia>30)
	{
		LedOn(LED_1);
		LedOn(LED_2);
		LedOn(LED_3);
	}
}

void MedirDistancia(void *pvParameter)
{
	while(1) 
	{
		printf ("midiendo\n\r");
		if(on==true)
			distancia=HcSr04ReadDistanceInCentimeters();
		else
			LedsOffAll();
		vTaskDelay(CONFIG_BLINK_PERIOD_MEDIR / portTICK_PERIOD_MS);
	}
}

void MostrarDistancia(void *pvParameter)
{
	while(1) 
	{
		printf ("actualizando\n\r");

		if(on==true)
		{
			ActualizarLed();
			if(hold!=true)
			{
			LcdItsE0803Write(distancia);
			}
		}
		else
		{
			LedsOffAll();
			LcdItsE0803Off();
		}
		vTaskDelay(CONFIG_BLINK_PERIOD/ portTICK_PERIOD_MS);
	}
	
}

void LeerTeclas(void *pvParameter)
{
	while(1)
	{printf ("teclas\n\r");

     	teclas  = SwitchesRead();
     	switch(teclas)
		{
     		case SWITCH_1:
				on = !on;
			break;
			case SWITCH_2:
				hold = !hold;
			break;

		}
		vTaskDelay(CONFIG_BLINK_PERIOD_TECLAS / portTICK_PERIOD_MS);
	}
}

void app_main(void){

	HcSr04Init(GPIO_3, GPIO_2);
	LedsInit();
	LedsOffAll();
	LcdItsE0803Init();
	SwitchesInit();
	
	xTaskCreate(&LeerTeclas, "teclas_task",2048, NULL,5,NULL);
	xTaskCreate(&MedirDistancia, "distancia_task",2048, NULL,5,NULL);
	xTaskCreate(&MostrarDistancia, "distancia_task",2048, NULL,5,NULL);
}
/*==================[end of file]============================================*/