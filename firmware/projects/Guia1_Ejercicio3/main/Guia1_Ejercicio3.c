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
 * @author Sacks Irina
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define ON 1
#define OFF 0
#define TOGGLE 8
/*==================[internal data definition]===============================*/
struct leds
{
	uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
}; 

/*==================[internal functions declaration]=========================*/
void Control_Led(struct leds *my_leds){
	switch(my_leds->mode){
		case ON:
			LedOn(my_leds->n_led);
			break;
		case OFF:
			LedOff(my_leds->n_led);
			break;
		case TOGGLE:
			int i=0;
			while(i<my_leds->n_ciclos){
				LedToggle(my_leds->n_led);
				for(int j=0; j<my_leds->periodo/100; j++){
					vTaskDelay(100 / portTICK_PERIOD_MS);
				}
				i++;
			}
			break;
		default:
			break;
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	uint8_t teclas;
	SwitchesInit();
	LedsInit();

	struct leds LED;
	LED.mode=TOGGLE;
	LED.n_ciclos=100;
	LED.periodo=500;
	LED.n_led=LED_2;

	Control_Led(&LED);	
	
}
/*==================[end of file]============================================*/