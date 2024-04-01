/** @mainpage Obtencion por display de un dato de 32 bits
 *
 * @section genDesc General Description
 *
 * Este programa opera con un dato de 32 bits, lo convierte en BCD y lo decodifica a 7 segmentos 
 * para mostrar por display 3 digitos como maximo.
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
 * | 22/03/2024 | Document creation		                         |
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
/**
 * @brief estructura del tipo gpioConf_t
 * 	asigna el numero de pin y establece si es entrada o salida
*/
typedef struct 
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO digection '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
/** @brief Convierte un dato binario de 32 bits a BCD y almacena cada digito de salida en un arreglo
 * @param data numero de 32 bits
 * @param digits cantidad de digitos de salida del numero
 * @param bcd_number puntero a un arreglo donde se almacenan los n digitos
 * @return int8_t
*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t * bcd_number){
	for(int i=0; i<digits ; i++){
		bcd_number[digits-1-i]=data%10;
		data/=10;
	};
	return(0);
}
/** @brief Cambia el estado de cada pin según el estado del bit correspondiente en el BCD ingresado
 * @param dig_bcd digito del numero bcd
 * @param gpi almacena el estado de cada pin segun los bits del bcd ingresado
*/
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

/**
 * @brief Muestra por display el valor recibido
 * @param bin_data numero de 32 bits
 * @param cant_digit cantidad de digitos de salida del numero
 * @param gpio_dig almacena el estado de cada pin segun los bits del numero ingresado
 * @param gpio_pos mapea los puertos con el dígito del LCD a donde mostrar un dato
*/
void mostrarDisplay(uint32_t bin_data, uint8_t cant_digit, gpioConf_t *gpio_dig, gpioConf_t *gpio_pos){
	uint8_t num_bcd[cant_digit];
	convertToBcdArray(bin_data, cant_digit, num_bcd);
	for(int i=0; i<cant_digit; i++){
		cambiaEstado(num_bcd[i], gpio_dig);
		GPIOOn(gpio_pos[i].pin);
		GPIOOff(gpio_pos[i].pin);
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t numero_bin=123;
	uint8_t cant_digitos=3;

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

	gpioConf_t gpio_pos[3];
	gpio_pos[0].pin=GPIO_19;
	gpio_pos[1].pin=GPIO_18;
	gpio_pos[2].pin=GPIO_9;

	for(int i=0; i<3;i++){
		gpio_pos[i].dir=GPIO_OUTPUT;
	}
	
	for(int i=0; i<3;i++){
		GPIOInit(gpio_pos[i].pin, gpio_pos[i].dir);
	}

	mostrarDisplay(numero_bin, cant_digitos, gpio_dig, gpio_pos);

}
/*==================[end of file]============================================*/