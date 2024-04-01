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
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t * bcd_number){
	for(int i=0; i<digits ; i++){
		bcd_number[digits-1-i]=data%10;
		data/=10;
	};
	return(0);
}

/*==================[external functions definition]==========================*/
void app_main(void){
	uint32_t numero_bin=123;
	uint8_t cant_digitos=3;
	uint8_t num_bcd[cant_digitos];
	convertToBcdArray(numero_bin, cant_digitos, num_bcd);

	for(int i=0; i<cant_digitos; i++){
		printf("BCD[%d]: %d\n", i, num_bcd[i]);
	}
	while(1);

}
/*==================[end of file]============================================*/