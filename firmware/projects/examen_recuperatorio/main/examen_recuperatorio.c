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
#include <led.h>
#include "hc_sr04.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*==================[macros and definitions]=================================*/
/** Periodo de la interrupción del temporizador de la medición de la distancia. */
#define CONFIG_BLINK_PERIOD 100000
/** Periodo de la interrupción del temporizador de la medición de peso. */
#define CONFIG_BLINK_PERIOD_PES0 100000
TaskHandle_t medir_task_handle = NULL;

/*==================[internal data definition]===============================*/
/** Variable que indica si la medición de distancia está activada. */
bool on = true;
/** Variable que almacena el valor que mide de distancia.*/
uint16_t distancia;
/** Variable que almacena la velocidad calculada.*/
float velocidad;
/**Me guarda el valor que me da la balanza*/
uint16_t valor_sensado;
/**Me guarda el valor del peso*/
uint16_t peso;

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

void FuncTimerDistancia(void* param){

    vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE); 
}

void MostrarMensaje()
{
	UartSendString(UART_PC, "Peso: \r\n");
	UartSendString(UART_PC, (char*)UartItoa(peso, 10));
	UartSendString(UART_PC, " \r");
	UartSendString(UART_PC, "Velocidad: \r\n");
	UartSendString(UART_PC, (char*)UartItoa(velocidad, 10));
	UartSendString(UART_PC, " \r");
}

void Pesar()
{
	for(int i=0; i<50; i++)
	{
		AnalogInputReadSingle(CH1, &valor_sensado);
		
		vTaskDelay(100 / portTICK_PERIOD_MS);

	}

}

void CalcularVelodidad()
{
	velocidad=distancia/CONFIG_BLINK_PERIOD;
	if (velocidad>3)
	{
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if(velocidad>=0 && distancia<8)
	{ 
		LedOff(LED_1);
		LedOn(LED_2);
		LedOff(LED_3);
	}
	else if(velocidad=0)
	{ 
		LedOff(LED_1);
		LedOff(LED_2);
		LedOn(LED_3);
		Pesar();
	}
}

void MedirDistancia(void *pvParameter)
{
	while(true) 
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		distancia=HcSr04ReadDistanceInCentimeters();
			
		if(distancia<100)
		{
			CalcularVelocidad();
		}

			
	}
}

void app_main(void){
	
	// Configuración de temporizador
    timer_config_t timer_medir_distancia = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimerDistancia,
        .param_p = NULL
    };

	serial_config_t my_uart = {
		.port = UART_PC, 
		.baud_rate = 115200, 
		.func_p = NULL, 
		.param_p = NULL
	};
	UartInit(&my_uart);

	analog_input_config_t analog = 
	{
		.input = CH1, //le paso el canal
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&analog);
	AnalogOutputInit();

    TimerInit(&timer_medir_distancia);
	xTaskCreate(&MedirDistancia, "distancia_task_handle", 2048, NULL, 5, NULL);

    TimerStart(timer_medir_distancia.timer);


}
/*==================[end of file]============================================*/