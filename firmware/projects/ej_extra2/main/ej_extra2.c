/*! @mainpage Sensor de bicicletas
 *
 * @section El programa desarrollado es una alerta para ciclirtas
 * Posee un sensor HC-SR04 el cual mide la distancia de los vehículos a la bicicleta y
 * enciende determinados leds y una alarma sonora dependiendo de la distancia
 * Posee un acelerómetro analogico en el casco, el programa suma la aceleración en los 3 ejes (canales)
 * y detecta si hubo una caída
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	Buzzer	 	| 	GPIO_17		|
 * |  acelerometro	| 	  CH1		|
 * |  acelerometro	| 	  CH2		|
 * |  acelerometro	| 	  CH3		|
 * | 	TRIGGER	 	| 	 GPIO_2		|
 * |     ECHO       |    GPIO_3     |
 * |   	 +5V 	    | 	 +5V		|
 * |   	 GND 	    | 	 GND		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 4/11/2024 | Document creation		                         |
 *
 * @author Camila Perea (camila.pereaa@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <led.h>
#include "hc_sr04.h"
#include "gpio_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "switch.h"
#include "timer_mcu.h"
#include "ble_mcu.h"
#include "buzzer.h"
/*==================[macros and definitions]=================================*/
/**Tiempo al que quiero que se realice el sensado */
#define CONFIG_BLINK_PERIOD_SENSADO 500
/**Tiempo al que quiero que se realice el SENSADO DEL ACELEROMETRO */
#define CONFIG_BLINK_PERIOD_ACELEROMETRO 10
/**Defino GPIO de la placa para el buzzer*/
#define GPIO_ALARMA GPIO_17

/*==================[internal data definition]===============================*/
/**Variable que almacena el valor que mide de distancia.*/
uint16_t distancia;
/**Me guarda el valor de  voltaje */
uint16_t valor_sensado;
/**Me guarda el valor G que obtengo realizando una regla de 3 simples con el voltaje que mide el sensor*/
uint16_t data_X, data_Y, data_Z;
/**Me guarda el valor escalar de la suma de la aceleración el los 3 ejes*/
uint16_t suma;
/**Si es TRUE enciende el sensor y el acelerometro, si es FALSE lo apaga.*/
bool on=true;

TaskHandle_t medir_distancia_task_handle = NULL;
TaskHandle_t acelerometro_task_handle = NULL;
TaskHandle_t buzzer_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
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
 * @fn void MedirDistancia(void *pvParameter);
 * @brief Escribe en distancia lo que sensa.
 * @param[in] 
 * @return 
*/
void MedirDistancia(void *pvParameter);

/**
 * @fn void Leds(void *pvParameter);
 * @brief Prende o apaga los leds dependiendo que distancia este midiendo el sensor
 * LLama al GPIO correspondiente para encender el buzzer
 * @param[in] 
 * @return 
*/
void Leds(void *pvParameter);

/**
 * @fn void Buzzer(void *pvParameter);
 * @brief LLama al GPIO correspondiente para encender el buzzer
 * @param[in] 
 * @return 
*/
void Buzzer(void *pvParameter);

/**
 * @fn void Acelerometro(void *pvParameter);
 * @brief Toma los datos analogicos del acelerometro en los 3 canales, cambia el voltale a G y los suma 
 * @param[in] 
 * @return 
*/
void Acelerometro(void *pvParameter);

/**
 * @fn void MostrarMensajeTask();
 * @brief Tarea para mostrar los mensajes por la uart
 * @param[in] 
 * @return 
*/
void MostrarMensaje(uint8_t mensaje);

/*==================[external functions definition]==========================*/

void FuncTimerA(void* param)
{
	vTaskNotifyGiveFromISR(medir_distancia_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE);
	vTaskNotifyGiveFromISR(buzzer_task_handle, pdFALSE);

}

