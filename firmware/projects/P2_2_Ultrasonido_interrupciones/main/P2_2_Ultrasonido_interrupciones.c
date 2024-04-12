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
#include "hc_sr04.h"
#include "led.h"
#include "switch.h"
#include "lcditse0803.h"
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
#define refresco 1000
#define CONFIG_PERIOD_DIS 1000000
/*==================[internal data definition]===============================*/
TaskHandle_t leer_distancia_handle = NULL;
TaskHandle_t mostrar_distancia_handle = NULL;

uint16_t distancia;
bool control=false;
bool pausa=false;
/*==================[internal functions declaration]=========================*/
void FuncTimerA_Distancia (void* param){
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
}

void voometro(uint16_t dis_cm){
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

static void LeerDistancia(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
       		distancia = HcSr04ReadDistanceInCentimeters();
		}
    }
}

static void MostrarDistancia(void *pvParameter){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
			if(!pausa){
				LcdItsE0803Write(distancia);
			}
			voometro(distancia);
		}
		else{
			LedsOffAll();
			LcdItsE0803Off();
		}
	}
}

void LeerTeclaOnOff(void){
	control=!control;	
}

void LeerTeclaHold(void){
	pausa=!pausa;
}

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