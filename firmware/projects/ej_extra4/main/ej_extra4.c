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
 * |     ESP32      |   Peripheral 	|
 * |:--------------:|:--------------|
 * | 	Bomba agua 	| 	GPIO_15		|
 * | 	Bomba pHA 	| 	GPIO_16		|
 * | 	Bomba pHB 	| 	GPIO_17		|
 * | Sensor humedad | 	GPIO_18		|
 * | 	Sensor pH 	| 	CH1 		|
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
#include "gpio_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "switch.h"

/*==================[macros and definitions]=================================*/
#define TIME_CONTROL_US   3000000   // 3 s
#define TIME_REPORT_US    5000000   // 5 s

#define GPIO_BOMBA_AGUA   GPIO_15
#define GPIO_BOMBA_PHA    GPIO_16
#define GPIO_BOMBA_PHB    GPIO_17
#define GPIO_SENSOR_HUM   GPIO_18

#define PH_MIN  6.0
#define PH_MAX  6.7

/*==================[global variables]=======================================*/
TaskHandle_t control_task_handle = NULL;
TaskHandle_t report_task_handle  = NULL;

bool sistema_encendido = false;

float ph_actual = 0.0;
bool humedad_ok = true;

bool bomba_agua_on = false;
bool bomba_pHA_on  = false;
bool bomba_pHB_on  = false;

/*==================[functions]==============================================*/
float ConvertirVoltajeAPh(uint16_t mv)
{
    return (mv / 3000.0f) * 14.0f;
}

/*==================[Timers ISR]=============================================*/
void TimerControlISR(void *param)
{
    vTaskNotifyGiveFromISR(control_task_handle, pdFALSE);
}

void TimerReportISR(void *param)
{
    vTaskNotifyGiveFromISR(report_task_handle, pdFALSE);
}

/*==================[Tasks]==================================================*/
void ControlTask(void *pvParameter)
{
    uint16_t ph_mv;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!sistema_encendido)
        {
            GPIOOff(GPIO_BOMBA_AGUA);
            GPIOOff(GPIO_BOMBA_PHA);
            GPIOOff(GPIO_BOMBA_PHB);
            continue;
        }

        /* ---- HUMEDAD ---- */
        humedad_ok = GPIORead(GPIO_SENSOR_HUM);

        if (!humedad_ok)
        {
            GPIOOn(GPIO_BOMBA_AGUA);
            bomba_agua_on = true;
        }
        else
        {
            GPIOOff(GPIO_BOMBA_AGUA);
            bomba_agua_on = false;
        }

        /* ---- PH ---- */
        AnalogInputReadSingle(CH1, &ph_mv);
        ph_actual = ConvertirVoltajeAPh(ph_mv);

        bomba_pHA_on = false;
        bomba_pHB_on = false;

        if (ph_actual < PH_MIN)
        {
            GPIOOn(GPIO_BOMBA_PHB);
            GPIOOff(GPIO_BOMBA_PHA);
            bomba_pHB_on = true;
        }
        else if (ph_actual > PH_MAX)
        {
            GPIOOn(GPIO_BOMBA_PHA);
            GPIOOff(GPIO_BOMBA_PHB);
            bomba_pHA_on = true;
        }
        else
        {
            GPIOOff(GPIO_BOMBA_PHA);
            GPIOOff(GPIO_BOMBA_PHB);
        }
    }
}

void ReportTask(void *pvParameter)
{
    char buffer[64];

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!sistema_encendido)
            continue;

        sprintf(buffer, "pH: %.2f, humedad %s\r\n",
                ph_actual,
                humedad_ok ? "correcta" : "incorrecta");
        UartSendString(UART_PC, buffer);

        if (bomba_agua_on)
            UartSendString(UART_PC, "Bomba de agua encendida\r\n");

        if (bomba_pHA_on)
            UartSendString(UART_PC, "Bomba pHA encendida\r\n");

        if (bomba_pHB_on)
            UartSendString(UART_PC, "Bomba pHB encendida\r\n");
    }
}

/*==================[Teclas]=================================================*/
void StartSystem(void)
{
    sistema_encendido = true;
}

void StopSystem(void)
{
    sistema_encendido = false;
}

/*==================[main]===================================================*/
void app_main(void)
{
    /* GPIO */
    GPIOInit(GPIO_BOMBA_AGUA, GPIO_OUTPUT);
    GPIOInit(GPIO_BOMBA_PHA, GPIO_OUTPUT);
    GPIOInit(GPIO_BOMBA_PHB, GPIO_OUTPUT);
    GPIOInit(GPIO_SENSOR_HUM, GPIO_INPUT);

    /* ADC */
    analog_input_config_t adc = {
        .input = CH1,
        .mode = ADC_SINGLE
    };
    AnalogInputInit(&adc);

    /* UART */
    serial_config_t uart = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&uart);

    /* Switches */
    SwitchesInit();
    SwitchActivInt(SWITCH_1, StartSystem, NULL);
    SwitchActivInt(SWITCH_2, StopSystem, NULL);

    /* Timers */
    timer_config_t timer_control = {
        .timer = TIMER_A,
        .period = TIME_CONTROL_US,
        .func_p = TimerControlISR
    };
    TimerInit(&timer_control);

    timer_config_t timer_report = {
        .timer = TIMER_B,
        .period = TIME_REPORT_US,
        .func_p = TimerReportISR
    };
    TimerInit(&timer_report);

    /* Tasks */
    xTaskCreate(ControlTask, "ControlTask", 2048, NULL, 5, &control_task_handle);
    xTaskCreate(ReportTask, "ReportTask", 2048, NULL, 4, &report_task_handle);

    TimerStart(timer_control.timer);
    TimerStart(timer_report.timer);
}

/*==================[end of file]============================================*/