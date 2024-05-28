/*! @mainpage Proyecto final de Electronica Programable
 *
 * @section genDesc General Description
 *
 *  Este programa implementa conexión BLE entre la ESP-EDU y una aplicación en el celular 
 * que permite controlar el encendido, apagado y pausa de un Robot de guiado automatico, 
 * asi como visualizar y recibir alertas sobre el nivel de batería del mismo.
 * El Robot de guiado automatico, implementa sensores de ultrasonido y motores DC para seguir una línea 
 * negra en el piso.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	B-1A	 	| 	GPIO_16		|
 * | 	B-1B	 	| 	GPIO_17		|
 * | 	A-1A	 	| 	GPIO_22		|
 * | 	A-1B	 	| 	GPIO_23		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	OUT	 		| 	GPIO_18		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	OUT	 		| 	GPIO_19		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND		 	| 	GND			|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 16/05/2024 | Document creation		                         |
 *
 * @author Sacks Irina
 *
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "ble_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 500
/*==================[internal data definition]===============================*/
bool encendido, pausa = false;
char nivel_bateria[50]="5";
/*==================[internal functions declaration]=========================*/
void read_data(uint8_t * data, uint8_t length){
	//Si el boton esta en On ('O') el robot debe andar (encendido es True)
	//Si el boton esta en Off ('o') el robot no debe andar (encendido es False)
	//Al apretar boton de pausa ('P') el robot debe frenar (pausa = true)
	//Si el boton pausa es soltado ('p') el robot debe continuar normal (pausa = false)
	switch(data[0]){
        case 'O':
            encendido = true;
			BleSendString("RR0G0B0*"); //Apaga el led rojo en la app del celular
            BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			BleSendString("*VR0G255B0*"); //Enciende led verde en la app del celular
			BleSendString("*MRobot en marcha\n*"); //Envio mensaje al monitor del estado robot en la app del celular
            break;
        case 'o':
            encendido = false;
			BleSendString("VR0G0B0*"); //Apaga el led verde en la app del celular
            BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			BleSendString("*RR255G0B0*"); //Enciende led rojo en la app del celular
			BleSendString("*MRobot estacionado\n*"); //Envio mensaje al monitor del estado robot en la app del celular
            break;
		//El mecanismo de la pausa deberia funcionar solo si el robot fue encendido, deberia ir abajo creo
		case 'P':
            if(encendido == true){
			    pausa = true;
                BleSendString("RR0G0B0*"); //Apaga el led rojo en la app del celular
			    BleSendString("*AR222G206B42*"); //Enciende led amarillo en la app del celular
			    BleSendString("*MRobot pausado\n*"); //Envio mensaje al monitor del estado robot en la app del celular
			    //Aca deberia llamar a la funcion que frena el robot
            }
			break;
		case 'p':
            if(encendido == true){
			    pausa = false;
			    BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			    //Aca no deberia de llamar a ninguna funcion porque deberian reanudarse el control de los detectores de linea
				BleSendString("*MRobot reanudado\n*");
			}
            break;
    }
	
	//Si el boton esta en On 
	if (encendido == true){
        BleSendString("RR0G0B0*"); //Apaga el led rojo en la app del celular
		BleSendString("*VR0G255B0*"); //Led verde
        char aux_bat[50];
        aux_bat[0] = '\0';
        strcat(aux_bat, "*B");
        strcat(aux_bat, "El nivel de bateria es de: ");
        strcat(aux_bat, nivel_bateria);
        strcat(aux_bat, "%\n*");
		BleSendString(aux_bat); //Envio el nivel de la bateria en porcentaje al monitor en la app del celular
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
    //Inicializo los LEDS
    LedsInit();

    //Inicializo Bluetooth
    ble_config_t ble_configuration = {
        "ROBOT_LINEA_IRINA",
        read_data
    };
    BleInit(&ble_configuration);

    //Leds cambian segun el estado del BLE: 
	//Conectado prende led verde, si se desconecta parpadea el led amarillo y si se apaga el led rojo
    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
				LedOff(LED_1);
				LedOff(LED_2);
                LedOn(LED_3);
            break;
            case BLE_DISCONNECTED:
                LedOff(LED_1);
                LedToggle(LED_2);
				LedOff(LED_3);
            break;
            case BLE_CONNECTED:
				LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            break;
        }
    }

}