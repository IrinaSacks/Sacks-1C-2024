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
 * |   	L9110	    |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	B-1A	 	| 	GPIO_16		|
 * | 	B-1B	 	| 	GPIO_17		|
 * | 	A-1A	 	| 	GPIO_22		|
 * | 	A-1B	 	| 	GPIO_23		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |   TCRT5000-I   |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	OUT	 		| 	GPIO_18		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |   TCRT5000-D   |   ESP32   	|
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
#include "pwm_mcu.h"
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
/*!
	@brief periodo en microsegundos, para detectar la linea
*/
#define CONFIG_PERIOD_DET 150000 
/*!
	@brief periodo en milisegundos, para el parpadeo de los leds segun la conexion del BLE
*/
#define CONFIG_BLINK_PERIOD 500
/*!
	@brief periodo en milisegundos, para el control de la bateria
*/
#define PERIODO_BAT 5000 

/*!
	@brief definicion GPIO del detector de linea izquierdo
*/
#define DETECTOR_IZQ GPIO_18 
/*!
	@brief definicion GPIO del detector de linea derecho
*/
#define DETECTOR_DER GPIO_19 

/*!
	@brief definicion GPIO del motor DC izquierdo a controlar con PWM
*/
#define MIA GPIO_16 
/*!
	@brief definicion GPIO del motor DC izquierdo
*/
#define	MIB GPIO_22 
/*!
	@brief definicion GPIO del motor DC derecho a controlar con PWM
*/
#define	MDA GPIO_17 
/*!
	@brief definicion GPIO del motor DC derecho
*/
#define MDB	GPIO_23 
/*==================[internal data definition]===============================*/
/**
 * @brief estructura del tipo gpioConf_t
 * 	asigna el numero de pin y establece si es entrada o salida con 0 o 1, respectivamente.
*/
typedef struct 
{   gpio_t pin;			
	io_t dir;			
} gpioConf_t;

TaskHandle_t detectar_linea_handle = NULL;

/**
 * @brief arreglo del tipo gpioConf_t
 * 	almacena los pines de los detectores de linea y si son salida o entrada
*/
gpioConf_t gpio_detector_linea[2]; 
/**
 * @brief arreglo del tipo pwm_out_t
 * 	almacena cual sera salida PWM
*/
pwm_out_t pwm_control_motores[2]; 

bool encendido, pausa = false;
char direccion='X';
/*==================[internal functions declaration]=========================*/
/** @brief Timer que envia una notificacion cada 0,15 seg a DetectarLinea
 * @return void
*/
void FuncTimerA_Detector (void* param){
	vTaskNotifyGiveFromISR(detectar_linea_handle, pdFALSE);
}

/**
 * @brief Función a ejecutarse ante un interrupción de recepción a través de la conexión BLE.
 * Si recibe 'O' enciende el sistema y un led verde, 'o' apaga el sistema y enciende led verde
 * Si el sistema esta encendido y recibe 'P' detiene el robot y enciende led amarillo, 'p' reanuda el robot y apaga el led amarillo.
 * Para ambos casos se informa por puerto serie el estado de la marcha del robot.
 * @param data      Puntero a array de datos recibidos
 * @param length    Longitud del array de datos recibidos
 */
void read_data(uint8_t * data, uint8_t length){
	switch(data[0]){
        case 'O':
            encendido = true;
			BleSendString("RR0G0B0*"); 
            BleSendString("AR0G0B0*"); 
			BleSendString("*VR0G255B0*"); 
			BleSendString("*MRobot en marcha\n*"); 
		break;
        case 'o':
            encendido = false;
			BleSendString("VR0G0B0*"); 
            BleSendString("AR0G0B0*"); 
			BleSendString("*RR255G0B0*"); 
			BleSendString("*MRobot estacionado\n*"); 
		break;
		case 'P':
            if(encendido == true){
			    pausa = true;
			    BleSendString("*AR222G206B42*"); 
			    BleSendString("*MRobot pausado\n*"); 
            }
		break;
		case 'p':
            if(encendido == true){
			    pausa = false;
			    BleSendString("AR0G0B0*"); 
				BleSendString("*MRobot reanudado\n*");
			}
        break;		
		default:
		break;
    }
}

/**
 * @brief Funcion que controla los motores DC y modifica su velocidad utilizando PWM 
 * MIB y MDB deben estar en HIGH para que el robot vaya hacia adelante.
 */
void ControlMotores(){ 
	GPIOOn(MIB);
	GPIOOn(MDB);
	switch(direccion){
		case 'A':
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			PWMSetDutyCycle(pwm_control_motores[0], 65);
			PWMSetDutyCycle(pwm_control_motores[1], 65);
			break;
		case 'I':
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			PWMSetDutyCycle(pwm_control_motores[0], 90); 
			PWMSetDutyCycle(pwm_control_motores[1], 30); 
			break;
		case 'D':
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			PWMSetDutyCycle(pwm_control_motores[0], 30); 
			PWMSetDutyCycle(pwm_control_motores[1], 90); 	
			break;
		case 'F':
			PWMSetDutyCycle(pwm_control_motores[0], 100); 
			PWMSetDutyCycle(pwm_control_motores[1], 100); 	
			break;

		default:
			break;
	}
}

