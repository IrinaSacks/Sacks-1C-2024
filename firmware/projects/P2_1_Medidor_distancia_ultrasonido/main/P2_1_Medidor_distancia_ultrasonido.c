/*! @mainpage Medidor de distancia por ultrasonido y mostrado por display
 *
 * @section genDesc General Description
 *
 * Este programa, mediante Tareas, controla si se apreto SWITCH 1 o 2, mide la distancia en centimetros utilizando un sensor de ultrasonido y la muestra por un display. 
 * Además permite, mediante interrupciones, detener o encencer las mediciones al apretar SWITCH_1 o pausar la actualizacion del display al apretar SWITCH_2 
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
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
#include "hc_sr04.h"
#include "led.h"
#include "switch.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
/** 
	@brief refresco periodo de 1 segundo para el Delay
*/
#define refresco 1000
/**
	@brief refresco_tecla periodo de 100 mseg para el Delay usado para leer si se apreto un switch
*/
#define refresco_tecla 100
/*==================[internal data definition]===============================*/
/*! 
	@brief distancia Almacena el valor de la distancia medida por el sensor de ultrasonido en cm
*/
uint16_t distancia;
/**
	@brief control variable que indica True cuando hay que realizar mediciones y mostrarlas por display de lo contrario es falsa
*/
bool control=true;
/*! 
	@brief pausa variable que indica True si los datos mostrados por display no deben actualizarse
*/
bool pausa=false;
/*==================[internal functions declaration]=========================*/
/** @brief Prende o apaga determinados leds segun el valor de la distancia medida en cm a la que esta un objeto
 * @param dis_cm distancia en cm medida por sensor de ultrasonido
 * @return void
*/
void vumetro(uint16_t dis_cm){
	if(dis_cm < 10){
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if ((dis_cm > 10) && (dis_cm < 20)){
		LedOn(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
	}
	else if((dis_cm > 20) && (dis_cm < 30)){
		LedOn(LED_1);
		LedOn(LED_2);
		LedOff(LED_3);
	}
	else if((dis_cm > 30)){
		LedOn(LED_1);
		LedOn(LED_2);
		LedOn(LED_3);
	}
}

/** @brief Lee y almacena la distancia cada 1 segundo 
 * @param *pvParameter
 * @return static void
*/
static void LeerDistancia(void *pvParameter){
    while(1){
		if(control){
       		distancia = HcSr04ReadDistanceInCentimeters();
		}
		vTaskDelay(refresco/portTICK_PERIOD_MS);
    }
}

/** @brief lee el switch apretado cada 0.1 segundo y segun esto modifica variables booleanas
 * @param *pvParameter 
 * @return static void
*/
static void LeerTecla(void *pvParameter){
    uint8_t teclas;
	while(1){
        teclas  = SwitchesRead();
		switch(teclas){
    		case SWITCH_1:
				control=!control;
    		break;
    		case SWITCH_2:
    			pausa=!pausa;
    		break;
    	}
    vTaskDelay(refresco_tecla/portTICK_PERIOD_MS);
	}
}

/** @brief Actualiza cada 1 segundo la distancia en el display si no se apreto el switch 2, y tambien actualiza el vumetro 
 * @param *pvParameter 
 * @return static void
*/
static void MostrarDistancia(void *pvParameter){
	while(1){
		if(control){
			if(!pausa){
				LcdItsE0803Write(distancia);
			}
			vumetro(distancia);
		}
		vTaskDelay(refresco/portTICK_PERIOD_MS);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);

	xTaskCreate(&LeerTecla, "LeerTecla", 512, NULL, 5, NULL);
	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, NULL);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, NULL);

}
/*==================[end of file]============================================*/