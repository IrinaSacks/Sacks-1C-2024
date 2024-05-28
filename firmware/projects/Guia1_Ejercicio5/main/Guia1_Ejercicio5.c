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
#include "gpio_mcu.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct 
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO digection '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t * bcd_number){
	for(int i=0; i<digits ; i++){
		bcd_number[digits-1-i]=data%10;
		data/=10;
	};
	return(0);
}

void cambiaEstado(int dig_bcd, gpioConf_t *gpi){
	for(int i=0; i<4; i++){
		if(dig_bcd & (1<<i)){
		GPIOOn(gpi[i].pin);
		}
		else{
		GPIOOff(gpi[i].pin);
		}
	}
};

/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t numero_bin=123;
	uint8_t cant_digitos=3;
	uint8_t num_bcd[cant_digitos];
	convertToBcdArray(numero_bin, cant_digitos, num_bcd);

	gpioConf_t gpio_dig[4];
	gpio_dig[0].pin=GPIO_20;
	gpio_dig[1].pin=GPIO_21;
	gpio_dig[2].pin=GPIO_22;
	gpio_dig[3].pin=GPIO_23;

	for(int i=0; i<4;i++){
		gpio_dig[i].dir=GPIO_OUTPUT;
	}
	
	for(int i=0; i<4;i++){
		GPIOInit(gpio_dig[i].pin, gpio_dig[i].dir);
	}

	for(int i=0; i<cant_digitos; i++){
		cambiaEstado(num_bcd[i], gpio_dig);
	}

}
/*==================[end of file]============================================*/