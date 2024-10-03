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
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "math.h"
#include "gpio_mcu.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
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

/** Periodo de la interrupción del temporizador A en microsegundos.*/
#define CONFIG_BLINK_PERIOD1 1000000

/** Periodo de la interrupción del temporizador B en microsegundos.*/
#define CONFIG_BLINK_PERIOD 500000

TaskHandle_t medir_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/**
 * @fn static void FuncTimerA(void *pvParameter);
 * @brief Notifica cada tarea cuando debe ser interrumpida.
 * @param[in] void *pvParameter
 * @return 
*/
void FuncTimerA(void* param);

/**
 * @fn static void FuncTimerB(void *pvParameter);
 * @brief Función invocada en la interrupción del timer B
 * @param[in] void *pvParameter
 * @return
 */
void FuncTimerB(void* param);

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

void FuncTimerA(void* param)
{
    //le manda una notif a la tarea (se usa el handle no el nombre de la tarea)
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);
}

void FuncTimerB(void* param)
{
    vTaskNotifyGiveFromISR(display_task_handle, pdFALSE);    /* Envía una notificación a la tarea asociada al LED_2 */
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		if(on==true)
			distancia=HcSr04ReadDistanceInCentimeters();
		else
			LedsOffAll();
	}
}

void DisplayTask(void *pvParameter)
{
	while(1) 
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		if(on==true)
		{
			LedsTask();
			if(!hold)
			{
			LcdItsE0803Write(distancia);
			}
		}
		else
		{
			LedsOffAll();
			LcdItsE0803Off();
		}
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
   LedsInit();
	timer_config_t timer_led_1 = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timer_led_1);
    timer_config_t timer_led_2 = {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD1,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timer_led_2);

/// creación de las tareas que quiero ejecutar 
	xTaskCreate(&MedirTask, "Medir", 2048, NULL, 5, &medir_task_handle);
	xTaskCreate(&DisplayTask, "Display", 512, NULL, 5, &display_task_handle);

/* Inicialización del conteo de timers */
	TimerStart(timer_led_1.timer);
 TimerStart(timer_led_2.timer);
}
/*==================[end of file]============================================*/