/** @brief Recibe una notificiacion cada 0,15 seg, lee la salida de los detectores de linea y llama a la funcion ControlMotores y le pasa
 * un caracter con la direccion que debe tomar el robot.
 * @return void
*/
static void DetectarLinea(void *pvParameter){ 
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     
		bool det_izq = GPIORead(gpio_detector_linea[0].pin);
		bool det_der = GPIORead(gpio_detector_linea[1].pin);
		if(encendido == true && pausa == false){
			if(det_izq==true && det_der==true){
				direccion = 'A';
			}
			else if(det_izq==true && det_der==false){
				direccion = 'D';
			}
			else if(det_izq==false && det_der==true){
				direccion = 'I';
			}
			else if(det_izq==false && det_der==false){
				direccion = 'F';
			}
			ControlMotores();

		}else if((encendido==true && pausa==true) || encendido ==false){
			direccion = 'F';
			ControlMotores();
		}else if(encendido==false){
			direccion = 'F';
			ControlMotores();
		}
		
	}
}

/** @brief Funcion llamada cada 5 seg, si el sistema esta encendido convierte la salida analogica del CH1 en digital y a mV, 
 * la representa en porcentaje y envia un mensaje por el puerto serie sobre el estado de la bateria.
 * @return void
*/
static void ControlBateria(void *pvParameter){ 
	uint16_t nivel_bat_analog, nivel_bat_dig;
	char niv_bateria[50];
	niv_bateria[0] = '\0';
	char aux_bat[200];
	aux_bat[0] = '\0';
	while(1){
		if(encendido == true){
			AnalogInputReadSingle(CH1, &nivel_bat_analog);
			nivel_bat_dig = (nivel_bat_analog*100/2100); 
			sprintf(niv_bateria, "%u", nivel_bat_dig);

			if(nivel_bat_dig >=0 && nivel_bat_dig<=10){
				sprintf(aux_bat,"*BEl nivel de bateria es de: %s%% \n NIVEL BATERIA CRITICO! Cambie las baterias...\n\n*", niv_bateria);
			}else if(nivel_bat_dig >10 && nivel_bat_dig <=20){
				sprintf(aux_bat,"*BEl nivel de bateria es de: %s%% \n NIVEL DE BATERIA BAJO!\n\n*", niv_bateria);
			}else if(nivel_bat_dig >20 && nivel_bat_dig<=50){
				sprintf(aux_bat,"*BEl nivel de bateria es de: %s%% \n NIVEL DE BATERIA MEDIO\n\n*", niv_bateria);
			}else if(nivel_bat_dig>50 && nivel_bat_dig <=100){
				sprintf(aux_bat,"*BEl nivel de bateria es de: %s%% \n NIVEL DE BATERIA BUENO\n\n*", niv_bateria);	
			}
			BleSendString(aux_bat);
		}
		vTaskDelay(PERIODO_BAT/portTICK_PERIOD_MS);
	}		
}

/*==================[external functions definition]==========================*/
void app_main(void){

    LedsInit();

    ble_config_t ble_configuration = {
        "ROBOT_SEG_LINEA",
        read_data
    };
    BleInit(&ble_configuration);

	gpio_detector_linea[0].pin=DETECTOR_IZQ; 
    gpio_detector_linea[1].pin=DETECTOR_DER;
	for(int i=0; i<2;i++){
		gpio_detector_linea[i].dir=GPIO_INPUT; 
	}
	for(int i=0; i<2;i++){
		GPIOInit(gpio_detector_linea[i].pin, gpio_detector_linea[i].dir);
	}

    gpioConf_t gpio_control_motores[4]; 
    gpio_control_motores[0].pin=MIB; 
    gpio_control_motores[1].pin=MDB; 
	gpio_control_motores[2].pin=MIA; 
	gpio_control_motores[3].pin=MDA;  	

    for(int i=0; i<4;i++){
		gpio_control_motores[i].dir=GPIO_OUTPUT; 
	}	
    for(int i=0; i<4;i++){
		GPIOInit(gpio_control_motores[i].pin, gpio_control_motores[i].dir);
	}

	pwm_control_motores[0]=PWM_0;
	pwm_control_motores[1]=PWM_1;
	PWMInit(pwm_control_motores[0], MIA, 50);
	PWMInit(pwm_control_motores[1], MDA, 50);

	analog_input_config_t senal_analogica_bat = {			
		.input= CH1,			
		.mode= ADC_SINGLE,		
		.func_p= NULL,			
		.param_p=NULL	
	};	
	AnalogInputInit(&senal_analogica_bat);

    timer_config_t timer_detector = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_DET,
        .func_p = FuncTimerA_Detector,
        .param_p = NULL  
    };
	TimerInit(&timer_detector);
	
	xTaskCreate(&DetectarLinea, "DetectarLinea", 2048, NULL, 5, &detectar_linea_handle); 
	xTaskCreate(&ControlBateria, "ControlBateria", 2048, NULL, 5, NULL);

	TimerStart(timer_detector.timer);

    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
				LedOff(LED_1);
				LedOff(LED_2);
                LedOn(LED_3);
                encendido = false;
				pausa=false;
            break;
            case BLE_DISCONNECTED:
                LedOff(LED_1);
                LedToggle(LED_2);
				LedOff(LED_3);
                encendido = false;
				pausa=false;
            break;
            case BLE_CONNECTED:
				LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            break;
        }
    }

}