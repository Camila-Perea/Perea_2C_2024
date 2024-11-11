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
 * |    HC-SR04     |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	 +5V 	 	| 	 +5V		|
 * | 	 GND 	 	| 	 GND		|
 *
 **|    Barrera    |    ESP32   	|
 * |:-------------:|:---------- ----|
 * | 	PIN        |   	GPIO_15		|
 *
 * |   Balanza     |    ESP32   	|
 * |:-------------:|:---------- ----|
 * | 	galga 1    |     CH1 	    |
 * | 	galga 2    |   	 CH2		|
 * 
 * @section changelog Changelog
 *
*  |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/11/2024 | Examen Promocional Recuperatorio               |
 *
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
#include "math.h"
/*==================[macros and definitions]=================================*/
/** Periodo de la interrupción del temporizador de la medición de la distancia. */
#define CONFIG_BLINK_PERIOD 100000 // 0,1 seg
/** Handle para la tarea la medicion de distancia */
TaskHandle_t medir_task_handle = NULL;
/** Handle para la tarea de la apertura y cierre de la barrera */
TaskHandle_t barrera_task_handle = NULL;
/**Defino GPIO para la apertura y cierre de la barrera */
#define GPIO_BARRERA GPIO_15

/*==================[internal data definition]===============================*/
/** Variable que indica si la medición de distancia está activada. */
bool on = true;
/** Variable que almacena el valor que mide de distancia.*/
uint16_t distancia;
/** Variable que almacena la velocidad calculada.*/
float velocidad;
/**Me guarda el valor que me da la galga 1*/
uint16_t valor_sensado_1;
/**Me guarda el valor que me da la galga 2*/
uint16_t valor_sensado_2;
/**Me guarda el valor del peso total*/
uint16_t peso;
/**Me guarda el valor del peso que lee el canal 1*/
float galga_1 = 0;
/**Me guarda el valor del peso que lee el canal 2*/
float galga_2 = 0;
/**Me guarda el valor del peso promedio de la galga 1*/
float promedio_1 = 0;
/**Me guarda el valor del peso promedio de la galga 2*/
float promedio_2 = 0;

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

/**
 * @fn FuncTimerDistancia
 * @brief Función que se ejecuta en la interrupción del temporizador A.
 * @param param 
 */
void FuncTimerDistancia(void* param){

    vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE); 
}

/**
 * @fn FuncTimerDistancia
 * @brief Función que se ejecuta en la interrupción del temporizador B.
 * @param param 
 */
void FuncTimerBarrera(void* param){

    vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE); 
}

/**
 * @fn MostrarMensaje()
 * @brief Funcion encargada de enviar una notificacion por la UART
 * @param 
 */
void MostrarMensaje()
{
	UartSendString(UART_PC, "Peso: \r\n");
	UartSendString(UART_PC, (char*)UartItoa(peso, 10));
	UartSendString(UART_PC," kg\n");
	UartSendString(UART_PC, "Velocidad: \r\n");
	UartSendString(UART_PC, (char*)UartItoa(velocidad, 10));
	UartSendString(UART_PC," m/s\n");
}

/**
 * @fn Pesar()
 * @brief Esta funcion se encargada de medir el peso en cada galga.
 * Esta medida obtenida analogicamente, se convierte a kg, se suma y luego se promedia.
 * @param 
 */
void Pesar()
{
	for(int i=0; i<50; i++)
	{
		//Leo el voltaje que mide cada galga en su respectivo canal
		AnalogInputReadSingle(CH1, &valor_sensado_1);
		AnalogInputReadSingle(CH2, &valor_sensado_2);

		//Tomo el valor de voltaje sensado, lo guardo y le voy sumando el valor de cada medición
		galga_1 += (valor_sensado_1*20000)/3000;
		galga_2 += (valor_sensado_2*20000)/3000;
	}
		//Calculo del promedio
		promedio_1= galga_1/50;
		promedio_2= galga_2/50;

		//El peso total es la suma de los promedios de ambas galgas
		peso = promedio_1 + promedio_2;

		vTaskDelay(5/ portTICK_PERIOD_MS); //tasa de 200 muestras por segundo
	

}
/**
 * @fn CalcularVelocidad
 * @brief Calcula la velocidad instantanea del camion 
 * @param 
 */
void CalcularVelocidad()
{
	uint16_t distanciaAnterior = 0;
	float aux = 0;
	velocidad = (distancia*100)-(distanciaAnterior*100)/(0.1); // Velocidad en m/s
	distanciaAnterior= distancia;
}

/**
 * @fn ControlLeds
 *  @brief Controla el encendido de los LEDs según la velocidad.
 * - Prende el LED 3 si la velocidad es mayor a 8 m/s.
 * - Prende el LED 2 si la velocidad es menor a 8 m/s y mayor a 0 m/s.
 * - Prende el LED 1 si la velocidad es 0 m/s.
 * @param 
 */
void ControlLeds()
{
	if (velocidad>8)
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
	else if(velocidad==0)
	{ 
		LedOff(LED_1);
		LedOff(LED_2);
		LedOn(LED_3);
		Pesar();
	}
}

/**
 * @fn MedirDistancia(void *pvParameter)
 * @brief Mide la distancia utilizando el sensor HcSr04. La tarea se desbloquea
 * mediante notificaciones para realizar la medición.
 * @param param no utilizado
 */
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

/**
 * @fn AperturaBarrera()
 * @brief  Tarea encargada controlar desde la PC la apertura y cierre de la barrera.
 * Ante el envío del caracter 'o' se abre la barrera y con el caracter 'c', se cierra
 * @param param no utilizado
 */
void AperturaBarrera()
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t caracter;
        UartReadByte(UART_PC, &caracter);
		if (caracter == 'o') 
		{
           	GPIOOn(GPIO_BARRERA);
        } 
		else if (caracter == 'c') 
		{    
            GPIOOff(GPIO_BARRERA);
        }
	}
}

void app_main(void){
	
	// Configuración de la UART
	serial_config_t my_uart = {
		.port = UART_PC, 
		.baud_rate = 115200, 
		.func_p = NULL, 
		.param_p = NULL
	};
	UartInit(&my_uart);
	
	// Configuración de las entradas analógicas
	analog_input_config_t analog_1 = 
	{
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&analog_1);

	analog_input_config_t analog_2 = 
	{
		.input = CH2,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&analog_2);

	// Configuración de temporizador de la medicion de la distancia
    timer_config_t timer_medir_distancia = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimerDistancia,
        .param_p = NULL
    };

    TimerInit(&timer_medir_distancia);

	// Configuración de temporizador de la apertura y cierre de la barrera
	 timer_config_t timer_barrera = {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD,
        .func_p = FuncTimerBarrera,
        .param_p = NULL
    };

    TimerInit(&timer_barrera);

	xTaskCreate(&MedirDistancia, "distancia_task_handle", 2048, NULL, 5, NULL);
	xTaskCreate(&AperturaBarrera, "barrera_task_handle", 2048, NULL, 5, &barrera_task_handle);

    TimerStart(timer_medir_distancia.timer);
	TimerStart(timer_barrera.timer);

}
/*==================[end of file]============================================*/