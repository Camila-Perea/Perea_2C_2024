/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * Cebador de mate 
 * 
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral         |   ESP32   	   |
 * |:---------------------:|:--------------|
 * |       HcSr04 	       | GPIO_3, GPIO_2|
 * |       BOMBA 	       |  	GPIO_9     |
 * |     SensorTemp	       |   	CH1 	   |
 * |RESISTENCIA_CALEFACTORA|   	GPIO_23    |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 15/12/2025 | Document creation		                         |
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
#include "timer_mcu.h"
#include "hc_sr04.h"
#include "gpio_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define TIME_PERIOD1 1000000
#define TIME_PERIOD2 1000000

#define GPIO_BOMBA GPIO_9
#define SENSOR_TEMPERATURA CH1
#define RESISTENCIA_CALEFACTORA GPIO_23

#define TEMP_OBJEIVO 75.0
#define VOLTAJE_MIN 0.0
#define VOLTAJE_MAX 3300
#define TEMPERATURA_MIN -10.0
#define TEMPERATURA_MAX 100.0
/*==================[internal data definition]===============================*/
TaskHandle_t task_handle1 = NULL;
TaskHandle_t task_handle2 = NULL;
bool start;
/*==================[internal functions declaration]=========================*/
/** @fn  void Notify(void *param)
 * @brief  notifica a la tareas task_handle1
 * @param *param
 */
void Notify1(void *param)
{
	vTaskNotifyGiveFromISR(task_handle1, pdFALSE);
}

/** @fn  void Notify(void *param)
 * @brief  notifica a la tareas task_handle2
 * @param *param
 */
void Notify2(void *param)
{
	vTaskNotifyGiveFromISR(task_handle2, pdFALSE);
}

/** @fn  void dispensarAgua()
 * @brief  activa la bomba para dispensar agua
 */
void dispensarAgua()
{
	GPIOOn(GPIO_BOMBA);
}

/** @fn  void noDispensarAgua()
 * @brief  desactiva la bomba para dispensar agua
 */
void noDispensarAgua()
{
	GPIOOff(GPIO_BOMBA);
}

/** @fn  ConvertirVoltajeATemperatura(uint16_t voltaje)
 * @brief  convierte los valores de voltaje a temperatura
 * @param voltaje
 */
uint16_t ConvertirVoltajeATemperatura(uint16_t voltaje)
{
	return TEMPERATURA_MIN + (voltaje - VOLTAJE_MIN) * (TEMPERATURA_MAX - TEMPERATURA_MIN) / (VOLTAJE_MAX - VOLTAJE_MIN);
}

/** @fn  void calentarAgua()
 * @brief  activa la resistencia calefactora para calentar agua
 */
void calentarAgua()
{
	GPIOOn(RESISTENCIA_CALEFACTORA);
}

/** @fn  void noCalentarAgua()
 * @brief  desactiva la resistencia calefactora para calentar agua
 */
void noCalentarAgua()
{
	GPIOOff(RESISTENCIA_CALEFACTORA);
}

/** @fn void FunctionStop()
 * @brief Interrupcion que Detiene las operaciones del cebador.
 */
void FunctionStop()
{
	start = false;
}

/** @fn void FunctionStart()
 * @brief Interrupcion que Inicia las operaciones del cebador.
 */
void FunctionStart()
{
	start = true;
}

/** @fn  void Suministro_agua(void *pvParameter)
 * @brief  Tarea para suministrar agua basado en la distancia medida por el sensor ultrasonico
 * @param *param
 */
