/*! @mainpage Medidor de distancia por ultrasonido y mostrado por display usando interrupciones y puerto serie
 *
 * @section genDesc General Description
 *
 * Este programa, mediante Tareas y timer, mide la distancia en centimetros utilizando un sensor de ultrasonido y la muestra por un display. 
 * Además permite, mediante interrupciones, leer si se apreto un SWITCH y detener/encencer las mediciones si fue el SWITCH_1, o pausar la actualizacion del display si fue el SWITCH_2. 
 * Incluye conexion de puerto serie con la pc, de forma que pueden visualizarse las mediciones en la pantalla. Y que al apretar las teclas 'H' y 'O', se pueda pausar la actualizacion del display o apagar/encender las mediciones, respectivamente.
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
#include "switch.h"
#include "led.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"

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
bool control=false;
/*!
	@brief variable que indica True si los datos mostrados por display no deben actualizarse
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

/** @brief Recibe una notificación, luego lee y almacena la distancia cada 1 segundo 
 * @return void
*/
void LeerDistancia(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
       		distancia = HcSr04ReadDistanceInCentimeters();
		}
    }
}

/** @brief Recibe una notificación y actualiza cada 1 segundo el display, leds y el puerto serie de la pc. Si el SWITCH 2 fue apretado el display queda no se actualiza
 * @return void
*/
void MostrarDistancia(void *pvParameter){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(control){
			if(!pausa){
				LcdItsE0803Write(distancia);
				UartSendString(UART_PC, (const char*)UartItoa(distancia, 10));
				UartSendString(UART_PC, " ");
				UartSendString(UART_PC, "cm");
				UartSendString(UART_PC, "\r\n");
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

/** @brief Timer que envia dos notificaciones cada 1 segundo a LeerDistancia y MostrarDistancia
 * @return void
*/
void FuncTimerA_Distancia (void *param){
	vTaskNotifyGiveFromISR(leer_distancia_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_distancia_handle, pdFALSE);
}

/** @brief lee 1 byte del puerto serie e identifica si se apreto la tecla H u O, y llama a una funcion diferente en cada caso
 * @return void
*/
void controlXTeclado(void *param){
	uint8_t tecla;
	UartReadByte(UART_PC, &tecla);
	switch (tecla){
		case 'O':
			LeerTeclaOnOff();
			break;
		case 'H':
			LeerTeclaHold();
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

	serial_config_t serial_puerto_pc ={
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = controlXTeclado,
		.param_p = NULL
	};

	UartInit(&serial_puerto_pc);

	xTaskCreate(&LeerDistancia, "LeerDistancia", 2048, NULL, 5, &leer_distancia_handle);
	xTaskCreate(&MostrarDistancia, "MostrarDistancia", 2048, NULL, 5, &mostrar_distancia_handle);
		
	//interrupciones	
	SwitchActivInt(SWITCH_1, &LeerTeclaOnOff, NULL);
	SwitchActivInt(SWITCH_2, &LeerTeclaHold, NULL);

	TimerStart(timer_distancia.timer);
}
/*==================[end of file]============================================*/