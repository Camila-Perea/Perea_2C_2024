/*! @mainpage guia2_ej4
 *
 * @section Este programa digitaliza una señal analógica y la transmite a un graficador de puerto serie 
 * de la PC. 
 * Luego convierte una señal digital de un ECG en una señal analógica para visualizarla utilizando 
 * el osciloscopio.
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
 * | 03/10/2024 | Document creation		                         |
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
#include "uart_mcu.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/

/**Defino el periodo al que se va a convertir la señal analogica a digital.*/
#define CONFIG_BLINK_PERIOD_MEDIR 20000

/**Defino cada cuanto mando los datos que recibo por CH.*/
#define CONFIG_BLINK_PERIOD_ECG 10000

TaskHandle_t medir= NULL;
TaskHandle_t main_task_handle = NULL;

/**Defino el tamaño de la muestra*/
#define BUFFER_SIZE 231

/*==================[internal data definition]===============================*/

/**Arreglo que guarda los datos del ecg.*/
uint8_t ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/**
 * @fn static void FuncTimer(void *pvParameter);
 * @brief Función que invocada la interrupción del timer del conversor ADC
 * @param[in] void *pvParameter
 * @return
 */
void FuncTimer(void *pvParameter);

/**
 * @fn static void ConversorADC(void *pvParameter);
 * @brief Función que lee el dato del canal y lo convierte a ASCII
 * @param[in] void *pvParameter
 * @return
 */
void ConversorADC(void *pvParameter);

/**
 * @fn static void ConversorECG(void *pvParameter);

 * @brief Función que escribe la muestra del ecg cada un determinado periodo.
 * @param[in] void *pvParameter
 * @return
 */
void ConversorECG(void *pvParameter);

/**
 * @fn static void FuncTimerOut(void *pvParameter);
 * @brief Función que invocada la interrupción del timer del conversorECG
 * @param[in] void *pvParameter
 * @return
 */
void FuncTimerOut(void *pvParameter);

/*==================[internal functions declaration]=========================*/

void FuncTimer(void *pvParameter)
{
    vTaskNotifyGiveFromISR(medir, pdFALSE);  
}

void ConversorADC(void *pvParameter)
{
	uint16_t medicion; 

	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		AnalogInputReadSingle(CH1, &medicion); //devuelve el valor digitalizado
		UartSendString(UART_PC, (char*)UartItoa(medicion, 10));
		UartSendString(UART_PC, "\r\n");
	}
}

void FuncTimerOut(void *pvParameter)
{
	vTaskNotifyGiveFromISR(main_task_handle, pdFALSE);  
}

void ConversorECG(void *pvParameter)
{
	uint8_t i=0;
	while (1)
	{
		if(i<BUFFER_SIZE)
		{
			AnalogOutputWrite(ecg[i]);
			i++;
		}
		else 
		{
			i=0;
		}

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

	}
}


/*==================[external functions definition]==========================*/

void app_main(void)
{
	analog_input_config_t analog = {
		.input = CH1, //le paso el canal
		.mode = ADC_SINGLE,//el modo en el que va a operar
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	
	AnalogInputInit(&analog);
	AnalogOutputInit();

	//Inicialización de los timers
	timer_config_t timer_1 = {
			.timer = TIMER_A,
			.period = CONFIG_BLINK_PERIOD_MEDIR,
			.func_p = FuncTimer,
			.param_p = NULL
	};
	TimerInit(&timer_1);

	serial_config_t my_uart = {
		.port = UART_PC, 
		.baud_rate = 38400, // 1/38400 * 50 < 20milisegundos de muestreo
		.func_p = NULL, 
		.param_p = NULL
	};
	UartInit(&my_uart);

	timer_config_t timer_ecg = {
			.timer = TIMER_B,
			.period = CONFIG_BLINK_PERIOD_ECG,
			.func_p = FuncTimerOut,
			.param_p = NULL
	};
	TimerInit(&timer_ecg);

	xTaskCreate(&ConversorADC, "conversorADC", 2048, NULL, 5, &medir);
	xTaskCreate(&ConversorECG, "conversorECG", 2048, NULL, 5, &main_task_handle);
	TimerStart(timer_1.timer);
	TimerStart(timer_ecg.timer);


}
/*==================[end of file]============================================*/