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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hc_sr04.h"
#include "led.h"
#include "switch.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
#define refresco 1000
#define refresco_tecla 100
/*==================[internal data definition]===============================*/
uint16_t distancia;
bool control=true;
bool pausa=false;
/*==================[internal functions declaration]=========================*/
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
		if(control){
       		distancia = HcSr04ReadDistanceInCentimeters();
			vTaskDelay(refresco/portTICK_PERIOD_MS);
		}
    }
}

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

static void MostrarDistancia(void *pvParameter){
	while(1){
		if(control){
			if(!pausa){
				LcdItsE0803Write(distancia);
				vTaskDelay(refresco/portTICK_PERIOD_MS);
			}
			voometro(distancia);
		}
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