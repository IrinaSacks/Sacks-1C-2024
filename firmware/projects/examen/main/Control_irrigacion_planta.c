/*! @mainpage Irrigacion automatica de plantas
 *
 * @section genDesc Dispositivo basado en la ESP-EDU que permite controlar el riego y el pH de una plantera.
 *
 * El sistema inicia al apretar SWITCH 1 y se detiene al apretar el SWITCH 2.
 * De estar iniciado el sistema, cada 3 segundos mide el nivel de humedad y pH de la planta. 
 * Con un sensor de humedad mide la humedad presente en la tierra, y si es inferior a la necesaria se enciende 
 * la bomba peristaltica del agua.
 * Con un sensor analogico de pH, se mide el pH de la planta. Cuando el nivel es menor a 6 se enciende la bomba 
 * peristaltica de solución básica, y cuando es mayor a 6.7 enciende la bomba de la solución ácida.
 * Cada 5 segundos se informa mediante puerto serie el estado del sistema, incluyendo nivel pH y que bombas estan encendidas 
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	BOMBA_AGUA 	| 	GPIO_16		|
 * | 	BOMBA_pHA 	| 	GPIO_17		|
 * | 	BOMBA_pHB 	| 	GPIO_22		|
 * | 	SENSOR HUM 	| 	GPIO_23		|
 * | 	GND		 	| 	GND			|
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
#include "gpio_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
/*!
	@brief periodo del timerA en microseg, para la medicion de pH y humedad
*/
#define CONFIG_PERIOD_MEDICION 3000000 //3 seg
/*!
	@brief periodo del timerB en microseg, para  mostrar por puerto serie el estado del sistema
*/
#define CONFIG_PERIOD_MOSTRAR 5000000 // 5 seg
/*!
	@brief definicion GPIO para el Sensor de humedad
*/
#define SENSOR_H GPIO_23
/*!
	@brief definicion GPIO para la bomba peristaltica de agua
*/
#define BOMBA_A GPIO_16 
/*!
	@brief definicion GPIO para la bomba peristaltica de solucion de pH acida
*/
#define BOMBA_pHA GPIO_17 
/*!
	@brief definicion GPIO para la bomba peristaltica de solucion de pH basica
*/
#define BOMBA_pHB GPIO_22 
/*==================[internal data definition]===============================*/
TaskHandle_t control_agua_handle = NULL;

float nivel_pH;
uint8_t agua=false, inicio=false;
/*==================[internal functions declaration]=========================*/
/** @brief Timer que envia una notificaciones cada 3seg a ControlAgua y ControlpH
 * @return void
*/
void FuncTimerA_Med (void* param){
	vTaskNotifyGiveFromISR(control_agua_handle, pdFALSE);
}


/** @brief Interrupcion que cambia el estado de la variable inicio al apretar SWITCH 1
 * @return void
*/
void LeerTeclaOn(void){
	inicio=true;	
}

/** @brief Interrupcion que cambia el estado de la variable inicio al apretar SWITCH 2
 * @return void
*/
void LeerTeclaOff(void){
	inicio=false;
}

/** @brief Recibe una notificación cada 3 seg, si el sistema inicio lee el estado del sensor humedad y enciende o apaga la bomba de agua
 * @return void
*/
static void ControlAgua(void *pvParameter){ 
	uint8_t estado;
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     
		if (inicio){
			estado = GPIORead(SENSOR_H);
			if (estado){
				GPIOOn(BOMBA_A);
				agua = true;
			} 
			else {
				GPIOOff(BOMBA_A);
				agua=false;
			}
		}
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	SwitchesInit();

	GPIOInit(SENSOR_H, GPIO_INPUT);
	GPIOInit(BOMBA_A, GPIO_OUTPUT);
	GPIOInit(BOMBA_pHA, GPIO_OUTPUT);
	GPIOInit(BOMBA_pHB, GPIO_OUTPUT);

	timer_config_t timer_medicion = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_MEDICION,
        .func_p = FuncTimerA_Med,
        .param_p = NULL  
    };
	TimerInit(&timer_medicion);


	xTaskCreate(&ControlAgua, "Control_Agua", 2048, NULL, 5, &control_agua_handle);

	SwitchActivInt(SWITCH_1, &LeerTeclaOn, NULL);
	SwitchActivInt(SWITCH_2, &LeerTeclaOff, NULL);

	TimerStart(timer_medicion.timer);
}
/*==================[end of file]============================================*/
