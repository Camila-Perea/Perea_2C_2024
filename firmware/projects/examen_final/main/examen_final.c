/*! @mainpage EXAMEN FINAL - Alimentador Automatico de Mascotas
 *
 * @section genDesc General Description
 *
 * Se pretende diseñar un dispositivo basado en la ESP-EDU que se utilizará para 
 * suministrar alimento y agua a una mascota.
 * El programa utiliza un sensor HC_SR04 para medir distancia y así obtener el volumen de agua en un recipiente, 
 * Por medio de una balanza analógica y un conversor ADC se obtiene el peso de alimento de dicho recipiente. 
 * Informa a traves de UART el volumen de agua y el peso de alimento. 
 * El sistema se inicia o detiene con la tecla 1 y se indica con el LED_1 cuando esta encendido.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |    ESP32   	|
 * |:--------------:|:--------------|
 * |  HC-SR04 Trig  |    GPIO_3     |
 * |  HC-SR04 Echo  |    GPIO_2     |
 * |      ADC       |     CH0       |
 * |  GPIO_SWITCH1  |    GPIO_4     |
 * |     LED_1      |    GPIO_11    |
 * |   GPIO_AGUA    |    GPIO_20    |
 * | GPIO_ALIMENTO  |    GPIO_21    |
 * 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 16/12/2025 | Document creation		                         |
 *
 * @author Camila Perea (camila.perea@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "hc_sr04.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
#include "gpio_mcu.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
/**Tiempo en el que se debe realizar una medicion de la cantidad de agua y alimento */
#define TIME_MEDICION     5000000 // 5 segundos 

#define GPIO_AGUA          GPIO_20
#define GPIO_ALIMENTO      GPIO_21

#define DISTANCIA_LLENO    20    // distancia del sensor a los 2500 cm3 (7.9 cm de altura)
#define DISTANCIA_VACIO    27    // distancia del sensor a 1000 cm3 (3.18 cm de altura)

/*==================[internal data definition]===============================*/
static bool sistema_on = false;
static uint16_t distancia_cm;
static uint16_t peso_alimento_g;
static uint16_t adc_alimento_mV;
static uint16_t volumen_cm3;

/*==================[task handle]============================================*/
TaskHandle_t task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/** @fn  void Notify(void *param)
 * @brief  Notifica a la tareas task_handle
 * @param *param
 */
void Notify(void *param)
{
	vTaskNotifyGiveFromISR(task_handle, pdFALSE);
}

/** @fn  void Tecla1_ISR(void *param)
 * @brief  Función que alterna el estado del sistema (ON/OFF) al presionar la tecla 1
 * @param *param
 */
void Tecla1_ISR(void)
{
    sistema_on = !sistema_on;
}

/** @fn  void TareaControlAguaYAlimento(void *param)
 * @brief  Tarea que controla la cantidad de agua y de alimento 
 * @param *param
 */
static void TareaControlAguaYAlimento(void *pvParameter)
{
    while (1)
    {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (sistema_on)
        {
            LedOn(LED_1);

            /* ----------- CONTROL DE AGUA ----------- */
            distancia_cm = HcSr04ReadDistanceInCentimeters();

            if (distancia_cm > DISTANCIA_VACIO)
            {
                GPIOOn(GPIO_AGUA);   // Falta agua, activo la electrovalvula
            }
            else if (distancia_cm < DISTANCIA_LLENO)
            {
                GPIOOff(GPIO_AGUA);  // Recipiente lleno, desactivo la electrovalvula
            }

            /* ----------- CONTROL DE ALIMENTO ----------- */
            AnalogInputReadSingle(CH0, &adc_alimento_mV);

			// Conversion de mV a gramos
            peso_alimento_g = (adc_alimento_mV * 1000) / 3300;

            if (peso_alimento_g < 50)
            {
                GPIOOn(GPIO_ALIMENTO);
            }
            else if (peso_alimento_g > 500)
            {
                GPIOOff(GPIO_ALIMENTO);
            }
			
			// Conversion a cm3 para la lectura por uart
			volumen_cm3 = 314 * (30 - distancia_cm);

            /* ----------- INFORME POR UART ----------- */
            UartSendString(UART_PC, "Agua: ");
            UartSendString(UART_PC, (char *)UartItoa(volumen_cm3, 10));
            UartSendString(UART_PC, " cm3, Alimento: ");
            UartSendString(UART_PC, (char *)UartItoa(peso_alimento_g, 10));
            UartSendString(UART_PC, " gr\r\n");
        }
        else
        {
            LedOff(LED_1);
            GPIOOff(GPIO_AGUA);
            GPIOOff(GPIO_ALIMENTO);
        }

    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    /* Inicializaciones */
    HcSr04Init(GPIO_3, GPIO_2);

    GPIOInit(GPIO_AGUA, GPIO_OUTPUT);
    GPIOInit(GPIO_ALIMENTO, GPIO_OUTPUT);

    LedsInit();

    SwitchesInit();
    SwitchActivInt(SWITCH_1, Tecla1_ISR, NULL);

    analog_input_config_t adc_config = {
        .input = CH0,
        .mode = ADC_SINGLE
    };
    AnalogInputInit(&adc_config);

    serial_config_t uart_config = {
        .port = UART_PC,
        .baud_rate = 9600,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&uart_config);

	timer_config_t timer = {
		.timer = TIMER_A,
		.period = TIME_MEDICION,
		.func_p = Notify,
		.param_p = NULL};

	TimerInit(&timer); // inicializo timer 
	TimerStart(timer.timer); 
		
    xTaskCreate(TareaControlAguaYAlimento,
        "Control Agua y Alimento",
        2048,
        NULL,
        5,
        &task_handle
    );
}
/*==================[end of file]============================================*/