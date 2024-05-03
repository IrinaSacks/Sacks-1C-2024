/*! @mainpage Medidor de distancia por ultrasonido y mostrado por display usando interrupciones
 *
 * @section genDesc General Description
 *
 * Este programa, mediante Tareas y timer, mide la distancia en centimetros utilizando un sensor de ultrasonido y la muestra por un display. 
 * Además permite, mediante interrupciones, leer si se apreto un SWITCH y detener/encencer las mediciones si fue el SWITCH_1, o pausar la actualizacion del display si fue el SWITCH_2 
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
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
/*! 
	@brief periodo del timerA en microsegundos
*/
#define CONFIG_PERIOD_DIS 1000000
/*==================[internal data definition]===============================*/
TaskHandle_t leer_distancia_handle = NULL;
TaskHandle_t mostrar_distancia_handle = NULL;
/*! 
	@brief Almacena el valor de la distancia medida por el sensor de ultrasonido en cm
*/
uint16_t distancia;
/*! 
	@brief variable que indica True cuando hay que realizar mediciones y mostrarlas por display de lo contrario es falsa
*/
bool control=true;
/*! 
	@brief variable que indica True si los datos mostrados por display no deben actualizarse
*/
bool pausa=false;
/*==================[internal functions declaration]=========================*/
/** @brief Timer que envia dos notificaciones cada 1 segundo a LeerDistancia y MostrarDistancia
 * @return void
*/
void FuncTimerA_Distancia (void* param){
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
}

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

/** @brief Recibe una notificación, luego lee y almacena la distancia cada 1 segundo 
 * @return static void
*/
static void LeerDistancia(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
       		distancia = HcSr04ReadDistanceInCentimeters();
		}
    }
}

/** Recibe una notificación y actualiza cada 1 segundo el display y los leds. Si el SWITCH 2 fue apretado el display no se actualiza
 * @return static void
*/
static void MostrarDistancia(void *pvParameter){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
			if(!pausa){
				LcdItsE0803Write(distancia);
			}
			vumetro(distancia);
		}
		else{
			LedsOffAll();
			LcdItsE0803Off();
		}
	}
}

/** @brief cambia el estado de la variable control
 * @return void
*/
void LeerTeclaOnOff(void){
	control=!control;	
}

/** @brief cambia el estado de la variable pausa
 * @return void
*/
void LeerTeclaHold(void){
	pausa=!pausa;
}

/** @brief modifica variables control o pausa segun el switch que fue apretado
 * @param tecla switch apretado
 * @return void
*/
void LeerTecla(switch_t tecla){
	switch(tecla){
		case SWITCH_1:
			control=!control;
		break;
		case SWITCH_2:
			pausa=!pausa;
		break;
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);

	timer_config_t timer_distancia = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_DIS,
        .func_p = FuncTimerA_Distancia,
        .param_p = NULL
    };

	TimerInit(&timer_distancia);

	SwitchActivInt(SWITCH_1, &LeerTeclaOnOff, NULL);
	SwitchActivInt(SWITCH_2, &LeerTeclaHold, NULL);
	//SwitchActivInt(SWITCH_1, &LeerTecla, SWITCH_1); Sirve para hacer una sola funcion, el de abajo igual 
	//SwitchActivInt(SWITCH_2, &LeerTecla, SWITCH_2);

	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, &leer_distancia_handle);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, &mostrar_distancia_handle);
	
	TimerStart(timer_distancia.timer);
}
/*==================[end of file]============================================*/