void Suministro_agua(void *pvParameter)
{
    uint16_t tiempo_cebado = 0;
    uint16_t distancia;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // La tarea espera hasta recibir una notificación

        if (!start)
        {
            noDispensarAgua();  // Apaga la bomba si el sistema no está habilitado
            tiempo_cebado = 0;  // Resetea el contador de cebado
            continue;  // Si no está habilitado, salta al siguiente ciclo
        }

        distancia = HcSr04ReadDistanceInCentimeters(); // Lee la distancia del sensor ultrasónico

        if (distancia > 5 && distancia < 10)  // Si el mate está en el rango de cebado
        {
            if (tiempo_cebado == 0)  // Si es el primer ciclo de cebado
            {
                UartSendString(UART_PC, "Mate en rango, comienza a cebar\r\n");
            }

            if (tiempo_cebado < 5)  // Si no ha alcanzado el tiempo máximo de cebado
            {
                dispensarAgua();  // Activa la bomba para cebar
                tiempo_cebado++;   // Aumenta el contador de cebado
            }
            else  // Si ya se completó el cebado
            {
                noDispensarAgua();  // Desactiva la bomba
                UartSendString(UART_PC, "Mate cebado, retírelo\r\n");
                tiempo_cebado = 0;   // Reinicia el contador para el siguiente cebado
            }
        }
        else  // Si el mate está fuera de rango
        {
            noDispensarAgua();  // Desactiva la bomba
            tiempo_cebado = 0;   // Resetea el contador de cebado
        }
    }
}

/** @fn  void Control_temperatura(void *pvParameter)
 * @brief  Tarea para controlar la temperatura del agua
 * @param *param
 */
void Control_temperatura(void *pvParameter)
{
	uint16_t temperatura_mV;
	uint16_t temperatura;
	while (1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // La tarea espera en este punto hasta recibir una notificación

		if (start)
		{
			AnalogInputReadSingle(SENSOR_TEMPERATURA, &temperatura_mV); // Lee el valor de la temepratura en voltaje (mV)
			temperatura = ConvertirVoltajeATemperatura(temperatura_mV);

			if (temperatura < TEMP_OBJEIVO)
			{
				calentarAgua();
				UartSendString(UART_PC, " “Temperatura correcta…acerque el mate”\r\n"); // mensaje por uart
			}
			else
				noCalentarAgua();
			UartSendString(UART_PC, " “Agua fría…espere”\r\n"); // mensaje por uart
		}
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	// configuro timer1 para el control del suministro de agua
	timer_config_t timer_1 = {
		.timer = TIMER_A,
		.period = TIME_PERIOD1,
		.func_p = Notify1,
		.param_p = NULL};

	TimerInit(&timer_1); // inicializo timer 1

	timer_config_t timer_2 = {
		// configuro timer 2 para el control de la temperatura del agua
		.timer = TIMER_B,
		.period = TIME_PERIOD2,
		.func_p = Notify2,
		.param_p = NULL};

	TimerInit(&timer_2); // inicializo timer 2

	TimerStart(timer_1.timer); // para que comience el timer 1
	TimerStart(timer_2.timer); // para que comience el timer 2

	HcSr04Init(GPIO_3, GPIO_2); // distancia

	GPIOInit(GPIO_BOMBA, GPIO_OUTPUT);
	GPIOInit(RESISTENCIA_CALEFACTORA, GPIO_OUTPUT);

    SwitchesInit();

	// ESTO CON INTERRUPCIONES
	SwitchActivInt(SWITCH_1, &FunctionStart, NULL);
	SwitchActivInt(SWITCH_2, &FunctionStop, NULL);

	// configuracion para entrada analogica
	analog_input_config_t analogInput1 = {
		.input = CH1,		// Se configura para leer del canal 1 del conversor analógico-digital (ADC)
		.mode = ADC_SINGLE, // Se configura para realizar una única lectura analógica
	};
	AnalogInputInit(&analogInput1); // Inicializa la entrada analógica utilizando la configuración proporcionada en analogInput1.

	// Para la uart
	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 115200, /*!< baudrate (bits per second) */
		.func_p = NULL,		 /*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/
		.param_p = NULL		 /*!< Pointer to callback function parameters */
	};
	UartInit(&my_uart);

	xTaskCreate(&Suministro_agua, "suministro de agua", 2048, NULL, 5, &task_handle1); // El puntero task_handle1 es una bandera se utiliza para almacenar el identificador de la tarea creada.
	xTaskCreate(&Control_temperatura, "control de temperatura", 2048, NULL, 5, &task_handle2);
}
/*==================[end of file]============================================*/