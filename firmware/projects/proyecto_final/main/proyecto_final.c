/*! @mainpage proyecto_final
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
 * | 10/10/2024 | Document creation		                         |
 *
 * @author Camila Perea (camila.perea@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/

#include <stdio.h>
#include <stdint.h>

#include "gpio_mcu.h"

#include "led.h"
#include "neopixel_stripe.h"
#include "ble_mcu.h"

/*==================[macros and definitions]=================================*/

#define BUILT_IN_RGB_LED_PIN          GPIO_8        /*> ESP32-C6-DevKitC-1 NeoPixel it's connected at GPIO_8 */
#define BUILT_IN_RGB_LED_LENGTH       1             /*> ESP32-C6-DevKitC-1 NeoPixel has one pixel */

#define NEOPIXEL_COLOR_WHITE          0x00FFFFFF  /*> Color white */
#define NEOPIXEL_COLOR_BLUE           0x000000FF  /*> Color blue */

/*==================[typedef]================================================*/

uint8_t data, leds;
typedef uint32_t neopixel_color_t;

/*==================[internal data definition]===============================*/

/**
 * @fn static void ControlLeds(void *pvParameter);

 * @brief 
 * @param[in] void *pvParameter
 * @return
 */
void ControlLeds(void *pvParameter);

/**
 * @fn static void BarridoCampoVisual(void *pvParameter);

 * @brief 
 * @param[in] void *pvParameter
 * @return
 */
void BarridoCampoVisual(void *pvParameter);

/*==================[internal functions declaration]=========================*/

void ControlLeds(void *pvParameter){

if(leds == true)
{
	NeoPixelInit;
}
else 
{
	data = leds;
	
}


}

void BarridoCampoVisual(void *pvParameter){

}

/*==================[external functions definition]==========================*/


void app_main(void){
	
	ble_config_t ble_configuration = {
        "ESP_EDU_1",
        ControlLeds
    };

	LedsInit();
    BleInit(&ble_configuration);

    /* Se inicializa el LED RGB de la placa */
    NeoPixelInit(BUILT_IN_RGB_LED_PIN, BUILT_IN_RGB_LED_LENGTH, &color);
    NeoPixelAllOff();
    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
                LedOff(LED_BT);
            break;
            case BLE_DISCONNECTED:
                LedToggle(LED_BT);
            break;
            case BLE_CONNECTED:
                LedOn(LED_BT);
            break;
        }
    }
}

/*==================[end of file]============================================*/