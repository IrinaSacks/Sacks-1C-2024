/*! @mainpage Osciloscopio
 *
 * @section genDesc General Description
 *
 * Este programa digitaliza una señal analógica y la transmite a un graficador de puerto serie de la PC. 
 * Además permite convertir una señal digital en una señal analógica y visualizarla con un graficador de puerto serie de la PC. 
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
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
/*!
	@brief periodo del timerA para la conversion analogico-digital en microsegundos
*/
#define CONFIG_PERIOD_AD 2000 
/*!
	@brief periodo del timerB para la conversion digital-analogico en microsegundos
*/
#define CONFIG_PERIOD_DA 4000
/*!
	@brief tamaño del arreglo que almacena los valores del ecg
*/
#define BUFFER_SIZE 231
/*==================[internal data definition]===============================*/
TaskHandle_t conversion_AD_handle = NULL;
TaskHandle_t conversion_DA_handle = NULL;
uint8_t contador_auxiliar=0;
/*!
	@brief arreglo de tamaño BUFFER_SIZE que almacena los valores de un ecg
*/
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};
/*==================[internal functions declaration]=========================*/
/** @brief Timer que envia una notificaciones cada 2000 mseg a conversionAD
 * @return void
*/
void tempAD (void *param){
	vTaskNotifyGiveFromISR(conversion_AD_handle, pdFALSE);
}
/** @brief Timer que envia una notificaciones cada 4000 mseg a conversionDA
 * @return void
*/
void tempDA (void *param){
	vTaskNotifyGiveFromISR(conversion_DA_handle, pdFALSE);
}

/** @brief Recibe una notificiacion cada 2000 mseg, lee y almacena la señal de entrada de CH1, convierte el dato a ASCII y lo envia por el puerto serie
 * @return void
*/
void conversionAD(void *pvParameter){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		uint16_t lectura_ecg;
		AnalogInputReadSingle(CH1, &lectura_ecg);
		UartSendString(UART_PC, (const char*)UartItoa(lectura_ecg, 10)); // Envio el dato convertido a ASCII
		UartSendString(UART_PC, "\r\n");		
	}
}

/** @brief Recibe una notificiacion cada 4000 mseg, lee cada valor del arreglo ecg, lo convierte en analogico y lo envia por el puerto serie
 * @return void
*/
void conversionDA(void *pvParameter){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(contador_auxiliar<BUFFER_SIZE){
			AnalogOutputWrite(ecg[contador_auxiliar]);
			contador_auxiliar++;
		}else if(contador_auxiliar==BUFFER_SIZE){
			contador_auxiliar=0;
		}
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){

	analog_input_config_t senal_analogica = {			
		.input= CH1,			
		.mode= ADC_SINGLE,		
		.func_p= NULL,			
		.param_p=NULL	
	};	
	AnalogInputInit(&senal_analogica);
	AnalogOutputInit();

	timer_config_t timerAD = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_AD,
        .func_p = tempAD,
        .param_p = NULL
    };
	TimerInit(&timerAD);

	timer_config_t timerDA = {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_DA,
        .func_p = tempDA,
        .param_p = NULL
    };
	TimerInit(&timerDA);

	serial_config_t serial_puerto_pc ={
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};
	UartInit(&serial_puerto_pc);

	xTaskCreate(&conversionAD, "conversion_AD", 2048, NULL, 5, &conversion_AD_handle); //tarea para conversion analogica a dig
	xTaskCreate(&conversionDA, "conversion_DA", 2048, NULL, 5, &conversion_DA_handle); //tarea para conversion dig a analogica

	//Recomendable ponerlo al final
	TimerStart(timerAD.timer);
	TimerStart(timerDA.timer);
}
/*==================[end of file]============================================*/