/*! @mainpage Guia2_ej3
 *
 * @section genDesc General Description
 *
 * Este programa utiliza FreeRTOS para gestionar tareas relacionadas con la medición de distancia utilizando 
 * un sensor ultrasónico HC-SR04 y el control de LEDs y una pantalla LCD. 
 * El sistema está diseñado para utilizar temporizadores y notificaciones de tareas para gestionar 
 * la medición de distancia y la actualización de la pantalla LCD basada en eventos de temporizador.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * | Peripheral   | ESP32   |
 * |:-------------|:--------|
 * | SENSOR_TRIG   | GPIO_3  |
 * | SENSOR_ECHO   | GPIO_2  |
 * | SWITCH_1      | GPIO_X  |
 * | SWITCH_2      | GPIO_Y  |
 * | LED_1         | GPIO_A  |
 * | LED_2         | GPIO_B  |
 * | LED_3         | GPIO_C  |
 * | LCD           | GPIO_D  |
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
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/

/**Variable que almacena el valor que mide de distancia.*/
uint16_t distancia;

/*Si es TRUE prende el display, si es FALSE lo apaga.*/
bool on=true;

/*Si es TRUE congela el valor que se muestra por display.*/
bool hold=false;

/** Periodo de la interrupción del temporizador A en microsegundos.*/
#define CONFIG_BLINK_PERIOD1 1000000

/** Periodo de la interrupción del temporizador B en microsegundos.*/
#define CONFIG_BLINK_PERIOD 500000

TaskHandle_t medir= NULL;
TaskHandle_t mostrar= NULL;

/*==================[internal data definition]===============================*/

/**
 * @fn static void FuncTimerA(void *pvParameter);
 * @brief Notifica cada tarea cuando debe ser interrumpida.
 * @param[in] void *pvParameter
 * @return 
*/
void FuncTimerA(void *pvParameter);

/**
 * @fn static void FuncTimerB(void *pvParameter);
 * @brief Función invocada en la interrupción del timer B
 * @param[in] void *pvParameter
 * @return
 */
void FuncTimerB(void *pvParameter);

/**
 * @fn static void CambiarEncendido();
 * @brief Si le paso 'O' prende el display o lo apaga.
 * @param[in] 
 * @return 
*/
void CambiarEncendido();

/**
 * @fn static void Congelar();
 * @brief Si se pasa H por consola deja fijo el valor que se veia por display, hasta que se vuelva a apretar H.
 * @param[in] 
 * @return 
*/
void Congelar();

/**
 * @fn static void MedirTask(void *pvParameter);
 * @brief Tarea para medir la distancia con el sensor, la guarda en centimetros en una variable.
 * @param[in] void *pvParameter
 * @return 
*/
void MedirDistancia(void *pvParameter);

/**
 * @fn static void MostrarDistancia(void *pvParameter);
 * @brief Tarea que escribe en el display las distancias que mide si está en ON, o apaga el display si esta !ON.
 * @param[in] void *pvParameter
 * @return 
*/
void MostrarDistancia(void *pvParameter);

/**
 * @fn static void LedsTask(void *pvParameter);
 * @brief Tarea para prender o apagar los leds dependiendo que distancia esté midiendo el sensor.
 * @param[in] void *pvParameter
 * @return 
*/
void LedsTask();

/**
 * @fn static void UartTask(void *pvParameter);
 * @brief Tarea que envía la distancia medida a través del puerto UART para que pueda ser monitoreada desde una PC.
 * @param[in] void *pvParameter
 * @return 
 */
void UartTask(void *pvParameter);

/**
 * @fn static void CambiarEstado(void *pvParameter);
 * @brief Función que cambia el estado de las variables encendido y hold ediante comandos enviados a través del puerto UART.
 * @param void *pvParameter 
 * @return 
 */
void CambiarEstado(void *pvParameter);

/*==================[internal functions declaration]=========================*/

void FuncTimerA(void *pvParameter)
{
    vTaskNotifyGiveFromISR(medir, pdFALSE);    /* Envía una notificación a la tarea asociada al LED_1 */
}

void FuncTimerB(void *pvParameter)
{
    vTaskNotifyGiveFromISR(mostrar, pdFALSE);    /* Envía una notificación a la tarea asociada al LED_2 */
}

void CambiarEncendido()
{
	on=!on;
}

void Congelar()
{
	hold=!hold;
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

void MedirDistancia(void *pvParameter)
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

void MostrarDistancia(void *pvParameter)
{
	while (true)
	{	
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		if(on)
		{
			LedsTask();
			if (!hold)
			{
				LcdItsE0803Write(distancia);
			}
		}
		else 
		{
			LcdItsE0803Off();
			LedsOffAll();
		}
	}
}

void UartTask(void *pvParameter)
{
	while(true)
	{
		UartSendString(UART_PC, "la distancia es:");
		UartSendString(UART_PC, (char*)UartItoa(distancia, 10));
		UartSendString(UART_PC, " cm");
		UartSendString(UART_PC, "\r\n");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void CambiarEstado(void *param)
{
	uint8_t caracter;
	UartReadByte(UART_PC, &caracter);
	if (caracter == 'o')
		on = !on;
	if (caracter == 'h')
		hold = !hold;
}

/*==================[external functions definition]==========================*/

 /**Función principal que inicializa el sistema y las tareas de FreeRTOS.
  Configura los LEDs, los interruptores, la pantalla LCD y el sensor HC-SR04. 
  Luego crea las tareas de FreeRTOS para medir la distancia y mostrar la distancia en la pantalla LCD.
 */
void app_main(void)
{
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

	serial_config_t pantalla = 
	{
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = CambiarEstado,
		.param_p = NULL,
	};

	UartInit(&pantalla);

	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);
	SwitchActivInt(SWITCH_1, CambiarEncendido, NULL);
	SwitchActivInt(SWITCH_2, Congelar, NULL);

	xTaskCreate(&MedirDistancia, "medir", 2048, NULL, 5, &medir);
	xTaskCreate(&MostrarDistancia, "mostrar", 512, NULL, 5, &mostrar);
	xTaskCreate(&UartTask, "UART", 512, &pantalla, 5, NULL);

	TimerStart(timer_led_1.timer);
	TimerStart(timer_led_2.timer);
}

/*==================[end of file]============================================*/