void FuncTimerB(void* param)
{
    vTaskNotifyGiveFromISR(acelerometro_task_handle, pdFALSE);    /* Envía una notificación a la tarea asociada al LED_2 */
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

void Leds(void *pvParameter)
{
	while(1)
	{ 
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(distancia>5000)
		{
			LedOn(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		else if(distancia>=3000 && distancia<5000)
		{ 
			LedOn(LED_1);
			LedOn(LED_2);
			LedOff(LED_3);
			MostrarMensaje(1); //mensaje de precaución 
		}
		else if(distancia<3000)
		{ 
			LedOn(LED_1);
			LedOn(LED_2);
			LedOn(LED_3); 
			MostrarMensaje(2); //mensaje de peligro
		}
    }
}

void Buzzer(void *pvParameter)
{
static bool estado = false;

	while(1)
	{ 
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(distancia >= 3000 && distancia < 5000)
		{
			// Precaución: toggle cada 1 s
			estado = !estado;
			if(estado)
				BuzzerOn();
			else
				BuzzerOff();
		}
		else if(distancia < 3000)
		{
			// Peligro: toggle cada 0.5 s
			estado = !estado;
			if(estado)
				BuzzerOn();
			else
				BuzzerOff();
		}
		else
		{
			// Distancia segura
			estado = false;
			BuzzerOff();
		}
    }
}
void Acelerometro(void *pvParameter)
{
	while(1) 
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (on == true)
		{
			AnalogInputReadSingle(CH1, &valor_sensado);
			data_X = (valor_sensado - 1.65)/300; //convierto Voltaje a G
			AnalogInputReadSingle(CH2, &valor_sensado);
			data_Y = (valor_sensado - 1.65)/300; //convierto Voltaje a G
			AnalogInputReadSingle(CH3, &valor_sensado);
			data_Z = (valor_sensado - 1.65)/300; //convierto Voltaje a G
		
			suma = data_X + data_Y + data_Z;
		}

		if(suma > 4)
		{
			MostrarMensaje(3);
		}

	}
	
}

void MostrarMensaje(uint8_t mensaje)
{ 
	if(on)
	{
		switch (mensaje)
			{
			case 1:
				UartSendString(UART_CONNECTOR, "Precaución, vehículo cerca, \r\n");
				UartSendString(UART_CONNECTOR, " \r");

				break;
			case 2:
				UartSendString(UART_CONNECTOR, "Peligro, vehículo cerca, \r\n");
				UartSendString(UART_CONNECTOR, " \r");

				break;
			case 3:
				UartSendString(UART_CONNECTOR, "Caída detectada\r\n");
				UartSendString(UART_CONNECTOR, " \r");

				break;
			default:
				break;
			}
	}
	
}

void app_main(void){

analog_input_config_t eje_x = {
		.input = CH1,
		.mode = ADC_SINGLE,
	};
	
analog_input_config_t eje_y = {
		.input = CH2,
		.mode = ADC_SINGLE,
	};

analog_input_config_t eje_z = {
		.input = CH3,
		.mode = ADC_SINGLE,
	};

serial_config_t pantalla = 
	{
		.port = UART_CONNECTOR,
		.baud_rate = 9600,
		.func_p = NULL,
		.param_p = NULL,
	};


/* Inicialización de timers */
	timer_config_t timer_distancia = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_SENSADO,
        .func_p = FuncTimerA,
        .param_p = NULL
    };

    timer_config_t timer_acelerometro = {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD_ACELEROMETRO,
        .func_p = FuncTimerB,
        .param_p = NULL
    };

LedsInit();
UartInit(&pantalla);
TimerInit(&timer_distancia);
TimerInit(&timer_acelerometro);
AnalogInputInit(&eje_x);
AnalogInputInit(&eje_y);
AnalogInputInit(&eje_z);
BuzzerInit(GPIO_ALARMA);
HcSr04Init(GPIO_3, GPIO_2);

xTaskCreate(&MedirDistancia, "MedirDistancia", 512, NULL, 5, &medir_distancia_task_handle);
xTaskCreate(&Acelerometro, "Acelerometro", 512, NULL, 5, &acelerometro_task_handle);
xTaskCreate(&Leds, "Leds", 512, NULL, 5, &leds_task_handle);
xTaskCreate(&Buzzer, "Buzzer", 512, NULL, 5, &buzzer_task_handle);


/* Inicialización del conteo de timers */
	TimerStart(timer_distancia.timer);
    TimerStart(timer_acelerometro.timer);

}
/*==================[end of file]============================================*/