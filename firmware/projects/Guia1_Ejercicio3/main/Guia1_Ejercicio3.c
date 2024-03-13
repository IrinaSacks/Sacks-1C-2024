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
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define ON 000000001
#define OFF 00000000
#define TOGGLE 00000010
/*==================[internal data definition]===============================*/
struct leds
{
	uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de encendido/apagado
	uint8_t periodo;    //indica el tiempo de cada ciclo
	
} my_leds; 
/*==================[internal functions declaration]=========================*/
void Control_Led(struct leds *my_leds){

	switch(my_leds->mode){
		case ON:
			switch(my_leds->n_led){
				case 1:
					LedOn(LED_1);
				case 2:
					LedOn(LED_2);
				case 3:
					LedOn(LED_3);
			}
		case OFF:
			switch(my_leds->n_led){
				case 1:
					LedOff(LED_1);
				case 2:
					LedOff(LED_2);
				case 3:
					LedOff(LED_3);
			}
		case TOGGLE:

	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	
}
/*==================[end of file]============================================*/