/*! @mainpage TC: Guia2_Ej2
 *
 * @section genDesc General Description
 * Este programa tiene como función leer la distancia que mide un sensor y mostrarla por display. Ademas cambia los leds que estan prendidos o
 * apagados dependiendo que valor tiene esa distancia. Por otro lado tambien realiza determinadas acciones según apreto una tecla u otra.
 * Todas estas funciones se realizan a traves de tareas y a un determinado periodo, definido con un timer.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	GPIO_3	 	| 	ECHO		|
 * | 	GPIO_2	 	| 	TRIGGER		|
 * | 	 +5V	 	| 	 +5V		|
 * |  	 GND	 	| 	 GND		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2024 | Document creation		                         |
 *
 * @author  Camila Perea (camila.perea@ingenieria.uner.edu.ar)
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
#include "switch.h"
#include "timer_mcu.h"

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
#define CONFIG_BLINK_PERIOD 1000000

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
/**
 * @fn static void FuncTimer(void *pvParameter);
 * @brief Notifica cada tarea cuando debe ser interrumpida.
 * @param[in] void *pvParameter
 * @return 
*/
void FuncTimer(void* param);

/**
 * @fn static void MedirTask(void *pvParameter);
 * @brief Tarea para medir la distancia con el sensor, la guarda en centimetros en una variable.
 * @param[in] void *pvParameter
 * @return 
*/
void MedirTask(void *pvParameter);

/**
 * @fn static void LedsTask(void *pvParameter);
 * @brief Tarea para prender o apagar los leds dependiendo que distancia esté midiendo el sensor.
 * @param[in] void *pvParameter
 * @return 
*/
void LedsTask();

/**
 * @fn static void DisplayTask(void *pvParameter);
 * @brief Tarea que escribe en el display las distancias que mide si está en ON, o apaga el display si esta !ON.
 * @param[in] void *pvParameter
 * @return 
*/
void DisplayTask(void *pvParameter);

/**
 * @fn static void CambiarEstado();
 * @brief Si le paso 'O' prende el display o lo apaga.
 * @param[in] 
 * @return 
*/
void CambiarEstado();

/**
 * @fn static void Congelar();
 * @brief Si se pasa H por consola deja fijo el valor que se veia por display, hasta que se vuelva a apretar H.
 * @param[in] 
 * @return 
*/
void Congelar();

/*==================[external functions definition]==========================*/

void FuncTimer(void* param)
{
    vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE);//le manda una notif a la tarea (se usa el handle no el nombre de la tarea)
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);
    vTaskNotifyGiveFromISR(display_task_handle, pdFALSE);
}

void LedsTask()
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

void MedirTask(void *pvParameter)
{
	while(1) 
	{
		printf ("midiendo\n\r");
		if(on==true)
			distancia=HcSr04ReadDistanceInCentimeters();
		else
			LedsOffAll();
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
	}
}

void DisplayTask(void *pvParameter)
{
	while(1) 
	{
		printf ("mostrar\n\r");
		if(on==true)
		{
			LedsTask();
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
	}
}

void CambiarEstado()
{
	on=!on;
}

void Congelar()
{
	hold=!hold;
}

void app_main(void)
{
HcSr04Init(GPIO_3, GPIO_2);
	LedsInit();
	LedsOffAll();
	SwitchesInit();
	LcdItsE0803Init();

	SwitchActivInt(SWITCH_1, CambiarEstado, NULL);
	SwitchActivInt(SWITCH_2, Congelar, NULL);
	
	/* Inicialización de timers */
    timer_config_t timer_medir = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimer,//puntero a la funcion que quiero q se ejecute x interrupcion
		.param_p = NULL
    };
    TimerInit(&timer_medir);
    timer_config_t timer_mostrar = {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimer,//puntero a la funcion que quiero q se ejecute x interrupcion
		.param_p = NULL
    };
    TimerInit(&timer_mostrar);

/// creación de las tareas que quiero ejecutar 
	xTaskCreate(&MedirTask, "Medir", 2048, NULL, 4, &medir_task_handle);
	xTaskCreate(&DisplayTask, "Display", 2048, NULL, 4, &display_task_handle);

/* Inicialización del conteo de timers */
	TimerStart(timer_medir.timer);
	TimerStart(timer_mostrar.timer);
}
/*==================[end of file]============================================*/